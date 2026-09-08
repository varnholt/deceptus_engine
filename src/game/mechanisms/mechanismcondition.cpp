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
   const auto terms = StringUtils::split(definition, ',');
   if (terms.empty())
   {
      return std::nullopt;
   }

   MechanismCondition condition;
   condition._definition = definition;

   for (const auto& term_definition : terms)
   {
      Term term;

      const auto trimmed_term = StringUtils::trim(term_definition);
      auto remainder = std::string_view{trimmed_term};
      if (remainder.starts_with('!'))
      {
         term._inverted = true;
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

         term._source = Term::Source::Mechanism;
         term._group = std::string{remainder.substr(0, separator_position)};
         term._name = std::string{remainder.substr(separator_position + 1)};
      }
      else if (remainder.starts_with(item_prefix))
      {
         remainder.remove_prefix(item_prefix.size());

         term._source = Term::Source::Item;
         term._name = std::string{remainder};
      }
      else
      {
         Log::Error() << "condition '" << definition << "' has a term with an unknown prefix: '" << term_definition << "'";
         return std::nullopt;
      }

      if (term._name.empty())
      {
         Log::Error() << "condition '" << definition << "' has a term without a name: '" << term_definition << "'";
         return std::nullopt;
      }

      condition._terms.push_back(std::move(term));
   }

   return condition;
}

void MechanismCondition::resolveReferences(const MechanismsByGroup& mechanisms_by_group)
{
   for (auto& term : _terms)
   {
      if (term._source != Term::Source::Mechanism)
      {
         continue;
      }

      const auto group = mechanisms_by_group.find(term._group);
      if (group == mechanisms_by_group.end() || group->second == nullptr)
      {
         Log::Error() << "condition '" << _definition << "' refers to unknown group '" << term._group << "'";
         continue;
      }

      const auto mechanism = std::ranges::find_if(
         *group->second,
         [&term](const auto& candidate)
         {
            const auto* game_node = dynamic_cast<const GameNode*>(candidate.get());
            return (game_node != nullptr) && (game_node->getObjectId() == term._name);
         }
      );

      if (mechanism == group->second->end())
      {
         Log::Error() << "condition '" << _definition << "' refers to '" << term._group << "/" << term._name << "' which does not exist";
         continue;
      }

      term._mechanism = std::dynamic_pointer_cast<InteractionInterface>(*mechanism);
      if (term._mechanism.expired())
      {
         Log::Error() << "condition '" << _definition << "' refers to '" << term._group << "/" << term._name
                      << "' which offers no interaction";
      }
   }
}

bool MechanismCondition::isSatisfied() const
{
   return std::ranges::all_of(_terms, [](const auto& term) { return isTermTrue(term) != term._inverted; });
}

bool MechanismCondition::isTermTrue(const Term& term)
{
   switch (term._source)
   {
      case Term::Source::Mechanism:
      {
         const auto mechanism = term._mechanism.lock();

         // a reference that could not be resolved has been reported when the level was loaded
         if (!mechanism)
         {
            return false;
         }

         return mechanism->isInteractionAvailable();
      }
      case Term::Source::Item:
      {
         return SaveState::getPlayerInfo()._inventory.has(term._name);
      }
   }

   return false;
}
