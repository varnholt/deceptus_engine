#include "playerinfo.h"

PlayerInfo PlayerInfo::sCurrent;


PlayerInfo& PlayerInfo::getCurrent()
{
   return sCurrent;
}
