#pragma once

#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"

#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include <optional>

struct TmxObject;

/// \brief draws a textured screen-space quad processed by configurable shaders.
struct ShaderLayer : public GameMechanism, public GameNode
{
   /// \brief creates a shader layer instance.
   /// \param parent owning game node in the scene graph.
   ShaderLayer(GameNode* parent = nullptr);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "ShaderLayer".
   std::string_view objectName() const override;

   /// \brief draws the quad with shader uniforms and optional uv parameters.
   /// \param target render target.
   /// \param normal normal-map render target, unused by this mechanism.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal) override;

   /// \brief accumulates elapsed time for time-driven shader uniforms.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the rectangle covered by the shader layer.
   /// \return layer bounds in pixel space.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief caches uniform locations for all known shader uniforms.
   virtual void checkUniforms();

   /// \brief called after base deserialization so subclasses can read their own TMX properties.
   /// \param data deserialization data passed through from the factory.
   virtual void readCustomProperties(const GameDeserializeData& data)
   {
   }

   std::unique_ptr<sf::Shader> _shader;
   sf::Vector2f _position;
   sf::Vector2f _size;
   sf::FloatRect _rect;
   std::shared_ptr<sf::Texture> _texture;
   float _time_offset = 0.0f;
   float _uv_width = 1.0f;
   float _uv_height = 1.0f;
   sf::Time _elapsed;

   std::optional<sf::Shader::UniformLocation> _u_texture_loc;
   std::optional<sf::Shader::UniformLocation> _u_time_loc;
   std::optional<sf::Shader::UniformLocation> _u_resolution_loc;
   std::optional<sf::Shader::UniformLocation> _u_uv_height_loc;

   /// \brief creates and configures a shader layer from tmx object properties.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialization data containing shader, texture, and uv settings.
   /// \return configured shader layer instance or nullptr on invalid input.
   static std::shared_ptr<ShaderLayer> deserialize(GameNode* parent, const GameDeserializeData& data);

   // customization factory
   using FactoryFunction = std::shared_ptr<ShaderLayer>(GameNode* parent);

   /// \brief registers a custom shader layer factory by customization id.
   /// \param id customization identifier read from tmx properties.
   /// \param factory_function factory callback creating a specialized shader layer.
   static void registerCustomization(const std::string& id, const std::function<FactoryFunction>&);
};
