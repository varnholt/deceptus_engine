#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class GameMechanism;
class InteractionInterface;

/// \brief a condition read from a tmx property, used to show a hint row only in certain situations.
///
/// a condition is a list of terms separated by commas. all of them have to be true. a term is either
///
///     mechanism:<group>/<name>   that mechanism can be used right now
///     item:<name>                the player carries that item
///
/// and a '!' in front of a term inverts it. examples:
///
///     mechanism:extras/handle    while the handle still lies in the locker
///     !item:handle,item:key      while the handle is gone and the key is there
class MechanismCondition
{
public:
   using MechanismsByGroup = std::unordered_map<std::string, std::vector<std::shared_ptr<GameMechanism>>*>;

   /// \brief reads a condition from a tmx property value.
   /// \param definition condition as written in the tmx property.
   /// \return the condition, or std::nullopt when the value is empty or cannot be read.
   static std::optional<MechanismCondition> parse(const std::string& definition);

   /// \brief looks up the mechanisms the condition refers to.
   /// \param mechanisms_by_group all mechanisms of the level, sorted into their groups.
   void resolveReferences(const MechanismsByGroup& mechanisms_by_group);

   /// \brief checks all terms.
   /// \return true when all of them are true.
   bool isSatisfied() const;

private:
   /// \brief one term of a condition.
   struct Term
   {
      /// \brief what a term looks at.
      enum class Source
      {
         Mechanism,  //!< a mechanism can be used
         Item        //!< the player carries an item
      };

      Source _source{Source::Mechanism};
      bool _inverted{false};
      std::string _group;  //!< mechanism group, empty for item terms
      std::string _name;   //!< mechanism name, or the item name for item terms
      std::weak_ptr<InteractionInterface> _mechanism;
   };

   /// \brief checks one term, ignoring its '!'.
   /// \param term term to check.
   /// \return true when the term is true.
   static bool isTermTrue(const Term& term);

   std::vector<Term> _terms;
   std::string _definition;
};
