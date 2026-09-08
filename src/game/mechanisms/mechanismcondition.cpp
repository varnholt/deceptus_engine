#include "mechanismcondition.h"

#include "framework/tools/log.h"
#include "framework/tools/stringutils.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"
#include "game/mechanisms/interactioninterface.h"
#include "game/state/savestate.h"

#include <algorithm>

namespace
{
constexpr auto mechanism_prefix = std::string_view{"mechanism:"};
constexpr auto item_prefix = std::string_view{"item:"};
}  // namespace

std::optional<MechanismCondition> MechanismCondition::parse(const std::string& definition)
{
   const auto requirement_definitions = StringUtils::split(definition, ',');
   if (requirement_definitions.empty())
   {
      return std::nullopt;
   }

   MechanismCondition condition;
   condition._definition = definition;

   for (const auto& requirement_definition : requirement_definitions)
   {
      Requirement requirement;

      const auto trimmed_definition = StringUtils::trim(requirement_definition);
      auto remainder = std::string_view{trimmed_definition};
      if (remainder.starts_with('!'))
      {
         requirement._inverted = true;
         remainder.remove_prefix(1);
      }

      if (remainder.starts_with(mechanism_prefix))
      {
         remainder.remove_prefix(mechanism_prefix.size());

         const auto separator_position = remainder.find('/');
         if (separator_position == std::string_view::npos)
         {
            Log::Error() << "condition '" << definition << "' expects 'mechanism:<group>/<name>'";
            return std::nullopt;
         }

         requirement._source = Requirement::Source::Mechanism;
         requirement._group = std::string{remainder.substr(0, separator_position)};
         requirement._name = std::string{remainder.substr(separator_position + 1)};
      }
      else if (remainder.starts_with(item_prefix))
      {
         remainder.remove_prefix(item_prefix.size());

         requirement._source = Requirement::Source::Item;
         requirement._name = std::string{remainder};
      }
      else
      {
         Log::Error() << "condition '" << definition << "' has a requirement with an unknown prefix: '" << requirement_definition << "'";
         return std::nullopt;
      }

      if (requirement._name.empty())
      {
         Log::Error() << "condition '" << definition << "' has a requirement without a name: '" << requirement_definition << "'";
         return std::nullopt;
      }

      condition._requirements.push_back(std::move(requirement));
   }

   return condition;
}

void MechanismCondition::resolveReferences(const MechanismsByGroup& mechanisms_by_group)
{
   for (auto& requirement : _requirements)
   {
      if (requirement._source != Requirement::Source::Mechanism)
      {
         continue;
      }

      const auto group = mechanisms_by_group.find(requirement._group);
      if (group == mechanisms_by_group.end() || group->second == nullptr)
      {
         Log::Error() << "condition '" << _definition << "' refers to unknown group '" << requirement._group << "'";
         continue;
      }

      const auto mechanism = std::ranges::find_if(
         *group->second,
         [&requirement](const auto& candidate)
         {
            const auto* game_node = dynamic_cast<const GameNode*>(candidate.get());
            return (game_node != nullptr) && (game_node->getObjectId() == requirement._name);
         }
      );

      if (mechanism == group->second->end())
      {
         Log::Error() << "condition '" << _definition << "' refers to '" << requirement._group << "/" << requirement._name
                      << "' which does not exist";
         continue;
      }

      requirement._mechanism = std::dynamic_pointer_cast<InteractionInterface>(*mechanism);
      if (requirement._mechanism.expired())
      {
         Log::Error() << "condition '" << _definition << "' refers to '" << requirement._group << "/" << requirement._name
                      << "' which offers no interaction";
      }
   }
}

bool MechanismCondition::isSatisfied() const
{
   return std::ranges::all_of(
      _requirements, [](const auto& requirement) { return isRequirementMet(requirement) != requirement._inverted; }
   );
}

bool MechanismCondition::isRequirementMet(const Requirement& requirement)
{
   switch (requirement._source)
   {
      case Requirement::Source::Mechanism:
      {
         const auto mechanism = requirement._mechanism.lock();

         // a reference that could not be resolved has been reported when the level was loaded
         if (!mechanism)
         {
            return false;
         }

         return mechanism->isInteractionAvailable();
      }
      case Requirement::Source::Item:
      {
         return SaveState::getPlayerInfo()._inventory.has(requirement._name);
      }
   }

   return false;
}
