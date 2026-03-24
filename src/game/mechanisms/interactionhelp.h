#pragma once

#include <array>
#include <memory>

#include "game/animation/animationpool.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxObject;

/// \brief shows contextual interaction hints when the player enters a trigger area.
class InteractionHelp : public GameMechanism, public GameNode
{
public:
   /// \brief creates an interaction-help mechanism and loads the help font.
   /// \param parent parent node in the scene graph.
   InteractionHelp(GameNode* parent = nullptr);

   /// \brief returns the mechanism registry name.
   /// \return string view containing `InteractionHelp`.
   std::string_view objectName() const override;

   /// \brief draws show or hide animations and current hint ui rows.
   /// \param target render target.
   /// \param normal normal-map render target (unused).
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief updates trigger transitions, animation alpha, and keyboard or controller icon selection.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief initializes trigger bounds, show or hide animations, and help rows from TMX properties.
   /// \param data deserialize context containing TMX object data.
   void deserialize(const GameDeserializeData& data);

   /// \brief returns the interaction trigger rectangle.
   /// \return trigger rectangle in pixels.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

private:
   enum class InteractionType
   {
      Read,
      Examine,
      Use
   };

   /// \brief stores one help row with button icon variants and text.
   struct HelpElement
   {
      std::unique_ptr<sf::Sprite> _button_sprite;
      std::unique_ptr<sf::Text> _text;
      sf::IntRect _button_rect_keyboard;
      sf::IntRect _button_rect_controller;
   };

   sf::FloatRect _rect_px;
   bool _player_intersected_in_last_frame{false};
   bool _active{false};
   std::shared_ptr<Animation> _animation_show;
   std::shared_ptr<Animation> _animation_hide;
   std::optional<float> _button_alpha;
   static constexpr int32_t button_max_count = 2;
   std::shared_ptr<sf::Texture> _button_texture;
   std::vector<HelpElement> _help_elements;
   sf::Font _font;
};
