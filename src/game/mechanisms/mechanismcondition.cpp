#include "mechanismcondition.h"

#include "framework/tools/log.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"
#include "game/state/savestate.h"

#include <algorithm>
#include <cctype>

namespace
{
constexpr auto mechanism_prefix = std::string_view{"mechanism:"};
constexpr auto item_prefix = std::string_view{"item:"};

/// \brief removes leading and trailing whitespace.
/// \param value string to trim.
/// \return trimmed copy of the given string.
std::string trim(const std::string& value)
{
   const auto is_whitespace = [](unsigned char character) { return std::isspace(character) != 0; };

   const auto first = std::ranges::find_if_not(value, is_whitespace);
   const auto last = std::find_if_not(value.rbegin(), value.rend(), is_whitespace).base();

   if (first >= last)
   {
      return {};
   }

   return std::string{first, last};
}

/// \brief splits a condition definition into its comma separated terms.
/// \param definition condition definition to split.
/// \return trimmed terms without the empty ones.
std::vector<std::string> splitTerms(const std::string& definition)
{
   std::vector<std::string> terms;

   auto term_start = std::string::size_type{0};
   while (term_start <= definition.size())
   {
      const auto separator_position = definition.find(',', term_start);
      const auto term_end = (separator_position == std::string::npos) ? definition.size() : separator_position;
      const auto term = trim(definition.substr(term_start, term_end - term_start));

      if (!term.empty())
      {
         terms.push_back(term);
      }

      if (separator_position == std::string::npos)
      {
         break;
      }

      term_start = separator_position + 1;
   }

   return terms;
}
}  // namespace

std::optional<MechanismCondition> MechanismCondition::parse(const std::string& definition)
{
   const auto terms = splitTerms(definition);
   if (terms.empty())
   {
      return std::nullopt;
   }

   MechanismCondition condition;
   condition._definition = definition;

   for (const auto& term_definition : terms)
   {
      Term term;

      auto remainder = std::string_view{term_definition};
      if (remainder.starts_with('!'))
      {
         term._negated = true;
         remainder.remove_prefix(1);
      }

      if (remainder.starts_with(mechanism_prefix))
      {
         remainder.remove_prefix(mechanism_prefix.size());

         const auto separator_position = remainder.find('/');
         if (separator_position == std::string_view::npos)
         {
            Log::Error() << "condition '" << definition << "' expects 'mechanism:<group_id>/<object_id>'";
            return std::nullopt;
         }

         term._source = Term::Source::Mechanism;
         term._group_id = std::string{remainder.substr(0, separator_position)};
         term._object_id = std::string{remainder.substr(separator_position + 1)};

         if (term._group_id.empty() || term._object_id.empty())
         {
            Log::Error() << "condition '" << definition << "' has an empty group or object id";
            return std::nullopt;
         }
      }
      else if (remainder.starts_with(item_prefix))
      {
         remainder.remove_prefix(item_prefix.size());

         term._source = Term::Source::Item;
         term._object_id = std::string{remainder};

         if (term._object_id.empty())
         {
            Log::Error() << "condition '" << definition << "' has an empty item name";
            return std::nullopt;
         }
      }
      else
      {
         Log::Error() << "condition '" << definition << "' has a term without a known prefix: '" << term_definition << "'";
         return std::nullopt;
      }

      condition._terms.push_back(std::move(term));
   }

   return condition;
}

void MechanismCondition::resolveReferences(const std::vector<std::shared_ptr<GameMechanism>>& all_mechanisms)
{
   for (auto& term : _terms)
   {
      if (term._source != Term::Source::Mechanism)
      {
         continue;
      }

      const auto referenced_mechanism = std::ranges::find_if(
         all_mechanisms,
         [&term](const auto& mechanism)
         {
            if (mechanism->getGroupId() != term._group_id)
            {
               return false;
            }

            const auto* game_node = dynamic_cast<const GameNode*>(mechanism.get());
            return (game_node != nullptr) && (game_node->getObjectId() == term._object_id);
         }
      );

      if (referenced_mechanism == all_mechanisms.end())
      {
         Log::Error() << "condition '" << _definition << "' references '" << term._group_id << "/" << term._object_id
                      << "' which does not exist";
         continue;
      }

      term._mechanism = *referenced_mechanism;
   }
}

bool MechanismCondition::isSatisfied() const
{
   return std::ranges::all_of(_terms, [](const auto& term) { return isTermPresent(term) != term._negated; });
}

const std::string& MechanismCondition::getDefinition() const
{
   return _definition;
}

bool MechanismCondition::isTermPresent(const Term& term)
{
   switch (term._source)
   {
      case Term::Source::Mechanism:
      {
         const auto referenced_mechanism = term._mechanism.lock();

         // an unresolved reference has been reported when the level was loaded, it just never holds
         if (!referenced_mechanism)
         {
            return false;
         }

         return referenced_mechanism->isInteractionAvailable();
      }
      case Term::Source::Item:
      {
         return SaveState::getPlayerInfo()._inventory.has(term._object_id);
      }
   }

   return false;
}
