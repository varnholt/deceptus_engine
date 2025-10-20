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

    const int tiles_x = static_cast<int>(fan->_pixel_rect.size.x) / PIXELS_PER_TILE;
    const int tiles_y = static_cast<int>(fan->_pixel_rect.size.y) / PIXELS_PER_TILE;

    if (dir_str == "up")
    {
        const auto y_tl = tiles_y - 1;
        for (int x_tl = 0; x_tl < tiles_x; ++x_tl)
        {
            placeTile(fan, data, x_tl, y_tl);
        }
    }
    else if (dir_str == "down")
    {
        const auto y_tl = 0;
        for (int x_tl = 0; x_tl < tiles_x; ++x_tl)
        {
            placeTile(fan, data, x_tl, y_tl);
        }
    }
    else if (dir_str == "left")
    {
        const auto x_tl = tiles_x - 1;
        for (int y_tl = 0; y_tl < tiles_y; ++y_tl)
        {
            placeTile(fan, data, x_tl, y_tl);
        }
    }
    else if (dir_str == "right")
    {
        const auto x_tl = 0;
        for (int y_tl = 0; y_tl < tiles_y; ++y_tl)
        {
            placeTile(fan, data, x_tl, y_tl);
        }
    }
    else
    {
        std::cerr << "Warning: Fan direction '" << dir_str << "' not recognized. No tiles placed.\n";
    }

    return fan;
}

void Fan::placeTile(const std::shared_ptr<Fan>& fan, const GameDeserializeData& data, int x_tl, int y_tl)
{
    FanSection section(fan->_texture);

    section.tile_position_px = {static_cast<int>(fan->_pixel_rect.position.x) + (x_tl * PIXELS_PER_TILE),
                                static_cast<int>(fan->_pixel_rect.position.y) + (y_tl * PIXELS_PER_TILE)};

    section.direction = fan->_direction;
    section.rect = {{static_cast<float>(section.tile_position_px.x), static_cast<float>(section.tile_position_px.y)},
                    {static_cast<float>(PIXELS_PER_TILE), static_cast<float>(PIXELS_PER_TILE)}};

    b2BodyDef body_def;
    body_def.type = b2_staticBody;
    body_def.position = b2Vec2(static_cast<float>(section.tile_position_px.x) * MPP, static_cast<float>(section.tile_position_px.y) * MPP);
    section.body = data._world->CreateBody(&body_def);

    b2PolygonShape shape;
    shape.SetAsBox(0.5F, 0.5F);

    b2FixtureDef fixture_def;
    fixture_def.shape = &shape;
    fixture_def.density = 1.0F;
    fixture_def.isSensor = false;
    section.body->CreateFixture(&fixture_def);

    section.sprite->setPosition({static_cast<float>(section.tile_position_px.x), static_cast<float>(section.tile_position_px.y)});

    fan->_tiles.push_back(std::move(section));
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

    for (auto& section : _tiles)
    {
        section.scroll_offset += dt.asSeconds() * 25.0F * _speed * _lever_lag;
    }

    updateSprite();
    collide();
}

void Fan::updateSprite()
{
    for (auto& section : _tiles)
    {
        int x_offset = static_cast<int>(section.scroll_offset) % 8;
        section.sprite->setTextureRect({{x_offset * PIXELS_PER_TILE, 0}, {PIXELS_PER_TILE, PIXELS_PER_TILE}});
    }
}

void Fan::draw(sf::RenderTarget& color, sf::RenderTarget&)
{
    for (const auto& section : _tiles)
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
        Player::getCurrent()->getBody()->ApplyForceToCenter(b2Vec2(2.0F * _direction.x, -_direction.y), true);
    }
}
