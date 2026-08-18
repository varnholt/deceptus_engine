#pragma once

#include "game/level/room.h"

#include <SFML/Graphics.hpp>

#include <memory>
#include <optional>

/// \brief updates the gameplay camera position with focus zones, panic lines, and room clamping.
class CameraSystem
{
public:
   /// \brief advances horizontal and vertical camera tracking for one frame.
   /// \param dt elapsed simulation time since the previous update.
   /// \param view_width_px active view width in pixels.
   /// \param view_height_px active view height in pixels.
   void update(const sf::Time& dt, float view_width_px, float view_height_px);

   /// \brief returns the camera's top-left x position for rendering.
   /// \return world x coordinate in pixels after focus-zone centering.
   float getX() const;

   /// \brief returns the camera's top-left y position for rendering.
   /// \return world y coordinate in pixels adjusted by the configured vertical view ratio.
   float getY() const;

   /// \brief returns the left edge of the horizontal focus zone in view space.
   /// \return focus-zone left boundary in pixels.
   float getFocusZoneX0() const;

   /// \brief returns the right edge of the horizontal focus zone in view space.
   /// \return focus-zone right boundary in pixels.
   float getFocusZoneX1() const;

   /// \brief returns the upper panic line used for vertical camera triggering.
   /// \return upper panic-line y coordinate in view space pixels.
   float getPanicLineY0() const;

   /// \brief returns the lower panic line used for vertical camera triggering.
   /// \return lower panic-line y coordinate in view space pixels.
   float getPanicLineY1() const;

   /// \brief returns the current horizontal focus-zone shift caused by facing direction.
   /// \return signed focus offset in pixels.
   float getFocusOffset() const;

   /// \brief returns the center of the shifted horizontal focus zone.
   /// \return focus-zone center in view space pixels.
   float getFocusZoneCenter() const;

   /// \brief returns the last computed horizontal delta from player to camera center.
   /// \return horizontal delta in pixels before velocity scaling.
   float getDx() const;

   /// \brief returns the last computed vertical delta from player to camera center.
   /// \return vertical delta in pixels before velocity scaling.
   float getDy() const;

   /// \brief snaps camera center to the current player position with room-bound correction.
   void syncNow();

   /// \brief snaps camera to an arbitrary world pixel position and locks tracking until unlockCamera is called.
   /// \param x_px target world x in pixels.
   /// \param y_px target world y in pixels.
   void snapTo(float x_px, float y_px);

   /// \brief releases the lock set by snapTo and resumes normal player-tracking behaviour.
   void unlockCamera();

   /// \brief returns the camera center in world pixel coordinates as tracked by snapTo and syncNow.
   /// \return world-space center of the camera view.
   sf::Vector2f getCenterPx() const;

   /// \brief returns the global camera system instance.
   /// \return singleton camera system used by gameplay and rendering.
   static CameraSystem& getInstance();

private:
   /// \brief constructs the singleton with zeroed camera state.
   CameraSystem() = default;

   /// \brief updates horizontal camera tracking, including focus-zone triggers and room locks.
   /// \param delta_time elapsed frame time used for smoothing and interpolation.
   void updateX(const sf::Time& delta_time);

   /// \brief updates vertical camera tracking, panic behavior, and y-axis acceleration.
   /// \param delta_time elapsed frame time used for smoothing and interpolation.
   void updateY(const sf::Time& delta_time);

   float _x_px = 0.0f;
   float _y_px = 0.0f;

   float _dx_px = 0.0f;
   float _dy_px = 0.0f;

   float _focus_zone_x0_px = 0.0f;
   float _focus_zone_x1_px = 0.0f;
   float _focus_zone_center_px = 0.0f;
   float _focus_offset_px = 0.0f;

   float _panic_line_y0_px = 0.0f;
   float _panic_line_y1_px = 0.0f;
   bool _panic = false;

   float _view_width_px = 0.0f;
   float _view_height_px = 0.0f;

   bool _focus_x_triggered = false;
   bool _focus_y_triggered = false;
   bool _locked = false;  //!< when true, update() skips tracking; set by snapTo, cleared by unlockCamera.

   bool _no_y_update_triggered = false;
   sf::Time _y_update_start_time;
};
