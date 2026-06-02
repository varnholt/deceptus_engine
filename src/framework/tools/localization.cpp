#include "localization.h"

#include "framework/tools/log.h"

#include "json/json.hpp"

#include <SFML/Graphics.hpp>
#include <fstream>
#include <sstream>

Localization& Localization::getInstance()
{
   static Localization instance;
   return instance;
}

void Localization::load(const std::string& locale)
{
   _locale = locale;
   _locale_path = std::string{"data/locale/"} + locale + ".json";
   _translations.clear();
   _missing_keys.clear();

   std::ifstream file_stream(_locale_path);
   if (!file_stream.is_open())
   {
      Log::Warning() << "localization: could not open " << _locale_path;
      return;
   }

   std::string json_text((std::istreambuf_iterator<char>(file_stream)), std::istreambuf_iterator<char>());

   try
   {
      const auto json = nlohmann::json::parse(json_text);
      for (const auto& [key, value] : json.items())
      {
         _translations[key] = value.get<std::string>();
      }
   }
   catch (const std::exception& exception)
   {
      Log::Error() << "localization: failed to parse " << _locale_path << ": " << exception.what();
      return;
   }

   Log::Info() << "localization: loaded " << _translations.size() << " strings for locale '" << locale << "'";
}

const std::string& Localization::getLocale() const
{
   return _locale;
}

std::string_view Localization::translate(std::string_view source_text) const
{
   const auto last_non_space = source_text.find_last_not_of(' ');
   if (last_non_space != std::string_view::npos)
   {
      source_text = source_text.substr(0, last_non_space + 1);
   }

   const auto found = _translations.find(std::string{source_text});
   if (found != _translations.end() && !found->second.empty())
   {
      return found->second;
   }

   // record untranslated strings so flushMissingKeys() can write them back
   if (!_locale_path.empty() && !source_text.empty())
   {
      const auto insert_result = _missing_keys.insert(std::string{source_text});
#ifdef DEVELOPMENT_MODE
      if (insert_result.second)
      {
         flushMissingKeys();
      }
#endif
   }

   return source_text;
}

void Localization::flushMissingKeys() const
{
   if (_locale_path.empty() || _missing_keys.empty())
   {
      return;
   }

   // load the current file so we preserve existing translations
   nlohmann::json json_data;
   {
      std::ifstream file_stream(_locale_path);
      if (file_stream.is_open())
      {
         try
         {
            json_data = nlohmann::json::parse(std::string{std::istreambuf_iterator<char>(file_stream), std::istreambuf_iterator<char>()});
         }
         catch (const std::exception&)
         {
            json_data = nlohmann::json::object();
         }
      }
      else
      {
         json_data = nlohmann::json::object();
      }
   }

   auto new_count = 0;
   for (const auto& key : _missing_keys)
   {
      if (json_data.count(key) == 0)
      {
         json_data[key] = "";
         new_count++;
      }
   }

   if (new_count == 0)
   {
      return;
   }

   std::ofstream out_stream(_locale_path);
   if (!out_stream.is_open())
   {
      Log::Warning() << "localization: could not write missing keys to " << _locale_path;
      return;
   }

   out_stream << json_data.dump(2) << "\n";
   Log::Info() << "localization: wrote " << new_count << " missing key(s) to " << _locale_path;
   _missing_keys.clear();
}

std::string tr(std::string_view source_text)
{
   return std::string{Localization::getInstance().translate(source_text)};
}

std::string getFontPath()
{
   if (Localization::getInstance().getLocale() == "ja")
   {
      return "data/fonts/mona12.ttf";
   }
   return "data/fonts/deceptum.ttf";
}

const sf::Font& getFont()
{
   static sf::Font font = []
   {
      sf::Font loaded_font;
      loaded_font.openFromFile(getFontPath());
      const_cast<sf::Texture&>(loaded_font.getTexture(12)).setSmooth(false);
      const_cast<sf::Texture&>(loaded_font.getTexture(14)).setSmooth(false);
      return loaded_font;
   }();
   return font;
}
