#pragma once

#include "camerasystemconfiguration.h"
#include "room.h"

#include <SFML/Graphics.hpp>

#include <optional>
#include <memory>

class CameraSystem
{
   public:

      void update(const sf::Time& dt, float viewWidth, float viewHeight);

      float getX() const;
      float getY() const;

      float getFocusZoneX0() const;
      float getFocusZoneX1() const;

      float getPanicLineY0() const;
      float getPanicLineY1() const;

      float getFocusOffset() const;
      float getFocusZoneCenter() const;

      void syncNow();

      static CameraSystem& getCameraSystem();


   private:

      CameraSystem() = default;

      void updateX(const sf::Time& dt);
      void updateY(const sf::Time& dt);

      float _x = 0.0f;
      float _y = 0.0f;

      float _focus_zone_x0 = 0.0f;
      float _focus_zone_x1 = 0.0f;
      float _focus_zone_center = 0.0f;
      float _focus_offset = 0.0f;

      float _panic_line_y0 = 0.0f;
      float _panic_line_y1 = 0.0f;
      bool _panic = false;

      float _view_width = 0.0f;
      float _view_height = 0.0f;

      bool _focus_x_triggered = false;
      bool _focus_y_triggered = false;

      bool _no_y_update_triggered = false;
      sf::Time _y_update_start_time;

      static CameraSystem sInstance;
};

