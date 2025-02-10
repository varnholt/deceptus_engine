#include "controlleroverlay.h"

#include "framework/image/psd.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/log.h"
#include "game/config/gameconfiguration.h"
#include "game/controller/gamecontrollerdata.h"
#include "game/controller/gamecontrollerintegration.h"

#include <iostream>

ControllerOverlay::ControllerOverlay()
{
   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/game/controller.psd");

   _texture_size.x = psd.getWidth();
   _texture_size.y = psd.getHeight();

   for (const auto& layer : psd.getLayers())
   {
      if (!layer.isImageLayer())
      {
         continue;
      }

      auto tmp = std::make_shared<Layer>();
      tmp->_visible = layer.isVisible();

      auto texture = std::make_shared<sf::Texture>();
      auto sprite = std::make_shared<sf::Sprite>();

      if (!texture->create(static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight())))
      {
         Log::Fatal() << "failed to create texture: " << layer.getName();
      }

      texture->update(reinterpret_cast<const sf::Uint8*>(layer.getImage().getData().data()));

      sprite->setTexture(*texture, true);
      sprite->setPosition(static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop()));
      sprite->setColor(sf::Color{255, 255, 255, static_cast<uint8_t>(layer.getOpacity())});

      tmp->_texture = texture;
      tmp->_sprite = sprite;

      _layers[layer.getName()] = tmp;
   }
}

void ControllerOverlay::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto w = GameConfiguration::getInstance()._view_width;
   auto h = GameConfiguration::getInstance()._view_height;

   // draw layers
   auto windowView = sf::View(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   windowView.move(-w + _texture_size.x + 10.0f, -h + _texture_size.y + 10.0f);
   window.setView(windowView);

   auto controller_bg = _layers["controller_bg"];
   auto analog_l = _layers["analog_l"];
   auto analog_r = _layers["analog_r"];
   auto button_a = _layers["button_a"];
   auto button_x = _layers["button_x"];
   auto button_b = _layers["button_b"];
   auto button_y = _layers["button_y"];
   auto dp_down = _layers["dp_down"];
   auto dp_up = _layers["dp_up"];
   auto dp_left = _layers["dp_left"];
   auto dp_right = _layers["dp_right"];
   auto lb = _layers["lb"];
   auto rt = _layers["rt"];
   auto rb = _layers["rb"];
   auto lt = _layers["lt"];
   auto view = _layers["view"];
   auto menu = _layers["menu"];
   auto xbox = _layers["xbox"];

   controller_bg->draw(window, states);

   if (GameControllerData::getInstance().isControllerUsed())
   {
      auto controller = GameControllerIntegration::getInstance().getController();
      auto joystick_info = GameControllerData::getInstance().getJoystickInfo();

      auto pressed = [&](SDL_GamepadButton button) -> bool
      {
         const auto button_values = joystick_info.getButtonValues();
         const auto button_pressed = button_values[static_cast<size_t>(button /*Id*/)];
         return button_pressed;
      };

      auto axis = [&](SDL_GamepadAxis axis) -> float
      {
         const auto axis_values = joystick_info.getAxisValues();
         const auto axis_id = controller->getAxisIndex(axis);
         return axis_values[static_cast<uint32_t>(axis_id)] / 32767.0f;
      };

      auto hat = [&](int32_t hat) -> bool
      {
         const auto button_values = joystick_info.getHatValues();
         const auto dpad_pressed = button_values.empty() ? false : button_values.at(0) & hat;
         return dpad_pressed;
      };

      // draw analog axes
      static const float analogFactor = 3.0f;
      const auto x0 = axis(SDL_GAMEPAD_AXIS_LEFTX);
      const auto y0 = axis(SDL_GAMEPAD_AXIS_LEFTY);
      const auto x1 = axis(SDL_GAMEPAD_AXIS_RIGHTX);
      const auto y1 = axis(SDL_GAMEPAD_AXIS_RIGHTY);
      const auto tl = axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
      const auto tr = axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
      analog_l->_sprite->setOrigin(-x0 * analogFactor, -y0 * analogFactor);
      analog_r->_sprite->setOrigin(-x1 * analogFactor, -y1 * analogFactor);
      analog_l->_sprite->setColor(pressed(SDL_GAMEPAD_BUTTON_LEFT_STICK) ? sf::Color::Red : sf::Color::White);
      analog_r->_sprite->setColor(pressed(SDL_GAMEPAD_BUTTON_RIGHT_STICK) ? sf::Color::Red : sf::Color::White);
      analog_l->draw(window, states);
      analog_r->draw(window, states);
      if (tr > -0.8f)
         rt->draw(window, states);
      if (tl > -0.8f)
         lt->draw(window, states);

      // draw buttons
      if (pressed(SDL_GAMEPAD_BUTTON_SOUTH))
         button_a->draw(window, states);
      if (pressed(SDL_GAMEPAD_BUTTON_WEST))
         button_x->draw(window, states);
      if (pressed(SDL_GAMEPAD_BUTTON_EAST))
         button_b->draw(window, states);
      if (pressed(SDL_GAMEPAD_BUTTON_NORTH))
         button_y->draw(window, states);
      if (pressed(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))
         lb->draw(window, states);
      if (pressed(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))
         rb->draw(window, states);
      if (pressed(SDL_GAMEPAD_BUTTON_BACK))
         view->draw(window, states);
      if (pressed(SDL_GAMEPAD_BUTTON_GUIDE))
         xbox->draw(window, states);
      if (pressed(SDL_GAMEPAD_BUTTON_START))
         menu->draw(window, states);

      // draw dpad
      if (hat(SDL_HAT_UP))
         dp_up->draw(window, states);
      if (hat(SDL_HAT_DOWN))
         dp_down->draw(window, states);
      if (hat(SDL_HAT_LEFT))
         dp_left->draw(window, states);
      if (hat(SDL_HAT_RIGHT))
         dp_right->draw(window, states);
   }
}
