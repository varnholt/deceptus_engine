#pragma once

namespace LocalizationLoader
{
/// \brief reads GameConfiguration::_language and calls Localization::load() accordingly.
void loadFromConfig();
}  // namespace LocalizationLoader
