// Nintendo Switch framebuffer-object probe.
//
// The engine gets as far as creating its first sf::RenderTexture and then fails with
// "Impossible to create render texture (failed to link the target texture to the
// framebuffer)". That message only says glCheckFramebufferStatus was not COMPLETE; it
// does not say why.
//
// This probe replicates VRSFML's exact call sequence and varies one thing at a time, so a
// single run says which ingredient the driver rejects:
//
//   - the plain colour-only FBO (what RenderTargets::create asks for first)
//   - the stencil variants, since the engine asks for stencilBits = 8
//   - the same colour-only FBO with the texture created on a *different, shared* context,
//     which is what VRSFML actually does (Texture::create runs under GLSharedContextGuard
//     while the FBO is created on the window context)
//
// Output goes to stderr, which reaches svcOutputDebugString via consoleDebugInit and shows
// up in Ryujinx's guest log as "KernelSvc OutputDebugString".

#include <switch.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <glad/glad.h>

#include <cstdio>

namespace
{

EGLDisplay egl_display = nullptr;
EGLConfig egl_config = nullptr;
EGLSurface egl_surface = nullptr;
EGLContext egl_context_primary = nullptr;
EGLContext egl_context_secondary = nullptr;

// VRSFML's RenderTargets ask for textures the size of the scaled view; 1280x720 is what
// the Switch NWindow reports in handheld mode, so probe at that size rather than a
// convenient power of two.
constexpr GLsizei probe_width = 1280;
constexpr GLsizei probe_height = 720;

const char* framebufferStatusToString(GLenum status)
{
   switch (status)
   {
      case GL_FRAMEBUFFER_COMPLETE:
         return "GL_FRAMEBUFFER_COMPLETE";
      case GL_FRAMEBUFFER_UNDEFINED:
         return "GL_FRAMEBUFFER_UNDEFINED";
      case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
         return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
      case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
         return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
      case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
         return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
      case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
         return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
      case GL_FRAMEBUFFER_UNSUPPORTED:
         return "GL_FRAMEBUFFER_UNSUPPORTED";
      case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
         return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
      case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
         return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
      default:
         return "unknown";
   }
}

void reportGlErrors(const char* stage)
{
   for (GLenum error = glGetError(); error != GL_NO_ERROR; error = glGetError())
   {
      fprintf(stderr, "[fbo-probe]   gl error after %s: 0x%04x\n", stage, error);
   }
}

// Creates a texture exactly the way sf::priv::bindAndInitializeTexture does: unsized
// GL_RGBA internal format, no mipmaps, nearest filtering.
GLuint createVrsfmlStyleTexture()
{
   GLuint texture = 0;
   glGenTextures(1, &texture);
   glBindTexture(GL_TEXTURE_2D, texture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, probe_width, probe_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   reportGlErrors("glTexImage2D(GL_RGBA)");
   return texture;
}

GLuint createSizedTexture(GLenum internal_format)
{
   GLuint texture = 0;
   glGenTextures(1, &texture);
   glBindTexture(GL_TEXTURE_2D, texture);
   glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internal_format), probe_width, probe_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   reportGlErrors("glTexImage2D(sized)");
   return texture;
}

// Attaches the given texture as colour attachment 0 and reports completeness. If a
// renderbuffer format is given it is created and attached the way VRSFML does.
void probeFramebuffer(const char* label, GLuint texture, GLenum renderbuffer_format, GLenum renderbuffer_attachment, bool use_multisample_storage_entry_point)
{
   GLuint framebuffer = 0;
   glGenFramebuffers(1, &framebuffer);
   glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
   reportGlErrors("glFramebufferTexture2D");

   GLuint renderbuffer = 0;
   if (renderbuffer_format != GL_NONE)
   {
      glGenRenderbuffers(1, &renderbuffer);
      glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);

      if (use_multisample_storage_entry_point)
      {
         // VRSFML always goes through the multisample entry point, passing 0 samples when
         // anti-aliasing is off. Worth proving that is equivalent here.
         glRenderbufferStorageMultisample(GL_RENDERBUFFER, 0, renderbuffer_format, probe_width, probe_height);
         reportGlErrors("glRenderbufferStorageMultisample");
      }
      else
      {
         glRenderbufferStorage(GL_RENDERBUFFER, renderbuffer_format, probe_width, probe_height);
         reportGlErrors("glRenderbufferStorage");
      }

      glFramebufferRenderbuffer(GL_FRAMEBUFFER, renderbuffer_attachment, GL_RENDERBUFFER, renderbuffer);
      reportGlErrors("glFramebufferRenderbuffer");
   }

