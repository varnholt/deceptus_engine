#include "localization.h"

#include "framework/tools/log.h"

#include "json/json.hpp"

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
   _translations.clear();

   const auto path = std::string{"data/locale/"} + locale + ".json";
   std::ifstream file_stream(path);
   if (!file_stream.is_open())
   {
      Log::Warning() << "localization: could not open " << path;
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
      Log::Error() << "localization: failed to parse " << path << ": " << exception.what();
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
   const auto found = _translations.find(std::string{source_text});
   if (found != _translations.end() && !found->second.empty())
   {
      return found->second;
   }
   return source_text;
}

std::string tr(std::string_view source_text)
{
   return std::string{Localization::getInstance().translate(source_text)};
}
