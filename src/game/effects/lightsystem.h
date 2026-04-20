#pragma once

#include <array>
#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

#include <SFML/Graphics.hpp>
#include "box2d/box2d.h"

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "json/json.hpp"

struct TmxObject;

/// \brief manages dynamic light sources, shadow volumes, and deferred light compositing.
class LightSystem
{
public:
   /// \brief callback to render geometry that occludes lights into the stencil buffer.
   using OccluderDrawCallback = std::function<void(sf::RenderTarget& target)>;
   /// \brief represents one light source instance loaded from level data.
   struct LightInstance : public GameNode
   {
      LightInstance(GameNode* parent) : GameNode(parent)
      {
         setClassName(typeid(LightInstance).name());
      }

      b2Vec2 _pos_m = b2Vec2{0.0f, 0.0f};            //!< world position of the light in box2d meters
      b2Vec2 _center_offset_m = b2Vec2{0.0f, 0.0f};  //!< offset from the light's box2d position to the center of its sprite
      sf::Vector2i _center_offset_px;                //!< pixel offset used for sprite positioning

      sf::Color _color = {255, 255, 255, 80};

      bool _enabled = true;

      std::shared_ptr<sf::Texture> _texture;
      std::unique_ptr<sf::Sprite> _sprite;

      int32_t _width_px = 256;
      int32_t _height_px = 256;

      /// \brief physics bodies excluded from shadow casting for this light (e.g. rope chain
      ///        elements whose positions would produce degenerate or unwanted shadow quads).
      std::unordered_set<b2Body*> _excluded_bodies;

      /// \brief repositions the sprite so it remains centered on the light's world position.
      void updateSpritePosition() const;

      /// \brief populates color, dimensions, and center offset from a json node.
      /// \param node json object containing any of: color_r, color_g, color_b, color_a,
      ///             width_px, height_px, center_offset_x_px, center_offset_y_px.
      void deserialize(const nlohmann::json& node);
   };

   std::vector<std::shared_ptr<LightInstance>> _lights;
   sf::Shader _light_shader;

   /// \brief increases all ambient light channels by the same amount.
   /// \param amount value added to each ambient rgba channel.
   void increaseAmbient(float amount);

   /// \brief decreases all ambient light channels by the same amount.
   /// \param amount value subtracted from each ambient rgba channel.
   void decreaseAmbient(float amount);

   /// \brief sets the ambient light color directly.
   /// \param color rgba color (0–255 per channel).
   void setAmbient(sf::Color color);

   /// \brief precomputes helper geometry and loads the fragment shader used for lighting.
   LightSystem();

   /// \brief creates a light instance from deserialized tmx object data and light properties.
   /// \param parent parent game node that owns the created light node.
   /// \param data deserialization payload containing tmx geometry and custom properties.
   /// \return newly created light instance with texture, color, falloff, and transform initialized.
   static std::shared_ptr<LightSystem::LightInstance> createLightInstance(GameNode* parent, const GameDeserializeData& data);

   /// \brief creates a light instance initialized from a json config node.
   /// \param parent parent game node that owns the created light node.
   /// \param node json object with any of: texture, color_r/g/b/a, width_px, height_px, center_offset_x/y_px.
   /// \return newly created light instance with default smooth texture overridden by json values.
   static std::shared_ptr<LightSystem::LightInstance> createLightInstance(GameNode* parent, const nlohmann::json& node);

   /// \brief renders per-light sprites with stencil-clipped shadow volumes into a light map target.
   /// \param target render target.
   /// \param states render states passed by caller and currently ignored.
   void draw(sf::RenderTarget& target1, sf::RenderTarget& target2, sf::RenderStates states);

   /// \brief renders light sprites to both textures then composites with shader.
   /// lights 0-3 render to light_map RGBA, lights 4-7 render to light_map2 RGBA.
   /// \param target render target.
   /// \param color_map scene color buffer.
   /// \param light_map first light accumulation buffer (lights 0-3).
   /// \param light_map2 second light accumulation buffer (lights 4-7).
   /// \param normal_map normal buffer used for per-pixel lighting.
   void draw(
      sf::RenderTarget& target,
      const std::shared_ptr<sf::RenderTexture>& color_map,
      const std::shared_ptr<sf::RenderTexture>& light_map,
      const std::shared_ptr<sf::RenderTexture>& light_map2,
      const std::shared_ptr<sf::RenderTexture>& normal_map
   );

   /// \brief draws debug visualization for active lights.
   /// \param target render target.
   void drawDebug(sf::RenderTarget& target);

   /// \brief sets the callback used to render occluder geometry to the stencil buffer.
   /// \param callback function that draws level geometry (e.g. z=24 layer) to stencil.
   void setOccluderCallback(OccluderDrawCallback callback);

private:
   /// \brief renders shadow extrusion triangles for geometry that should occlude a given light.
   /// \param target render target.
   /// \param light active light for which occluder shadows are generated.
   /// \param candidates pre-filtered list of shadow-casting bodies built once per frame.
   void drawShadowQuads(sf::RenderTarget& target, std::shared_ptr<LightInstance> light, const std::vector<b2Body*>& candidates) const;

   /// \brief renders level occluder geometry to the stencil buffer before shadow/light passes.
   /// \param target render target with active stencil context.
   void drawOccluders(sf::RenderTarget& target) const;

   /// \brief refreshes shader uniforms for active lights, ambient color, and target resolution.
   /// \param target render target.
   void updateLightShader(sf::RenderTarget& target);

   mutable std::vector<std::shared_ptr<LightInstance>> _active_lights;

   // cached texture pointers to avoid redundant setUniform calls each frame
   const sf::Texture* _last_color_map{nullptr};
   const sf::Texture* _last_light_map{nullptr};
   const sf::Texture* _last_light_map2{nullptr};
   const sf::Texture* _last_normal_map{nullptr};

   std::array<float, 4> _ambient_color = {1.0f, 1.0f, 1.0f, 1.0f};
   static constexpr auto segment_count = 20;
   std::array<b2Vec2, segment_count> _unit_circle;

   OccluderDrawCallback _occluder_callback;
};