   const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   fprintf(stderr, "[fbo-probe] %-46s : 0x%04x %s\n", label, status, framebufferStatusToString(status));

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glDeleteFramebuffers(1, &framebuffer);

   if (renderbuffer != 0)
   {
      glDeleteRenderbuffers(1, &renderbuffer);
   }
}

bool initializeEgl(NWindow* native_window)
{
   egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   if (!egl_display)
   {
      fprintf(stderr, "[fbo-probe] eglGetDisplay failed: 0x%x\n", eglGetError());
      return false;
   }

   if (eglInitialize(egl_display, nullptr, nullptr) == EGL_FALSE)
   {
      fprintf(stderr, "[fbo-probe] eglInitialize failed: 0x%x\n", eglGetError());
      return false;
   }

   if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
   {
      fprintf(stderr, "[fbo-probe] eglBindAPI failed: 0x%x\n", eglGetError());
      return false;
   }

   // matches what the engine asks for through sf::ContextSettings: 8 stencil bits
   const EGLint config_attribute_list[] = {
      EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
      EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
      EGL_RED_SIZE,        8,
      EGL_GREEN_SIZE,      8,
      EGL_BLUE_SIZE,       8,
      EGL_ALPHA_SIZE,      8,
      EGL_DEPTH_SIZE,      24,
      EGL_STENCIL_SIZE,    8,
      EGL_NONE
   };

   EGLint config_count = 0;
   eglChooseConfig(egl_display, config_attribute_list, &egl_config, 1, &config_count);
   if (config_count == 0)
   {
      fprintf(stderr, "[fbo-probe] eglChooseConfig found no matching config: 0x%x\n", eglGetError());
      return false;
   }

   egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, nullptr);
   if (!egl_surface)
   {
      fprintf(stderr, "[fbo-probe] eglCreateWindowSurface failed: 0x%x\n", eglGetError());
      return false;
   }

   const EGLint context_attribute_list[] = {
      EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
      EGL_CONTEXT_MAJOR_VERSION,       4,
      EGL_CONTEXT_MINOR_VERSION,       3,
      EGL_NONE
   };

   egl_context_primary = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribute_list);
   if (!egl_context_primary)
   {
      fprintf(stderr, "[fbo-probe] eglCreateContext (primary) failed: 0x%x\n", eglGetError());
      return false;
   }

   // VRSFML creates a hidden context first and every later context shares with it, so the
   // second context here is created the same way
   egl_context_secondary = eglCreateContext(egl_display, egl_config, egl_context_primary, context_attribute_list);
   if (!egl_context_secondary)
   {
      fprintf(stderr, "[fbo-probe] eglCreateContext (secondary, shared) failed: 0x%x\n", eglGetError());
      return false;
   }

   eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context_primary);
   return true;
}

void deinitializeEgl()
{
   if (!egl_display)
   {
      return;
   }

   eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

   if (egl_context_secondary)
   {
      eglDestroyContext(egl_display, egl_context_secondary);
      egl_context_secondary = nullptr;
   }

   if (egl_context_primary)
   {
      eglDestroyContext(egl_display, egl_context_primary);
      egl_context_primary = nullptr;
   }

   if (egl_surface)
   {
      eglDestroySurface(egl_display, egl_surface);
      egl_surface = nullptr;
   }

   eglTerminate(egl_display);
   egl_display = nullptr;
}

void reportDriverCapabilities()
{
   fprintf(stderr, "[fbo-probe] ---- gl driver ----\n");
   fprintf(stderr, "[fbo-probe] GL_VENDOR   : %s\n", glGetString(GL_VENDOR));
   fprintf(stderr, "[fbo-probe] GL_RENDERER : %s\n", glGetString(GL_RENDERER));
   fprintf(stderr, "[fbo-probe] GL_VERSION  : %s\n", glGetString(GL_VERSION));
   fprintf(stderr, "[fbo-probe] GL_SHADING_LANGUAGE_VERSION : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

   GLint major_version = 0;
   GLint minor_version = 0;
   GLint profile_mask = 0;
   GLint max_texture_size = 0;
   GLint max_color_attachments = 0;
   GLint max_samples = 0;
   glGetIntegerv(GL_MAJOR_VERSION, &major_version);
   glGetIntegerv(GL_MINOR_VERSION, &minor_version);
   glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile_mask);
   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
   glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
   glGetIntegerv(GL_MAX_SAMPLES, &max_samples);

   fprintf(stderr, "[fbo-probe] context %d.%d, core=%s compat=%s\n",
           major_version,
           minor_version,
           (profile_mask & GL_CONTEXT_CORE_PROFILE_BIT) ? "yes" : "no",
           (profile_mask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) ? "yes" : "no");
   fprintf(stderr, "[fbo-probe] GL_MAX_TEXTURE_SIZE %d, GL_MAX_COLOR_ATTACHMENTS %d, GL_MAX_SAMPLES %d\n",
           max_texture_size,
           max_color_attachments,
           max_samples);
   reportGlErrors("capability queries");
}

