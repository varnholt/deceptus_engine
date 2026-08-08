#pragma once

#include "framework/tools/sfmlshader.h"
#include "game/io/gamedeserializedata.h"
#include "game/level/gamenode.h"
#include "game/mechanisms/gamemechanism.h"
#include "game/shaders/postprocessing.h"

#include <SFML/Graphics.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

struct TmxObject;

/// \brief configures a full screen post processing shader from tmx, like ShaderLayer does for quads.
///
/// This mechanism never draws itself. Its pass runs after the level has been composited, so instead
/// of rendering it hands its shader to the post processing stage, which is why its render stage is
/// MechanismRenderStage::PostProcessing.
///
/// Any tmx property whose name starts with "u_" is forwarded to the shader as a uniform of the
/// property's tmx type, so an arbitrary shader can be driven without touching cpp. String values
/// holding comma separated numbers become vec2/vec3/vec4, and string values naming an existing file
/// are loaded as a texture.
///
/// A sized object rectangle acts as a trigger area: the effect is active while the player is inside
/// it. A zero-sized rectangle leaves the mechanism under script control through setMechanismEnabled.
struct PostProcessingMechanism : public GameMechanism, public GameNode
{
   /// \brief creates a post processing mechanism instance.
   /// \param parent owning game node in the scene graph.
   PostProcessingMechanism(GameNode* parent = nullptr);

   /// \brief returns the mechanism type name used by the serialization system.
   /// \return constant string view containing "PostProcessing".
   std::string_view objectName() const override;

   /// \brief accumulates elapsed time and evaluates the trigger area.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt) override;

   /// \brief returns the trigger rectangle of this mechanism.
   /// \return rectangle in pixel space, empty when the mechanism is script driven.
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   /// \brief updates the shader uniforms for a full screen pass over the given texture.
   /// \param texture texture the effect samples from.
   /// \return shader to draw the full screen sprite with, or nullptr when the shader failed to load.
   const sf::Shader* prepare(const sf::Texture& texture);

   /// \brief returns which part of the frame this effect is applied to.
   /// \return configured post processing scope.
   PostProcessing::Scope getScope() const;

   /// \brief creates and configures a post processing mechanism from tmx object properties.
   /// \param parent owning game node in the scene graph.
   /// \param data deserialization data containing shader paths, scope, and uniform properties.
   /// \return configured instance, or nullptr when no usable shader was given.
   static std::shared_ptr<PostProcessingMechanism> deserialize(GameNode* parent, const GameDeserializeData& data);

private:
   //! \brief one uniform parsed from a tmx property, kept in the type it was declared with
   using UniformValue = std::variant<float, int32_t, bool, sf::Glsl::Vec2, sf::Glsl::Vec3, sf::Glsl::Vec4, std::shared_ptr<sf::Texture>>;

   /// \brief pairs a uniform name with the value read from tmx.
   struct Uniform
   {
      std::string _name;    //!< uniform name as declared in the shader
      UniformValue _value;  //!< value forwarded on every frame
   };

   sfcompat::Shader _shader;                                  //!< the configured post processing shader
   std::vector<Uniform> _uniforms;                            //!< uniforms parsed from the "u_" tmx properties
   sf::FloatRect _rect;                                       //!< trigger area, empty when script driven
   bool _has_trigger_area{false};                             //!< whether the rectangle gates the effect
   PostProcessing::Scope _scope{PostProcessing::Scope::All};  //!< part of the frame the effect covers
   float _elapsed_s{0.0f};                                    //!< seconds elapsed, fed to u_time
};
