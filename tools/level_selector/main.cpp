#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <SDL.h>
#include <stdio.h>

#include "backends/imgui_impl_sdl.h"
#include "backends/imgui_impl_sdlrenderer.h"
#include "imgui.h"

namespace
{

struct LevelInfo
{
   std::filesystem::path _dir;
   std::filesystem::path _json_file_path;
   std::string _selected;
   int32_t _index = 0;
};

std::vector<std::shared_ptr<LevelInfo>> findLevelPaths()
{
   std::vector<std::shared_ptr<LevelInfo>> results;
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
               auto result = std::make_shared<LevelInfo>();
               result->_dir = potential_dir;
               result->_json_file_path = path;
               result->_index = results.size();
               results.push_back(result);
            }
         }
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

void writeLevelConfiguration(const std::vector<std::shared_ptr<LevelInfo>>& results)
{
   std::stringstream level_json;
   auto serialize_level = [&level_json, results](const auto& result)
   {
      auto path = result->_json_file_path.string();
      std::replace(path.begin(), path.end(), '\\', '/');
      level_json << "   {\"levelname\" : \"" << path << "\"}";
      if (result != results.at(results.size() - 1))
      {
         level_json << ",";
      }
      level_json << "\n";
   };

   level_json << "[\n";
   std::for_each(results.begin(), results.end(), serialize_level);
   level_json << "]";

   // std::cout << level_json.str() << std::endl;

   std::ofstream file("data/config/levels.json", std::ios::out);
   file << level_json.str();
}

void writeSaveState(const std::vector<std::shared_ptr<LevelInfo>>& results)
{
   auto valid_count = 0;
   for (auto& result : results)
   {
      try
      {
         std::stoi(result->_selected);
         valid_count++;
      }
      catch (const std::exception&)
      {
      }
   }

   std::stringstream state_json;
   state_json << "[\n";
   auto serialize_level = [&state_json, valid_count](const auto& result, int32_t index)
   {
      const auto level_name = result->_dir.filename().string();

      state_json << "{\n";
      state_json << "   \"checkpoint\" : 0,\n";
      state_json << "   \"levelindex\" : " << result->_index << ",\n";
      state_json << "   \"playerinfo\" : {\"name\" : \"" << level_name << "\"}\n";
      state_json << "}";

      if (index < valid_count - 1)
      {
         state_json << ",";
      }
      state_json << "\n";
   };

   for (auto slot = 0; slot < 3; slot++)
   {
      for (auto& result : results)
      {
         try
         {
            // std::cout << result->_selected << std::endl;
            if (std::stoi(result->_selected) - 1 == slot)
            {
               serialize_level(result, slot);
            }
         }
         catch (const std::exception&)
         {
         }
      }
   }

   state_json << "]\n";

   // std::cout << state_json.str() << std::endl;
   std::ofstream file("data/config/savestate.json", std::ios::out);
   file << state_json.str();
}

}  // namespace

int WinMain(int, char**)
{
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
   {
      printf("Error: %s\n", SDL_GetError());
      return -1;
   }

   SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
   auto window = SDL_CreateWindow("Deceptus Level Selector", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 360, 360, window_flags);
   auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
   if (!renderer)
   {
      SDL_Log("Error creating SDL_Renderer!");
      return 0;
   }

   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO();
   (void)io;

   ImGui::StyleColorsDark();

   ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
   ImGui_ImplSDLRenderer_Init(renderer);

   const auto results = findLevelPaths();
   std::vector<std::string> names;
   std::transform(
      results.cbegin(),
      results.cend(),
      std::back_insert_iterator(names),
      [](auto& result)
      {
         std::string dir = result->_dir.filename().string();
         return dir;
      }
   );

   std::vector<std::string> slots = {"n/a", "1", "2", "3"};

   for (const auto& result : results)
   {
      result->_selected = slots[0];
   }

   auto done = false;
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

      ImGui_ImplSDLRenderer_NewFrame();
      ImGui_ImplSDL2_NewFrame();
      ImGui::NewFrame();

      {
         ImGui::Begin("assign level to slot");

         for (const auto& result : results)
         {
            const auto level_name = result->_dir.filename().string();
            std::stringstream stream_slot;
            stream_slot << "##" << level_name;

            ImGui::Text("%s", level_name.c_str());
            ImGui::SameLine(200);
            ImGui::PushItemWidth(100);
            if (drawComboBox(stream_slot.str(), result->_selected, slots))
            {
               std::cout << "set " << level_name << " to " << result->_selected << std::endl;
            }
         }

         ImGui::End();
      }

      if (ImGui::BeginMainMenuBar())
      {
         if (ImGui::BeginMenu("File", true))
         {
            if (ImGui::MenuItem("Save Configuration"))
            {
               std::cout << "level configuration" << std::endl;
               writeLevelConfiguration(results);
               writeSaveState(results);
            }

            ImGui::EndMenu();
         }
         ImGui::EndMainMenuBar();
      }

      ImGui::Render();
      SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

      SDL_RenderClear(renderer);
      ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
      SDL_RenderPresent(renderer);
   }

   ImGui_ImplSDLRenderer_Shutdown();
   ImGui_ImplSDL2_Shutdown();
   ImGui::DestroyContext();

   SDL_DestroyRenderer(renderer);
   SDL_DestroyWindow(window);
   SDL_Quit();

   return 0;
}
