#pragma once

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>

#include <memory>
#include <optional>
#include <vector>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

/// \brief models a directional fan that animates vent tiles and pushes the player with wind.
class Fan
    : public GameMechanism
    , public GameNode
{
public:
     /// \brief creates a fan mechanism.
     /// \param parent parent node in the scene graph.
     Fan(GameNode* parent = nullptr);
     /// \brief returns the mechanism registry name.
     /// \return string view containing `Fan`.
     std::string_view objectName() const override;

     /// \brief draws all fan tile sprites.
     /// \param color color render target.
     /// \param normal normal-map render target (unused).
     void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
     /// \brief updates tile animation, power ramping, and wind collision with the player.
     /// \param dt elapsed frame time.
     void update(const sf::Time& dt) override;
     /// \brief returns the full fan area used for wind checks.
     /// \return fan rectangle in pixels.
     std::optional<sf::FloatRect> getBoundingBoxPx() override;
     /// \brief enables or disables the fan and resets startup or shutdown ramping.
     /// \param enabled true to enable airflow, false to begin ramping down.
     void setEnabled(bool enabled) override;

     /// \brief returns the configured fan rectangle.
     /// \return fan rectangle in pixel coordinates.
     const sf::FloatRect& getPixelRect() const;

     /// \brief builds a fan from TMX size and direction settings and creates per-tile instances.
     /// \param parent parent node in the scene graph.
     /// \param data deserialize context containing TMX object and world.
     /// \return configured fan instance.
     static std::shared_ptr<Fan> deserialize(GameNode* parent, const GameDeserializeData& data);

    enum class TileDirection
    {
        Up = 0,
        Right = 8,
        Left = 16,
        Down = 24,
    };

private:
    /// \brief stores one fan tile with sprite, bounds, and collision body.
    struct FanInstance
    {
        sf::Vector2i tile_position_px;
        sf::Vector2f direction;
        sf::FloatRect rect;
        b2Body* body = nullptr;
        std::unique_ptr<sf::Sprite> sprite;
        float sprite_offset = 0.0f;

        /// \brief creates a fan tile sprite from a shared texture atlas.
        /// \param tex shared texture used for this tile sprite.
        FanInstance(const std::shared_ptr<sf::Texture>& tex)
        {
            sprite = std::make_unique<sf::Sprite>(*tex);
            sprite->setTexture(*tex);
        }
    };

    /// \brief applies wind force to the player while the player intersects the fan area.
    void collide();

    /// \brief creates one fan tile sprite and static collision body and appends it to the fan.
    /// \param fan fan instance that receives the created tile section.
    /// \param data deserialize context with world access.
    /// \param x_tl tile x index inside the fan rectangle.
    /// \param y_tl tile y index inside the fan rectangle.
    static void insertInstance(const std::shared_ptr<Fan>& fan, const GameDeserializeData& data, int32_t x_tl, int32_t y_tl);

    std::vector<FanInstance> _instances;
    sf::Vector2f _direction;
    std::string _direction_string;
    TileDirection _direction_enum{TileDirection::Up};
    sf::FloatRect _pixel_rect;
    float _speed = 1.0f;
    float _lever_lag = 1.0f;
    int32_t _y_offset_tl{0};

    std::shared_ptr<sf::Texture> _texture;
};
