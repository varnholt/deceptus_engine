#include "editor.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cstdio>

static void glfwErrorCallback(int error_code, const char* description)
{
   std::fprintf(stderr, "GLFW error %d: %s\n", error_code, description);
}

int main()
{
   glfwSetErrorCallback(glfwErrorCallback);
   if (!glfwInit())
   {
      return 1;
   }

   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

   GLFWwindow* const window = glfwCreateWindow(1600, 900, "deceptus cutscene editor", nullptr, nullptr);
   if (!window)
   {
      glfwTerminate();
      return 1;
   }

   glfwMakeContextCurrent(window);
   glfwSwapInterval(1);

   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGui::StyleColorsDark();

   ImGui_ImplGlfw_InitForOpenGL(window, true);
   ImGui_ImplOpenGL3_Init("#version 330");

   ImGui::LoadIniSettingsFromDisk("cutscene_editor.ini");

   Editor editor;

   while (!glfwWindowShouldClose(window))
   {
      glfwPollEvents();

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      editor.draw();

      ImGui::Render();

      int framebuffer_width  = 0;
      int framebuffer_height = 0;
      glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
      glViewport(0, 0, framebuffer_width, framebuffer_height);
      glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      glfwSwapBuffers(window);
   }

   ImGui::SaveIniSettingsToDisk("cutscene_editor.ini");

   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplGlfw_Shutdown();
   ImGui::DestroyContext();

   glfwDestroyWindow(window);
   glfwTerminate();

   return 0;
}
