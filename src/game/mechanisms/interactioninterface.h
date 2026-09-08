#pragma once

/// \brief abstract interface for mechanisms the player can act on, so prompts and conditions can ask
///        whether an interaction is available without depending on the concrete mechanism.
class InteractionInterface
{
public:
   virtual ~InteractionInterface() = default;

   /// \brief checks whether a player standing at this mechanism could act on it right now.
   ///
   /// this is the state a mechanism already checks before it reacts to the action button.
   ///
   /// \return true when the interaction is currently available.
   virtual bool isInteractionAvailable() const = 0;
};
