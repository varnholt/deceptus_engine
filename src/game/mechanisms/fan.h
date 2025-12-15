#pragma once

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>

#include <memory>
#include <optional>
#include <vector>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

class Fan
    : public GameMechanism
    , public GameNode
{
public:
    Fan(GameNode* parent = nullptr);
    std::string_view objectName() const override;

    void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
    void update(const sf::Time& dt) override;
    std::optional<sf::FloatRect> getBoundingBoxPx() override;
    void setEnabled(bool enabled) override;

    const sf::FloatRect& getPixelRect() const;

    static std::shared_ptr<Fan> deserialize(GameNode* parent, const GameDeserializeData& data);

    enum class TileDirection
    {
        Up = 0,
        Right = 8,
        Left = 16,
        Down = 24,
    };

private:
    struct FanInstance
    {
        sf::Vector2i tile_position_px;
        sf::Vector2f direction;
        sf::FloatRect rect;
        b2Body* body = nullptr;
        std::unique_ptr<sf::Sprite> sprite;
        float sprite_offset = 0.0f;

        FanInstance(const std::shared_ptr<sf::Texture>& tex)
        {
            sprite = std::make_unique<sf::Sprite>(*tex);
            sprite->setTexture(*tex);
        }
    };

    void collide();

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
