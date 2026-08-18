// Nintendo Switch homebrew smoke test.
//
// Two jobs:
//   1. prove the devkitPro toolchain, EGL bring-up and .nro packaging work end to end
//   2. report what the Switch GL driver actually exposes at runtime
//
// Static analysis of the portlibs says the driver is OpenGL 4.3 core profile with no
// fixed-function entry points. That determines whether vanilla SFML can ever work here,
// so this prints the real strings and compiles a core-profile shader to confirm it on
// hardware.
//
// Output goes over nxlink (run `nxlink -s switch_smoke.nro` from the dev machine).

#include <switch.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <glad/glad.h>

#include <cstdio>

namespace
{

EGLDisplay egl_display = nullptr;
EGLContext egl_context = nullptr;
EGLSurface egl_surface = nullptr;

bool initializeEgl(NWindow* native_window)
{
   egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   if (!egl_display)
   {
      printf("eglGetDisplay failed: 0x%x\n", eglGetError());
      return false;
   }

   if (eglInitialize(egl_display, nullptr, nullptr) == EGL_FALSE)
   {
      printf("eglInitialize failed: 0x%x\n", eglGetError());
      return false;
   }

   // request desktop OpenGL rather than OpenGL ES; the Switch mesa/nouveau driver
   // supports both, and the engine's shaders target desktop GL
   if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
   {
      printf("eglBindAPI failed: 0x%x\n", eglGetError());
      return false;
   }

   EGLConfig egl_config = nullptr;
   EGLint config_count = 0;

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

   eglChooseConfig(egl_display, config_attribute_list, &egl_config, 1, &config_count);
   if (config_count == 0)
   {
      printf("eglChooseConfig found no matching config: 0x%x\n", eglGetError());
      return false;
   }

   egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, nullptr);
   if (!egl_surface)
   {
      printf("eglCreateWindowSurface failed: 0x%x\n", eglGetError());
      return false;
   }

   const EGLint context_attribute_list[] = {
      EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
      EGL_CONTEXT_MAJOR_VERSION,       4,
      EGL_CONTEXT_MINOR_VERSION,       3,
      EGL_NONE
   };

   egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribute_list);
   if (!egl_context)
   {
      printf("eglCreateContext failed: 0x%x\n", eglGetError());
      return false;
   }

   eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
   return true;
}

void deinitializeEgl()
{
   if (!egl_display)
   {
      return;
   }

   eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

   if (egl_context)
   {
      eglDestroyContext(egl_display, egl_context);
      egl_context = nullptr;
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
   printf("---- gl driver ----\n");
   printf("GL_VENDOR                   : %s\n", glGetString(GL_VENDOR));
   printf("GL_RENDERER                 : %s\n", glGetString(GL_RENDERER));
   printf("GL_VERSION                  : %s\n", glGetString(GL_VERSION));
   printf("GL_SHADING_LANGUAGE_VERSION : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

   GLint major_version = 0;
   GLint minor_version = 0;
   GLint profile_mask = 0;
   glGetIntegerv(GL_MAJOR_VERSION, &major_version);
   glGetIntegerv(GL_MINOR_VERSION, &minor_version);
   glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile_mask);

   printf("context                     : %d.%d\n", major_version, minor_version);
   printf("core profile bit            : %s\n", (profile_mask & GL_CONTEXT_CORE_PROFILE_BIT) ? "yes" : "no");
   printf("compatibility profile bit   : %s\n", (profile_mask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) ? "yes" : "no");
}

// Compiles a core-profile shader. If this succeeds the engine's WASM-era GLES3/core
// shader variants are the right starting point for the Switch renderer.
void reportShaderSupport()
{
   const auto* vertex_shader_source =
      "#version 430 core\n"
      "layout(location = 0) in vec2 in_position;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = vec4(in_position, 0.0, 1.0);\n"
      "}\n";

   const auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
   glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
   glCompileShader(vertex_shader);

   GLint compile_status = 0;
   glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compile_status);

   printf("---- shaders ----\n");
   printf("#version 430 core compile   : %s\n", compile_status ? "ok" : "FAILED");

   if (!compile_status)
   {
      char info_log[1024] = {};
      glGetShaderInfoLog(vertex_shader, sizeof(info_log), nullptr, info_log);
      printf("compile log                 : %s\n", info_log);
   }

   glDeleteShader(vertex_shader);
}

}  // namespace

int main(int argc, char* argv[])
{
   // route stdout to the host over nxlink
   socketInitializeDefault();
   nxlinkStdio();

   printf("deceptus switch smoke test\n");

   if (!initializeEgl(nwindowGetDefault()))
   {
      printf("egl bring-up failed, aborting\n");
      socketExit();
      return 1;
   }

   gladLoadGL();

   reportDriverCapabilities();
   reportShaderSupport();

   padConfigureInput(1, HidNpadStyleSet_NpadStandard);
   PadState pad;
   padInitializeDefault(&pad);

   printf("---- running, press + to exit ----\n");

   while (appletMainLoop())
   {
      padUpdate(&pad);
      if (padGetButtonsDown(&pad) & HidNpadButton_Plus)
      {
         break;
      }

      glClearColor(0.15f, 0.1f, 0.25f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      eglSwapBuffers(egl_display, egl_surface);
   }

   deinitializeEgl();
   socketExit();
   return 0;
}
