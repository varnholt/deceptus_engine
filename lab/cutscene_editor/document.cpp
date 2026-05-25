#include "document.h"

#include <fstream>

#include <nlohmann/json.hpp>

bool Document::load(const std::string& path)
{
   std::ifstream input_file(path);
   if (!input_file.is_open())
   {
      return false;
   }

   nlohmann::json json_data;
   try
   {
      input_file >> json_data;
   }
   catch (...)
   {
      return false;
   }

   _entries.clear();

   for (const auto& json_entry : json_data)
   {
      CutsceneEntry entry;

      if (json_entry.contains("at"))
      {
         entry._trigger_type = TriggerType::At;
         entry._at_time      = json_entry["at"].get<float>();
      }
      else if (json_entry.contains("on"))
      {
         entry._trigger_type = TriggerType::On;
         entry._on_event     = json_entry["on"].get<std::string>();
         if (json_entry.contains("delay"))
         {
            entry._delay = json_entry["delay"].get<float>();
         }
      }

      if (!json_entry.contains("action"))
      {
         continue;
      }

      entry._action_type = stringToActionType(json_entry["action"].get<std::string>());

      auto get_str = [&](const char* key, const std::string& default_value = {}) -> std::string
      {
         return json_entry.contains(key) ? json_entry[key].get<std::string>() : default_value;
      };
      auto get_float_val = [&](const char* key, float default_value = 0.0f) -> float
      {
         return json_entry.contains(key) ? json_entry[key].get<float>() : default_value;
      };
      auto get_int_val = [&](const char* key, int32_t default_value = 0) -> int32_t
      {
         return json_entry.contains(key) ? json_entry[key].get<int32_t>() : default_value;
      };
      auto get_bool_val = [&](const char* key, bool default_value = false) -> bool
      {
         return json_entry.contains(key) ? json_entry[key].get<bool>() : default_value;
      };

      entry._x              = get_float_val("x");
      entry._y              = get_float_val("y");
      entry._name           = get_str("name");
      entry._animation      = get_str("animation");
      entry._animation_file = get_str("animation_file", "data/sprites/animations.json");
      entry._looped         = get_bool_val("looped");
      entry._visible        = get_bool_val("visible", true);
      entry._speed          = get_float_val("speed", 80.0f);
      entry._event          = get_str("event");
      entry._factor         = get_float_val("factor", 1.0f);
      entry._duration_s     = get_float_val("duration_s", 1.0f);
      entry._easing         = get_str("easing", "linear");
      entry._file           = get_str("file");
      entry._transition     = get_str("transition", "crossfade");
      entry._duration_ms    = get_int_val("duration_ms", 1000);
      entry._post_action    = get_str("post_action", "none");
      entry._id             = get_str("id");

      _entries.push_back(entry);
   }

   _filepath = path;
   _modified = false;
   return true;
}

bool Document::save(const std::string& path) const
{
   nlohmann::json json_array = nlohmann::json::array();

   for (const auto& entry : _entries)
   {
      nlohmann::json json_entry;

      if (entry._trigger_type == TriggerType::At)
      {
         json_entry["at"] = entry._at_time;
      }
      else
      {
         json_entry["on"] = entry._on_event;
         if (entry._delay > 0.0f)
         {
            json_entry["delay"] = entry._delay;
         }
      }

      json_entry["action"] = actionTypeToString(entry._action_type);

      switch (entry._action_type)
      {
         case ActionType::SetCameraPosition:
         {
            json_entry["x"] = entry._x;
            json_entry["y"] = entry._y;
            break;
         }
         case ActionType::MoveCamera:
         {
            json_entry["x"]          = entry._x;
            json_entry["y"]          = entry._y;
            json_entry["duration_s"] = entry._duration_s;
            json_entry["easing"]     = entry._easing;
            if (!entry._event.empty())
            {
               json_entry["event"] = entry._event;
            }
            break;
         }
         case ActionType::SetZoom:
         {
            json_entry["factor"] = entry._factor;
            break;
         }
         case ActionType::PlayMusic:
         {
            json_entry["file"]        = entry._file;
            json_entry["transition"]  = entry._transition;
            json_entry["duration_ms"] = entry._duration_ms;
            json_entry["post_action"] = entry._post_action;
            break;
         }
         case ActionType::PlaySound:
         {
            json_entry["id"] = entry._id;
            break;
         }
         case ActionType::CreateSprite:
         {
            json_entry["name"]           = entry._name;
            json_entry["animation_file"] = entry._animation_file;
            json_entry["animation"]      = entry._animation;
            json_entry["x"]              = entry._x;
            json_entry["y"]              = entry._y;
            json_entry["looped"]         = entry._looped;
            break;
         }
         case ActionType::DestroySprite:
         {
            json_entry["name"] = entry._name;
            break;
         }
         case ActionType::SetSpriteAnimation:
         {
            json_entry["name"]      = entry._name;
            json_entry["animation"] = entry._animation;
            json_entry["looped"]    = entry._looped;
            break;
         }
         case ActionType::SetSpriteVisible:
         {
            json_entry["name"]    = entry._name;
            json_entry["visible"] = entry._visible;
            break;
         }
         case ActionType::MoveSprite:
         {
            json_entry["name"]  = entry._name;
            json_entry["x"]     = entry._x;
            json_entry["y"]     = entry._y;
            json_entry["speed"] = entry._speed;
            if (!entry._event.empty())
            {
               json_entry["event"] = entry._event;
            }
            break;
         }
         case ActionType::FadeIn:
         case ActionType::FadeOut:
         {
            json_entry["speed"] = entry._speed;
            break;
         }
         case ActionType::ShowDialogue:
         {
            json_entry["id"] = entry._id;
            break;
         }
         case ActionType::SetPlayerVisible:
         case ActionType::SetInfoLayerVisible:
         {
            json_entry["visible"] = entry._visible;
            break;
         }
         case ActionType::UnlockCamera:
         case ActionType::NextLevel:
         {
            break;
         }
      }

      json_array.push_back(json_entry);
   }

   std::ofstream out_file(path);
   if (!out_file.is_open())
   {
      return false;
   }
   out_file << json_array.dump(3);
   return true;
}

void Document::reset()
{
   _entries.clear();
   _filepath.clear();
   _modified = false;
}
