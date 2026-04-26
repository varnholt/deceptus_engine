#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

/// \brief loads and provides translated strings for the active locale.
///
/// translation files are json objects mapping source text to translated text,
/// stored under data/locale/<locale>.json. when a key is not found the source
/// text is returned unchanged, so the game works without any translation file.
///
/// usage:
/// \code
///   Localization::getInstance().load("de");
///   label.setString(std::string{tr("Quit")});
/// \endcode
class Localization
{
public:
   /// \brief returns the global localization singleton.
   [[nodiscard]] static Localization& getInstance();

   /// \brief loads translations from data/locale/<locale>.json.
   /// \param locale locale identifier such as "de" or "fr".
   void load(const std::string& locale);

   /// \brief returns the active locale identifier.
   [[nodiscard]] const std::string& getLocale() const;

   /// \brief looks up source_text in the active translation table.
   /// \param source_text english source text used as the lookup key.
   /// \return translated string view, or source_text when no translation is found.
   [[nodiscard]] std::string_view translate(std::string_view source_text) const;

private:
   std::unordered_map<std::string, std::string> _translations;
   std::string _locale;
};

/// \brief returns the translation of source_text in the active locale.
///
/// mirrors qt's tr() convention: the source text is the key and also the
/// english fallback when no translation file is loaded or the key is missing.
///
/// \param source_text english text to translate.
/// \return translated string, or source_text if no translation is available.
[[nodiscard]] std::string tr(std::string_view source_text);
