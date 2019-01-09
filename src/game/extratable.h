#ifndef EXTRATABLE_H
#define EXTRATABLE_H

#include "extrahealth.h"
#include "extraskill.h"
#include "extrakey.h"

#include <memory>
#include <vector>


class ExtraTable
{
public:
   ExtraTable();

   std::shared_ptr<ExtraHealth> mHealth;
   std::shared_ptr<ExtraKey> mKeys;
   std::shared_ptr<ExtraSkill> mSkills;
};

#endif // EXTRATABLE_H
