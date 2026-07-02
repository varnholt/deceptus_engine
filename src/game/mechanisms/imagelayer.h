#pragma once

#include "game/io/lazytexture.h"
#include "game/layers/parallaxsettings.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <optional>

struct TmxElement;

/// \brief renders a TMX image layer with lazy texture streaming and optional parallax.
class ImageLayer : public GameMechanism, public GameNode
{
public:
   /// \brief creates an image layer mechanism.
   /// \param parent parent node in the scene graph.
   ImageLayer(GameNode* parent = nullptr);

   /// \brief returns the mechanism registry name.
   /// \return string view containing `ImageLayer`.
   std::string_view objectName() const override;

   /// \brief draws the layer sprite and temporarily applies the parallax view when configured.
   /// \param target render target.
   /// \param normal normal-map render target (unused).
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;
   using GameMechanism::draw;

   /// \brief updates lazy texture chunk loading and creates or removes the sprite as needed.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief updates the internal parallax view from the current level view.
   /// \param level_view_x current level view x position in pixels.
   /// \param level_view_y current level view y position in pixels.
   /// \param view_width current view width in pixels.
   /// \param view_height current view height in pixels.
   void updateView(float level_view_x, float level_view_y, float view_width, float view_height);

   /// \brief resets the internal parallax view to a default full-screen viewport.
   /// \param view_width current view width in pixels.
   /// \param view_height current view height in pixels.
   void resetView(float view_width, float view_height);

   /// \brief starts background disk loading of the layer texture before gameplay begins.
   void preload() override;

   /// \brief forwards drain to the underlying lazy texture.
   /// \return true while the texture is still loading or waiting to upload to GPU.
   bool drainTextures();

   /// \brief returns bounds for mechanism queries.
   /// \return `std::nullopt` because image layers do not expose collision bounds.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief creates an image layer from a TMX image-layer element and its properties.
   /// \param element parsed TMX element expected to be an image layer.
   /// \param level_path base path of the current level for resolving image sources.
   /// \return configured image layer instance.
   static std::shared_ptr<ImageLayer> deserialize(const std::shared_ptr<TmxElement>& element, const std::filesystem::path& level_path);

private:
   std::unique_ptr<sf::Sprite> _sprite;
   std::shared_ptr<LazyTexture> _texture;
   sf::BlendMode _blend_mode = sf::BlendAlpha;
   sf::Vector2f _position;
   sf::Color _color;

   sf::View _parallax_view;
   std::optional<ParallaxSettings> _parallax_settings;
   std::vector<std::string> _restrict_to_rooms;
   std::filesystem::path _texture_path;
};
