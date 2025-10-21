#include "fan.h"

#include "constants.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"
#include "game/io/texturepool.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/player.h"

#include <iostream>
#include <map>

namespace
{
    const auto registered_fan = [] {
        auto& registry = GameMechanismDeserializerRegistry::instance();
        registry.mapGroupToLayer("Fan", "fans");
        registry.registerObjectGroup("Fan", [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms) {
            auto mechanism = Fan::deserialize(parent, data);
            mechanisms["fans"]->push_back(mechanism);
        });
        return true;
    }();
} // namespace

Fan::Fan(GameNode* parent) : GameNode(parent)
{
    setClassName(typeid(Fan).name());
}

std::string_view Fan::objectName() const
{
    return "Fan";
}

void Fan::setEnabled(bool enabled)
{
    GameMechanism::setEnabled(enabled);
    _lever_lag = enabled ? 0.0F : 1.0F;
}

const sf::FloatRect& Fan::getPixelRect() const
{
    return _pixel_rect;
}

std::optional<sf::FloatRect> Fan::getBoundingBoxPx()
{
    return _pixel_rect;
}

std::shared_ptr<Fan> Fan::deserialize(GameNode* parent, const GameDeserializeData& data)
{
    auto fan = std::make_shared<Fan>(parent);

    const auto& obj = data._tmx_object;
    fan->_texture = TexturePool::getInstance().get("data/sprites/fan.png");

    fan->_pixel_rect = {{obj->_x_px, obj->_y_px}, {obj->_width_px, obj->_height_px}};

    std::string dir_str = "up";
    if (obj->_properties)
    {
        const auto& map = obj->_properties->_map;

        if (auto dir_prop = map.find("direction"); dir_prop != map.end())
        {
            dir_str = dir_prop->second->_value_string.value_or("up");
        }

        if (auto speed_prop = map.find("speed"); speed_prop != map.end())
        {
            fan->_speed = speed_prop->second->_value_float.value_or(1.0F);
        }
    }

    static const std::map<std::string, sf::Vector2f> directions = {{"up", {0.0F, -1.0F}},
                                                                   {"down", {0.0F, 1.0F}},
                                                                   {"left", {-1.0F, 0.0F}},
                                                                   {"right", {1.0F, 0.0F}}};

    fan->_direction = directions.contains(dir_str) ? directions.at(dir_str) : sf::Vector2f{0.0F, -1.0F};

    fan->_direction_string = dir_str; // Store the string form too

    const auto tiles_x = static_cast<int>(fan->_pixel_rect.size.x) / PIXELS_PER_TILE;
    const auto tiles_y = static_cast<int>(fan->_pixel_rect.size.y) / PIXELS_PER_TILE;

    if (dir_str == "up")
    {
        fan->_direction_enum = TileDirection::Up;
        fan->_y_offset_tl = 0;
        const auto y_tl = tiles_y - 1; // bottom of rect
        for (int x_tl = 0; x_tl < tiles_x; ++x_tl)
        {
            insertInstance(fan, data, x_tl, y_tl);
        }
    }
    else if (dir_str == "down")
    {
        fan->_direction_enum = TileDirection::Down;
        fan->_y_offset_tl = 3;
        const auto y_tl = 0; // top of rect
        for (int x_tl = 0; x_tl < tiles_x; ++x_tl)
        {
            insertInstance(fan, data, x_tl, y_tl);
        }
    }
    else if (dir_str == "left")
    {
        fan->_direction_enum = TileDirection::Left;
        fan->_y_offset_tl = 2;
        const auto x_tl = tiles_x - 1; // right of rect
        for (int y_tl = 0; y_tl < tiles_y; ++y_tl)
        {
            insertInstance(fan, data, x_tl, y_tl);
        }
    }
    else if (dir_str == "right")
    {
        fan->_direction_enum = TileDirection::Right;
        fan->_y_offset_tl = 1;
        const auto x_tl = 0; // left of rect
        for (int y_tl = 0; y_tl < tiles_y; ++y_tl)
        {
            insertInstance(fan, data, x_tl, y_tl);
        }
    }
    else
    {
        std::cerr << "Warning: Fan direction '" << dir_str << "' not recognized. No tiles placed.\n";
    }

    return fan;
}

