#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

#include <SDL.h>
#include <stdio.h>

#include "backends/imgui_impl_sdl.h"
#include "backends/imgui_impl_sdlrenderer.h"
#include "imgui.h"

namespace
{

struct Level
{
   std::filesystem::path _dir;
   std::filesystem::path _json_file_path;
};

std::vector<Level> findLevelPaths()
{
   std::vector<Level> results;
   std::vector<std::string> filter = {"config", "sprites"};
   std::string path = "data";

   for (const auto& potential_dir : std::filesystem::directory_iterator(path))
   {
      const auto basename = potential_dir.path().filename().string();
      if (std::find(filter.begin(), filter.end(), basename) != filter.end())
      {
         continue;
      }

      if (potential_dir.is_directory())
      {
         for (const auto& file : std::filesystem::directory_iterator(potential_dir))
         {
            const auto path = file.path().string();
            if (path.find(".json") != std::string::npos)
            {
               Level level;
               level._dir = potential_dir;
               level._json_file_path = path;
               results.push_back(level);
               // std::cout << file.path() << std::endl;
            }
         }
         // std::cout << entry.path() << std::endl;
      };
   }

   return results;
}

bool drawComboBox(const std::string& caption, std::string& current_item, const std::vector<std::string>& items)
{
   bool changed = false;

   if (ImGui::BeginCombo(caption.c_str(), current_item.c_str()))
   {
      for (auto n = 0; n < items.size(); n++)
      {
         const auto is_selected = (current_item == items[n]);

         if (ImGui::Selectable(items[n].c_str(), is_selected))
         {
            current_item = items[n];
            changed = true;
         }

         if (is_selected)
         {
            ImGui::SetItemDefaultFocus();
         }
      }

      ImGui::EndCombo();
   }

   return changed;
}
}  // namespace

int WinMain(int, char**)
{
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
   {
      printf("Error: %s\n", SDL_GetError());
      return -1;
   }

   // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
   SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

   // Create window with SDL_Renderer graphics context
   SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
   auto window = SDL_CreateWindow("Deceptus Level Selector", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 360, 720, window_flags);
   auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
   if (!renderer)
   {
      SDL_Log("Error creating SDL_Renderer!");
      return 0;
   }

   // Setup Dear ImGui context
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO();
   (void)io;

   // Setup Dear ImGui style
   ImGui::StyleColorsDark();

   // Setup Platform/Renderer backends
   ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
   ImGui_ImplSDLRenderer_Init(renderer);
   // Our state
   bool show_demo_window = true;
   bool show_another_window = false;
   ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

   // Main loop
   bool done = false;
   while (!done)
   {
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
         ImGui_ImplSDL2_ProcessEvent(&event);
         if (event.type == SDL_QUIT)
            done = true;
         if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
            done = true;
      }

      // Start the Dear ImGui frame
      ImGui_ImplSDLRenderer_NewFrame();
      ImGui_ImplSDL2_NewFrame();
      ImGui::NewFrame();

      {
         // Using the _simplified_ one-liner Combo() api here
         // See "Combo" section for examples of how to use the more complete BeginCombo()/EndCombo() api.
      }

      {
         static float f = 0.0f;
         static int counter = 0;

         ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!" and append into it.

         const auto results = findLevelPaths();
         std::vector<std::string> names;

         names = {"bla", "blub", "blib"};

         if (drawComboBox("levels", names[2], names))
         {
         }
         // std::transform(results.begin(), results.back(), names.begin(), [](const Level& result) { return result._dir.string(); });
         // drawComboBox()
         //         const char* items[] = {
         //            "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM",
         //            "OOOOOOO"};
         //         static int item_current = 0;
         //         ImGui::Combo("combo", &item_current, items, IM_ARRAYSIZE(items));
         //         ImGui::SameLine();

         ImGui::Text("This is some useful text.");           // Display some text (you can use a format strings too)
         ImGui::Checkbox("Demo Window", &show_demo_window);  // Edit bools storing our window open/close state
         ImGui::Checkbox("Another Window", &show_another_window);

         ImGui::SliderFloat("float", &f, 0.0f, 1.0f);             // Edit 1 float using a slider from 0.0f to 1.0f
         ImGui::ColorEdit3("clear color", (float*)&clear_color);  // Edit 3 floats representing a color

         if (ImGui::Button("Button"))  // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
         ImGui::SameLine();
         ImGui::Text("counter = %d", counter);

         ImGui::End();
      }

      // 3. Show another simple window.
      if (show_another_window)
      {
         ImGui::Begin(
            "Another Window", &show_another_window
         );  // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
         ImGui::Text("Hello from another window!");
         if (ImGui::Button("Close Me"))
            show_another_window = false;
         ImGui::End();
      }

      // Rendering
      ImGui::Render();
      SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
      SDL_SetRenderDrawColor(
         renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255)
      );
      SDL_RenderClear(renderer);
      ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
      SDL_RenderPresent(renderer);
   }

   // Cleanup
   ImGui_ImplSDLRenderer_Shutdown();
   ImGui_ImplSDL2_Shutdown();
   ImGui::DestroyContext();

   SDL_DestroyRenderer(renderer);
   SDL_DestroyWindow(window);
   SDL_Quit();

   return 0;
}
