#include "DestructibleBlockingRect.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

#include "game/audio/audio.h"
#include "game/io/texturepool.h"
#include "game/io/valuereader.h"
#include "game/mechanisms/gamemechanismdeserializerregistry.h"
#include "game/player/player.h"

#include <filesystem>

// Register the mechanism with the deserializer registry so that TMX files
// containing an object group of type "DestructibleBlockingRect" will spawn
// instances of this class and add them to the "destructible_blocking_rects"
// layer.  Both the named layer and the object group are supported to mirror
// conventions used by other mechanisms.
namespace
{
const auto registered_destructible_blocking_rect = []()
{
    auto& registry = GameMechanismDeserializerRegistry::instance();
    registry.mapGroupToLayer("DestructibleBlockingRect", "destructible_blocking_rects");
    registry.registerLayerName(
        "destructible_blocking_rects",
        [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
        {
            auto mechanism = std::make_shared<DestructibleBlockingRect>(parent, data);
            mechanisms["destructible_blocking_rects"]->push_back(mechanism);
        });
    registry.registerObjectGroup(
        "DestructibleBlockingRect",
        [](GameNode* parent, const GameDeserializeData& data, auto& mechanisms)
        {
            auto mechanism = std::make_shared<DestructibleBlockingRect>(parent, data);
            mechanisms["destructible_blocking_rects"]->push_back(mechanism);
        });
    return true;
}();
} // namespace

DestructibleBlockingRect::DestructibleBlockingRect(GameNode* parent, const GameDeserializeData& data)
    : FixtureNode(parent)
    , GameMechanism()
{
    // Basic metadata
    setClassName(typeid(DestructibleBlockingRect).name());
    setType(ObjectTypeObstacle);
    setObjectId(data._tmx_object->_name);

    // Read custom properties if available
    if (data._tmx_object->_properties)
    {
        const auto& map = data._tmx_object->_properties->_map;
        _config.frame_width  = ValueReader::readValue<int32_t>("frame_width",  map).value_or(_config.frame_width);
        _config.frame_height = ValueReader::readValue<int32_t>("frame_height", map).value_or(_config.frame_height);
        _config.frames       = ValueReader::readValue<int32_t>("frames",       map).value_or(_config.frames);
        // row is often specified as 1 or 2 in the editor; convert to zero-based
        auto row_val  = ValueReader::readValue<int32_t>("row",          map).value_or(_config.row);
        // if user specified "row" starting at 1, store zero-based row index
        if (row_val >= 1) {
            _config.row = row_val - 1;
        } else {
            _config.row = row_val;
        }
        _config.max_hits     = ValueReader::readValue<int32_t>("hits",         map).value_or(_config.max_hits);
        _config.hit_sound    = ValueReader::readValue<std::string>("hit_sound",    map).value_or(_config.hit_sound);
        _config.destroy_sound= ValueReader::readValue<std::string>("destroy_sound",map).value_or(_config.destroy_sound);
        _config.texture_path = ValueReader::readValue<std::string>("texture",   map).value_or(_config.texture_path);
        _config.z_index      = ValueReader::readValue<int32_t>("z",        map).value_or(_config.z_index);
    }
    // Initialize state based on config
    _state.hits_left = _config.max_hits;
    _state.current_frame = 0;
    _state.dead = false;

    // Determine pixel position from TMX object; anchor to bottom left
    const auto x_px = data._tmx_object->_x_px;
    const auto y_px = data._tmx_object->_y_px;

    // The physical blocking area is fixed at 48x96 pixels; convert to Box2D meters.
    // Box2D's SetAsBox method expects half-extents.  Compute half widths and
    // heights in metres and keep the full extents for the bounding box.
    constexpr float blocking_width_px  = 48.0f;
    constexpr float blocking_height_px = 96.0f;
    const float half_width_m  = 0.5f * blocking_width_px  * MPP;
    const float half_height_m = 0.5f * blocking_height_px * MPP;

    // Set up bounding box for chunking/culling
    _rect_px = sf::FloatRect{ static_cast<float>(x_px), static_cast<float>(y_px), blocking_width_px, blocking_height_px };
    addChunks(_rect_px);

    // Create Box2D body; position at the TMX object's coordinate, scaled to meters
    b2BodyDef body_def;
    body_def.type = b2_staticBody;
    body_def.position = MPP * b2Vec2{static_cast<float>(x_px), static_cast<float>(y_px)};
    _body = data._world->CreateBody(&body_def);

    // Define a rectangle shape; Box2D boxes are defined by half-extents and an offset.
    // Provide the half-width and half-height computed above and shift the shape by
    // the same amount so the rectangle's bottom-left corner matches the TMX
    // object's position.
    _shape.SetAsBox(half_width_m, half_height_m, b2Vec2(half_width_m, half_height_m), 0.0f);
    b2FixtureDef fixture_def;
    fixture_def.shape = &_shape;
    fixture_def.density = 1.0f;
    fixture_def.friction = 0.0f;
    fixture_def.restitution = 0.0f;
    auto* fixture = _body->CreateFixture(&fixture_def);
    // set user data so we can identify this object during collisions
    fixture->SetUserData(static_cast<void*>(this));

    // Load texture and optional normal map.  Use the base path provided by the
    // deserialize data to resolve relative paths.
    // If the texture path is relative (starts without slash) we append it to the
    // base path.  Otherwise we treat it as absolute.
    const auto resolved_texture_path = data._base_path / _config.texture_path;
    _texture = TexturePool::getInstance().get(resolved_texture_path);
    // Attempt to load a normal map by inserting "_normals" before the file
    // extension.  Skip if the file does not exist.
    const auto stem = resolved_texture_path.stem().string();
    const auto ext  = resolved_texture_path.extension().string();
    std::filesystem::path normal_map_path = resolved_texture_path.parent_path() / (stem + "_normals" + ext);
    if (std::filesystem::exists(normal_map_path))
    {
        _normal_map = TexturePool::getInstance().get(normal_map_path);
    }

    // Set up sprite anchored to the bottom left of the blocking area.  Since
    // sprites may be larger than the blocking area (for overhanging graphics),
    // shift the sprite up by its frame height.
    _sprite = std::make_unique<sf::Sprite>(*_texture);
    _sprite->setPosition(sf::Vector2f{ static_cast<float>(x_px), static_cast<float>(y_px) - static_cast<float>(_config.frame_height) });
    _sprite->setTextureRect(sf::IntRect{ 0, _config.row * _config.frame_height, _config.frame_width, _config.frame_height });

    // Preload audio samples; if strings are empty the call is ignored
    if (!_config.hit_sound.empty())
    {
        Audio::getInstance().addSample(_config.hit_sound);
    }
    if (!_config.destroy_sound.empty())
    {
        Audio::getInstance().addSample(_config.destroy_sound);
    }

    // z-index for draw order; store in config and set on mechanism
    setZ(_config.z_index);
}

std::optional<sf::FloatRect> DestructibleBlockingRect::getBoundingBoxPx()
{
    return _state.dead ? std::nullopt : std::optional<sf::FloatRect>(_rect_px);
}

void DestructibleBlockingRect::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
    if (_state.dead || !_sprite)
    {
        return;
    }
    // Draw the current frame to the color map
    color.draw(*_sprite);
    // If a normal map exists, draw with the normal map to the normal target
    if (_normal_map)
    {
        auto old_texture = _sprite->getTexture();
        _sprite->setTexture(*_normal_map);
        normal.draw(*_sprite);
        _sprite->setTexture(*old_texture);
    }
}

