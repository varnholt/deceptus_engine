#pragma once

#include "framework/tools/sfmlshader.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

struct PostProcessingMechanism;

/// \brief full screen post processing applied to the composited frame.
///
/// The effect runs on the very last blit, when the window render texture holding the gamma
/// corrected level plus all overlays goes to the window. That places it on top of the gamma
/// shader without needing another render target, at the cost of covering the hud as well.
class PostProcessing
{
public:
   /// \brief identifies the selectable full screen effects.
   enum class Effect
   {
      None,
      GameBoy,
      RgbSplit,
      Glitch
   };

   /// \brief identifies which part of the frame an effect is applied to.
   enum class Scope
   {
      All,   //!< the composited frame, overlays included
      Level  //!< the level only, hud and menus stay untouched on top
   };

   /// \brief retrieves the global post processing singleton.
   /// \return reference to the shared instance.
   static PostProcessing& getInstance();

   /// \brief loads all effect shaders, requires an active opengl context.
   void initialize();

   /// \brief advances the time fed into the animated effects.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt);

   /// \brief selects the active effect.
   /// \param effect effect to activate, Effect::None to disable post processing.
   void setEffect(Effect effect);

   /// \brief retrieves the active effect.
   /// \return currently selected effect.
   Effect getEffect() const;

   /// \brief selects which part of the frame the effect is applied to.
   /// \param scope scope to switch to.
   void setScope(Scope scope);

   /// \brief retrieves the active scope.
   /// \return currently selected scope.
   Scope getScope() const;

   /// \brief stores the post processing mechanism the level currently wants applied.
   ///
   /// Called once per frame from the render loop. Held as a weak reference so a level unload can
   /// never leave a dangling mechanism behind. A console-selected effect overrides it, which keeps
   /// the console usable for testing in levels that configure their own effect.
   /// \param mechanism active mechanism, or nullptr when the level has none.
   void setLevelEffect(const std::shared_ptr<PostProcessingMechanism>& mechanism);

   /// \brief tells whether an effect is selected and its shader is usable.
   /// \return true when a pass should be rendered.
   bool isActive() const;

   /// \brief updates the active effect's uniforms for a full screen pass over the given texture.
   /// \param texture texture the effect samples from.
   /// \return shader to draw the full screen sprite with, or nullptr when no effect is active.
   const sf::Shader* prepare(const sf::Texture& texture);

   /// \brief maps a console friendly effect name to an effect.
   /// \param name effect name such as "gameboy".
   /// \return the matching effect, or nullopt when the name is unknown.
   static std::optional<Effect> effectFromName(const std::string& name);

   /// \brief lists the console friendly names of all selectable effects.
   /// \return effect names in selection order, starting with "none".
   static std::vector<std::string> getEffectNames();

   /// \brief maps a console friendly scope name to a scope.
   /// \param name scope name such as "level".
   /// \return the matching scope, or nullopt when the name is unknown.
   static std::optional<Scope> scopeFromName(const std::string& name);

   /// \brief lists the console friendly names of all scopes.
   /// \return scope names in selection order, starting with "all".
   static std::vector<std::string> getScopeNames();

private:
   PostProcessing() = default;

   /// \brief pairs a selectable effect with the shader implementing it.
   struct EffectShader
   {
      Effect _effect{Effect::None};  //!< effect this shader implements
      std::string _fragment_path;    //!< fragment shader source path
      sfcompat::Shader _shader;      //!< loaded shader
   };

   /// \brief looks up the shader entry belonging to an effect.
   /// \param effect effect to look up.
   /// \return pointer to the entry, or nullptr when the effect has no shader.
   EffectShader* findEffectShader(Effect effect);

   std::vector<EffectShader> _effect_shaders;  //!< one entry per effect that has a shader
   Effect _effect{Effect::None};               //!< console-selected effect, overrides the level effect
   Scope _scope{Scope::All};                   //!< part of the frame the console-selected effect is applied to

   //! \brief effect configured by the level through a post processing mechanism
   std::weak_ptr<PostProcessingMechanism> _level_effect;
   float _elapsed_s{0.0f};    //!< seconds elapsed, fed to the animated effects
   bool _initialized{false};  //!< whether the shaders have been loaded
};
