#pragma once

#include <SFML/Graphics.hpp>
#include <cstdint>

#include "constants.h"

/// \brief manages manual camera look offsets from keyboard and controller input.
class CameraPanorama
{
public:
   /// \brief returns the global camera panorama controller.
   /// \return singleton instance used by the gameplay camera.
   static CameraPanorama& getInstance();

   /// \brief updates the panorama offset and applies snap-back when no look input is active.
   void update();

   /// \brief sets or clears a specific look-state bit.
   /// \param look look direction or state flag to modify.
   /// \param enable true to set the flag, false to clear it.
   void updateLookState(Look look, bool enable);

   /// \brief replaces the current look offset with a new target vector.
   /// \param desired desired panorama offset in pixels.
   void updateLookVector(const sf::Vector2f& desired);

   /// \brief handles key-press input that starts or steers camera panorama mode.
   /// \param event pressed-key event from SFML input dispatch.
   void processKeyPressedEvents(const sf::Event::KeyPressed* event);

   /// \brief handles key-release input that stops or unsets panorama directions.
   /// \param event released-key event from SFML input dispatch.
   void processKeyReleasedEvents(const sf::Event::KeyReleased* event);

   /// \brief checks whether keyboard panorama mode is currently active.
   /// \return true when the active look-state flag is set.
   bool isKeyboardLookActive() const;

   /// \brief returns the current camera panorama offset.
   /// \return look vector in pixel coordinates.
   const sf::Vector2f& getLookVector() const;

private:
   /// \brief constructs the singleton with inactive look state and zero offset.
   CameraPanorama() = default;

   // clang-format off
   /// \brief computes the next panorama vector from directional input and movement limits.
   /// \param looking_up true when input requests upward look movement.
   /// \param looking_down true when input requests downward look movement.
   /// \param looking_left true when input requests left look movement.
   /// \param looking_right true when input requests right look movement.
   /// \param can_look_up true when upward movement is not blocked by room bounds.
   /// \param can_look_down true when downward movement is not blocked by room bounds.
   /// \param can_look_left true when left movement is not blocked by room bounds.
   /// \param can_look_right true when right movement is not blocked by room bounds.
   /// \param x_relative horizontal input intensity after dead-zone removal.
   /// \param y_relative vertical input intensity after dead-zone removal.
   /// \return desired look vector in pixels, optionally clamped to the configured max distance.
   sf::Vector2f computeDesiredLookVector(
      bool looking_up,
      bool looking_down,
      bool looking_left,
      bool looking_right,
      bool can_look_up,
      bool can_look_down,
      bool can_look_left,
      bool can_look_right,
      float x_relative,
      float y_relative
   ) const;
   // clang-format on

   int32_t _look_state = static_cast<int32_t>(Look::Inactive);
   sf::Vector2f _look_vector;
};
