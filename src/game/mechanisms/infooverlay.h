#ifndef INFOOVERLAY_H
#define INFOOVERLAY_H

#include <SFML/Graphics.hpp>
#include <chrono>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief displays a timed overlay texture with fade-in, hold, and fade-out phases.
class InfoOverlay : public GameMechanism, public GameNode
{
public:
   /// \brief creates an info overlay in disabled state.
   /// \param parent parent node in the scene graph.
   InfoOverlay(GameNode* parent = nullptr);

   /// \brief returns the mechanism registry name.
   /// \return string view containing `InfoOverlay`.
   std::string_view objectName() const override;

   /// \brief advances overlay timing and updates sprite alpha based on the active phase.
   /// \param delta_time elapsed frame time.
   void update(const sf::Time& delta_time) override;

   /// \brief draws the overlay sprite, optionally in a fullscreen orthographic view.
   /// \param color color render target.
   /// \param normal normal-map render target (unused).
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

   /// \brief returns bounds for mechanism queries.
   /// \return `std::nullopt` because this mechanism has no collision bounds.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief creates and configures an info overlay from TMX object properties.
   /// \param parent parent node in the scene graph.
   /// \param data deserialize context with TMX object and properties.
   /// \return configured info overlay instance.
   static std::shared_ptr<InfoOverlay> setup(GameNode* parent, const GameDeserializeData& data);

private:
   using FloatSeconds = std::chrono::duration<float>;

   /// \brief groups timing and display options that control overlay playback.
   struct Settings
   {
      FloatSeconds _start_delay_duration{1.5f};
      FloatSeconds _fade_in_duration{1.5f};
      FloatSeconds _show_duration{3.0f};
      FloatSeconds _fade_out_duration{2.5f};
      bool _show_once{true};
      bool _fullscreen{true};
   };

   sf::FloatRect _rect;
   sf::IntRect _texture_rect;
   std::unique_ptr<sf::Sprite> _sprite;
   std::shared_ptr<sf::Texture> _texture;
   FloatSeconds _elapsed{0.0f};
   Settings _settings;
};

#endif  // INFOOVERLAY_H
