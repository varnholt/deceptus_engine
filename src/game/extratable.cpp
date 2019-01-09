#include "extratable.h"

#include "extrahealth.h"


ExtraTable::ExtraTable()
{
   mHealth = std::make_shared<ExtraHealth>();
   mSkills = std::make_shared<ExtraSkill>();
}