void DestructibleBlockingRect::update(const sf::Time& /*dt*/)
{
    // No continuous update required; frames advance only on hits.
}

void DestructibleBlockingRect::beginContact(b2Contact* /*contact*/, FixtureNode* other)
{
    if (_state.dead || !other)
    {
        return;
    }
    // Trigger hits only when the player's weapon sensors collide with the block.
    // We treat the left and right arm sensors as melee attacks.  Other sensors
    // such as foot or head sensors are ignored so the player can stand on the
    // block without damaging it.  Feel free to extend this check if your
    // engine provides explicit weapon object types.
    const auto other_type = other->getType();
    if (other_type == ObjectTypePlayerLeftArmSensor || other_type == ObjectTypePlayerRightArmSensor)
    {
        onHit(1);
    }
}

void DestructibleBlockingRect::endContact(FixtureNode* /*other*/)
{
    // Not used; contact end does not affect state
}

void DestructibleBlockingRect::onHit(int damage)
{
    // Trigger a hit for each point of damage.  This allows future
    // extensions where different weapons deal more than one damage.  We
    // stop processing if the mechanism becomes dead.
    if (_state.dead)
    {
        return;
    }
    for (int i = 0; i < damage; ++i)
    {
        hit();
        if (_state.dead)
        {
            break;
        }
    }
}

void DestructibleBlockingRect::hit()
{
    if (_state.dead)
    {
        return;
    }
    // decrement remaining hits
    --_state.hits_left;

    // Advance the animation frame up to the maximum number of configured frames
    ++_state.current_frame;
    if (_state.current_frame >= _config.frames)
    {
        _state.current_frame = _config.frames - 1;
    }
    // Update sprite rect accordingly
    if (_sprite)
    {
        _sprite->setTextureRect(sf::IntRect{ _state.current_frame * _config.frame_width,
                                             _config.row * _config.frame_height,
                                             _config.frame_width,
                                             _config.frame_height });
    }
    // Play hit sound if configured
    if (!_config.hit_sound.empty())
    {
        Audio::getInstance().playSample({ _config.hit_sound });
    }
    // When no hits remain, collapse the block
    if (_state.hits_left <= 0)
    {
        destroy();
    }
}

void DestructibleBlockingRect::destroy()
{
    if (_state.dead)
    {
        return;
    }
    _state.dead = true;
    // Play destruction sound if configured
    if (!_config.destroy_sound.empty())
    {
        Audio::getInstance().playSample({ _config.destroy_sound });
    }
    // Disable the physics body; this stops it from blocking the player
    if (_body)
    {
        _body->SetEnabled(false);
    }
    // Also disable the mechanism so it is no longer updated or drawn in chunk
    setEnabled(false);
}