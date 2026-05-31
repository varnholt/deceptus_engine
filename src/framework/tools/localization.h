#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

/// \brief loads and provides translated strings for the active locale.
///
/// translation files are json objects mapping source text to translated text,
/// stored under data/locale/<locale>.json. when a key is not found the source
/// text is returned unchanged, so the game works without any translation file.
///
/// any string passed to translate() that has no entry (or an empty entry) in
/// the active translation table is recorded internally. call flushMissingKeys()
/// at shutdown to write those strings back into the locale file so translators
/// can fill them in without running the extractor first.
///
/// usage:
/// \code
///   Localization::getInstance().load("de");
///   label.setString(tr("Quit"));
///   // at shutdown:
///   Localization::getInstance().flushMissingKeys();
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
   ///
   /// when no translation is found the source text is returned unchanged and
   /// the key is added to the internal missing-keys set.
   ///
   /// \param source_text english source text used as the lookup key.
   /// \return translated string view, or source_text when no translation is found.
   [[nodiscard]] std::string_view translate(std::string_view source_text) const;

   /// \brief writes all strings seen at runtime but not yet translated back into
   /// the locale file with empty values so translators can fill them in.
   ///
   /// does nothing when no locale is loaded or the locale file path is unknown.
   void flushMissingKeys() const;

private:
   std::unordered_map<std::string, std::string> _translations;
   mutable std::unordered_set<std::string> _missing_keys;
   std::string _locale;
   std::string _locale_path;
};

/// \brief returns the translation of source_text in the active locale.
///
/// mirrors qt's tr() convention: the source text is the key and also the
/// english fallback when no translation file is loaded or the key is missing.
///
/// \param source_text english text to translate.
/// \return translated UTF-8 string, or source_text if no translation is available.
[[nodiscard]] std::string tr(std::string_view source_text);
