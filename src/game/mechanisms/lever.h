#pragma once

#include <stdint.h>
#include <functional>

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;

/// \brief implements an interactable lever that drives linked mechanisms.
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

   /// \brief creates a lever mechanism.
   /// \param parent parent node in the scene graph.
   Lever(GameNode* parent = nullptr);

   /// \brief unregisters the optional inventory callback used for handle insertion.
   ~Lever();

   /// \brief returns the mechanism registry name.
   /// \return string view containing `Lever`.
   std::string_view objectName() const override;

   /// \brief preloads audio samples for toggling and handle insertion.
   void preload() override;

   /// \brief updates lever animation and handles player button-b interaction near the lever.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief draws the current lever sprite frame.
   /// \param color color render target.
   /// \param normal normal-map render target (unused).
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal) override;

   /// \brief returns the lever interaction rectangle.
   /// \return lever rectangle in pixels.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief sets the target side of the lever and notifies all receivers.
   /// \param enabled true to target the right state, false to target the left state.
   void setEnabled(bool enabled) override;

   /// \brief reports whether the current target state is right.
   /// \return true when the target state is right.
   bool isEnabled() const override;

   /// \brief advances to the next state according to lever type and plays switch audio.
   void toggle() override;

   /// \brief serializes current lever target state when serialization is enabled.
   /// \param j destination json object.
   void serializeState(nlohmann::json& j) override;

   /// \brief restores lever target state from serialized json data and updates receivers.
   /// \param j source json object containing state.
   void deserializeState(const nlohmann::json& j) override;

   /// \brief adds one callback that receives lever state changes.
   /// \param callback callback invoked with lever state as int32_t.
   void addCallback(const Callback& callback);

   /// \brief replaces all registered receiver callbacks.
   /// \param callbacks callback list invoked on state changes.
   void setCallbacks(const std::vector<Callback>& callbacks);

   /// \brief initializes lever settings, sprite setup, and optional handle-from-inventory behavior.
   /// \param data deserialize context with TMX object and properties.
   void setup(const GameDeserializeData& data);

   /// \brief notifies all registered callbacks with the current target state.
   void updateReceivers();

   /// \brief returns the lever rectangle in pixel coordinates.
   /// \return lever rectangle in pixels.
   const sf::FloatRect& getPixelRect() const;

   /// \brief returns configured target mechanism ids.
   /// \return target-id list parsed from TMX properties.
   const std::vector<std::string>& getTargetIds() const;

   /// \brief sets whether this lever currently has an interactable handle.
   /// \param handle_available true when the handle can be used.
   void setHandleAvailable(bool handle_available);

private:
   /// \brief updates the displayed sprite frame from current state and animation values.
   void updateSprite();

   /// \brief updates animation direction so the handle moves toward the current target state.
   void updateDirection();

   /// \brief updates whether the moving handle reached the target sprite offset.
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
