#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief trigger volume that emits a "pressed" event when the player presses a configured button inside it.
class ButtonRect : public GameMechanism, public GameNode
{
public:
   /// \brief creates an empty button trigger.
   /// \param parent parent node in the scene graph.
   ButtonRect(GameNode* parent = nullptr);

   /// \brief returns the mechanism type identifier.
   /// \return non-owning string view with value "ButtonRect".
   std::string_view objectName() const override;

   enum class Action
   {
      Enable,
      Disable,
      Toggle,
   };

   enum class Button
   {
      A,
      B,
      X,
      Y
   };

   /// \brief checks player overlap and emits an observer event when the configured input is pressed.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the trigger rectangle in pixel coordinates.
   /// \return rectangle used for overlap checks.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief initializes trigger bounds and the required button from tmx properties.
   /// \param data deserialize context with tmx object data.
   void setup(const GameDeserializeData& data);

   /// \brief resolves references to other mechanisms.
   /// \param mechanisms mechanism list available in the level.
   void findReference(const std::vector<std::shared_ptr<GameMechanism>>& mechanisms);


private:
   /// \brief executes the configured action against referenced targets.
   void processAction();

   sf::FloatRect _rect;
   bool _player_intersects = false;
   Button _button{Button::B};
};