void Fan::insertInstance(const std::shared_ptr<Fan>& fan, const GameDeserializeData& data, int32_t x_tl, int32_t y_tl)
{
    FanInstance instance(fan->_texture);

    instance.sprite_offset = std::rand() % 8;
    instance.tile_position_px = {static_cast<int>(fan->_pixel_rect.position.x) + (x_tl * PIXELS_PER_TILE),
                                 static_cast<int>(fan->_pixel_rect.position.y) + (y_tl * PIXELS_PER_TILE)};

    instance.direction = fan->_direction;
    instance.rect = {{static_cast<float>(instance.tile_position_px.x), static_cast<float>(instance.tile_position_px.y)},
                     {static_cast<float>(PIXELS_PER_TILE), static_cast<float>(PIXELS_PER_TILE)}};

    b2BodyDef body_def;
    body_def.type = b2_staticBody;
    body_def.position = b2Vec2(static_cast<float>(instance.tile_position_px.x) * MPP, static_cast<float>(instance.tile_position_px.y) * MPP);
    instance.body = data._world->CreateBody(&body_def);

    b2PolygonShape shape;

    // a rounded box prevents the player of getting stuck between the gaps
    //
    //      h       g
    //      _________
    //     /         \
    //   a |         | f
    //     |         |
    //   b |         | e
    //     \_________/
    //
    //      c       d

    // clang-format off
    constexpr float w = 0.5f;
    constexpr float e = 0.1f;
    std::array<b2Vec2, 8> rounded_box{
        b2Vec2{0, e},     // a
        b2Vec2{0, w - e}, // b
        b2Vec2{e, w},     // c
        b2Vec2{w - e, w}, // d
        b2Vec2{w, w - e}, // e
        b2Vec2{w, e},     // f
        b2Vec2{w - e, 0}, // g
        b2Vec2{e, 0},     // h
    };
    // clang-format on

    shape.Set(rounded_box.data(), static_cast<int32_t>(rounded_box.size()));

    b2FixtureDef fixture_def;
    fixture_def.shape = &shape;
    fixture_def.density = 1.0F;
    fixture_def.isSensor = false;
    instance.body->CreateFixture(&fixture_def);

    instance.sprite->setPosition({static_cast<float>(instance.tile_position_px.x), static_cast<float>(instance.tile_position_px.y)});

    fan->_instances.push_back(std::move(instance));
}

void Fan::update(const sf::Time& dt)
{
    if (!isEnabled())
    {
        if (_lever_lag <= 0.0F)
        {
            return;
        }
        _lever_lag -= dt.asSeconds();
    }
    else
    {
        _lever_lag = std::min(_lever_lag + dt.asSeconds(), 1.0F);
    }

    for (auto& instance : _instances)
    {
        instance.sprite_offset += dt.asSeconds() * 25.0F * _speed * _lever_lag;
        const auto x_offset = static_cast<int32_t>(instance.sprite_offset) % 8;
        instance.sprite->setTextureRect({{x_offset * PIXELS_PER_TILE, _y_offset_tl * PIXELS_PER_TILE}, {PIXELS_PER_TILE, PIXELS_PER_TILE}});
    }

    collide();
}

void Fan::draw(sf::RenderTarget& color, sf::RenderTarget&)
{
    for (const auto& section : _instances)
    {
        color.draw(*section.sprite);
    }
}

void Fan::collide()
{
    if (!isEnabled())
    {
        return;
    }

    const auto& player_rect = Player::getCurrent()->getPixelRectFloat();
    if (player_rect.findIntersection(_pixel_rect).has_value())
    {
        Player::getCurrent()->getBody()->ApplyForceToCenter(b2Vec2(2.0F * _direction.x, _direction.y), true);
    }
}
