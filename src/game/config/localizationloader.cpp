#include "localizationloader.h"

#include "framework/tools/localization.h"
#include "game/config/gameconfiguration.h"

void LocalizationLoader::loadFromConfig()
{
   Localization::getInstance().load(GameConfiguration::getInstance()._language);
}
