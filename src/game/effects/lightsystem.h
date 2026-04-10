#pragma once

#include <array>
#include <memory>
#include <vector>

#include "box2d/box2d.h"
#include <SFML/Graphics.hpp>

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"

struct TmxObject;

/// \brief manages dynamic light sources, shadow volumes, and deferred light compositing.
class LightSystem
{
public:
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
      
      // falloff values are loaded from tiled but not currently used by the shader
      // the sprite texture gradient provides distance falloff instead
      std::array<float, 3> _falloff = {0.4f, 3.0f, 20.0f};

      bool _enabled = true;

      std::shared_ptr<sf::Texture> _texture;
      std::unique_ptr<sf::Sprite> _sprite;

      int32_t _width_px = 256;
      int32_t _height_px = 256;

      /// \brief repositions the sprite so it remains centered on the light's world position.
      void updateSpritePosition() const;
   };

   std::vector<std::shared_ptr<LightInstance>> _lights;
   sf::Shader _light_shader;

   /// \brief increases all ambient light channels by the same amount.
   /// \param amount value added to each ambient rgba channel.
   void increaseAmbient(float amount);

   /// \brief decreases all ambient light channels by the same amount.
   /// \param amount value subtracted from each ambient rgba channel.
   void decreaseAmbient(float amount);

   /// \brief precomputes helper geometry and loads the fragment shader used for lighting.
   LightSystem();

   /// \brief creates a light instance from deserialized tmx object data and light properties.
   /// \param parent parent game node that owns the created light node.
   /// \param data deserialization payload containing tmx geometry and custom properties.
   /// \return newly created light instance with texture, color, falloff, and transform initialized.
   static std::shared_ptr<LightSystem::LightInstance> createLightInstance(GameNode* parent, const GameDeserializeData& data);

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

private:
   /// \brief renders shadow extrusion triangles for geometry that should occlude a given light.
   /// \param target render target.
   /// \param light active light for which occluder shadows are generated.
   void drawShadowQuads(sf::RenderTarget& target, std::shared_ptr<LightInstance> light) const;

   /// \brief refreshes shader uniforms for active lights, ambient color, and target resolution.
   /// \param target render target.
   void updateLightShader(sf::RenderTarget& target);

   mutable std::vector<std::shared_ptr<LightInstance>> _active_lights;

   std::array<float, 4> _ambient_color = {1.0f, 1.0f, 1.0f, 1.0f};
   static constexpr auto segment_count = 20;
   std::array<b2Vec2, segment_count> _unit_circle;
};