void runSingleContextProbes()
{
   fprintf(stderr, "[fbo-probe] ---- single context, %dx%d ----\n", probe_width, probe_height);

   const auto vrsfml_texture = createVrsfmlStyleTexture();
   probeFramebuffer("colour only (GL_RGBA texture)", vrsfml_texture, GL_NONE, GL_NONE, false);
   probeFramebuffer("colour + STENCIL_INDEX8 (multisample entry)", vrsfml_texture, GL_STENCIL_INDEX8, GL_STENCIL_ATTACHMENT, true);
   probeFramebuffer("colour + STENCIL_INDEX8 (plain entry)", vrsfml_texture, GL_STENCIL_INDEX8, GL_STENCIL_ATTACHMENT, false);
   probeFramebuffer("colour + DEPTH24_STENCIL8 -> STENCIL", vrsfml_texture, GL_DEPTH24_STENCIL8, GL_STENCIL_ATTACHMENT, false);
   probeFramebuffer("colour + DEPTH24_STENCIL8 -> DEPTH_STENCIL", vrsfml_texture, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, false);
   probeFramebuffer("colour + DEPTH_COMPONENT16 -> DEPTH", vrsfml_texture, GL_DEPTH_COMPONENT16, GL_DEPTH_ATTACHMENT, false);
   glDeleteTextures(1, &vrsfml_texture);

   const auto rgba8_texture = createSizedTexture(GL_RGBA8);
   probeFramebuffer("colour only (sized GL_RGBA8 texture)", rgba8_texture, GL_NONE, GL_NONE, false);
   glDeleteTextures(1, &rgba8_texture);
}

// VRSFML creates every texture on its shared context and every FBO on the context that is
// current at the time. If EGL sharing is not doing what SFML assumes, the texture name is
// simply unknown to the second context and the attachment comes back incomplete.
void runCrossContextProbe()
{
   fprintf(stderr, "[fbo-probe] ---- cross context (texture on shared ctx, fbo on window ctx) ----\n");

   eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context_primary);
   const auto texture = createVrsfmlStyleTexture();
   glFlush();
   fprintf(stderr, "[fbo-probe] texture %u created on primary context\n", texture);

   if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context_secondary) == EGL_FALSE)
   {
      fprintf(stderr, "[fbo-probe] eglMakeCurrent(secondary) failed: 0x%x\n", eglGetError());
      return;
   }

   const auto is_texture = glIsTexture(texture);
   fprintf(stderr, "[fbo-probe] glIsTexture on secondary context: %s\n", is_texture ? "yes" : "NO (sharing is broken)");

   probeFramebuffer("colour only, texture from other context", texture, GL_NONE, GL_NONE, false);

   eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context_primary);
   glDeleteTextures(1, &texture);
}

}  // namespace

int main(int argc, char* argv[])
{
   // stderr reaches svcOutputDebugString, which Ryujinx prints in its guest log; stdout
   // would need nxlink and a network connection
   consoleDebugInit(debugDevice_SVC);

   fprintf(stderr, "[fbo-probe] deceptus switch fbo probe\n");

   if (!initializeEgl(nwindowGetDefault()))
   {
      fprintf(stderr, "[fbo-probe] egl bring-up failed, aborting\n");
      return 1;
   }

   gladLoadGL();

   reportDriverCapabilities();
   runSingleContextProbes();
   runCrossContextProbe();

   fprintf(stderr, "[fbo-probe] done\n");

   padConfigureInput(1, HidNpadStyleSet_NpadStandard);
   PadState pad;
   padInitializeDefault(&pad);

   while (appletMainLoop())
   {
      padUpdate(&pad);
      if (padGetButtonsDown(&pad) & HidNpadButton_Plus)
      {
         break;
      }

      // a distinctive clear colour makes it obvious in a screenshot that the probe got
      // past every GL call rather than hanging somewhere
      glClearColor(0.05f, 0.35f, 0.15f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      eglSwapBuffers(egl_display, egl_surface);
   }

   deinitializeEgl();
   return 0;
}
