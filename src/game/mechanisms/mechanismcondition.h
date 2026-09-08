#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

class GameMechanism;

/// \brief evaluates a condition authored as a tmx property, so level data can gate ui and mechanism
///        state on the world instead of a level script.
///
/// a definition is a comma separated list of terms, all of which have to hold. every term reads one
/// piece of state that the game already tracks:
///
///     mechanism:<group_id>/<object_id>   the named mechanism currently offers an interaction
///     item:<item_name>                   the inventory holds that item
///
/// a leading '!' negates a term:
///
///     mechanism:extras/handle            while the handle is still lying in the locker
///     !item:handle,item:key              while the handle is gone and the key is carried
///
/// mechanism terms are resolved once after the level has been loaded, so evaluating a condition
/// costs a pointer dereference rather than a lookup by name.
class MechanismCondition
{
public:
   /// \brief parses a condition definition.
   /// \param definition condition as authored in the tmx property.
   /// \return parsed condition, or std::nullopt when the definition is empty or malformed.
   static std::optional<MechanismCondition> parse(const std::string& definition);

   /// \brief resolves all mechanism terms against the mechanisms of the loaded level.
   /// \param all_mechanisms every mechanism the level created.
   void resolveReferences(const std::vector<std::shared_ptr<GameMechanism>>& all_mechanisms);

   /// \brief evaluates all terms.
   /// \return true when every term holds.
   bool isSatisfied() const;

   /// \brief returns the definition this condition was parsed from, for logging.
   /// \return original condition string.
   const std::string& getDefinition() const;

private:
   /// \brief one term of a condition.
   struct Term
   {
      /// \brief state a term reads.
      enum class Source
      {
         Mechanism,  //!< a mechanism offers an interaction
         Item        //!< the inventory holds an item
      };

      Source _source{Source::Mechanism};
      bool _negated{false};
      std::string _group_id;   //!< mechanism group, empty for item terms
      std::string _object_id;  //!< mechanism object id, or the item name for item terms
      std::weak_ptr<GameMechanism> _mechanism;
   };

   /// \brief evaluates one term without applying its negation.
   /// \param term term to evaluate.
   /// \return true when the state the term reads is present.
   static bool isTermPresent(const Term& term);

   std::vector<Term> _terms;
   std::string _definition;
};
