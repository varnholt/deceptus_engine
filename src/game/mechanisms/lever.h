#pragma once

#include <stdint.h>
#include <functional>

#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;

class Lever : public GameMechanism, public GameNode
{
public:
   using Callback = std::function<void(int32_t)>;

   enum class Type
   {
      TwoState,
      TriState
   };

   enum class State
   {
      Left = -1,
      Middle = 0,
      Right = 1,
   };

   Lever(GameNode* parent = nullptr);
   ~Lever();

   void preload() override;
   void update(const sf::Time& dt) override;
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;
   void setEnabled(bool enabled) override;
   bool isEnabled() const override;
   void toggle() override;
   void serializeState(nlohmann::json& j) override;
   void deserializeState(const nlohmann::json& j) override;

   void addCallback(const Callback& callback);
   void setCallbacks(const std::vector<Callback>& callbacks);
   void setup(const GameDeserializeData& data);
   void updateReceivers();
   const sf::FloatRect& getPixelRect() const;
   const std::vector<std::string>& getTargetIds() const;
   void setHandleAvailable(bool handle_available);

private:
   void updateSprite();
   void updateDirection();
   void updateTargetPositionReached();

   Type _type = Type::TwoState;
   State _target_state = State::Left;
   State _state_previous = State::Left;

   sf::FloatRect _rect;
   std::unique_ptr<sf::Sprite> _sprite;
   std::shared_ptr<sf::Texture> _texture;
   int32_t _offset = 0;
   int32_t _dir = 0;
   float _idle_time_s = 0.0f;

   bool _reached = false;
   bool _reached_previous = false;
   bool _player_at_lever = false;
   bool _handle_available = true;

   std::vector<Callback> _callbacks;
   std::vector<std::string> _target_ids;
   std::optional<std::chrono::high_resolution_clock::time_point> _last_toggle_time;
   std::function<bool(const std::string&)> _handle_callback;
};
