#include "controlleroverlay.h"

#include "gameconfiguration.h"
#include "gamecontrollerdata.h"
#include "gamecontrollerintegration.h"
#include "image/psd.h"
#include "joystick/gamecontroller.h"

#include <iostream>


ControllerOverlay::ControllerOverlay()
{
   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/game/controller.psd");

   mTextureSize.x = psd.getWidth();
   mTextureSize.y = psd.getHeight();

   for (const auto& layer : psd.getLayers())
   {
      auto tmp = std::make_shared<Layer>();
      tmp->mVisible = layer.isVisible();

      auto texture = std::make_shared<sf::Texture>();
      auto sprite = std::make_shared<sf::Sprite>();

      texture->create(static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight()));
      texture->update(reinterpret_cast<const sf::Uint8*>(layer.getImage().getData().data()));

      sprite->setTexture(*texture, true);
      sprite->setPosition(static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop()));
      sprite->setColor(sf::Color{255, 255, 255, static_cast<uint8_t>(layer.getOpacity())});

      tmp->mTexture = texture;
      tmp->mSprite = sprite;

      mLayers[layer.getName()] = tmp;
   }
}


void ControllerOverlay::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   auto w = GameConfiguration::getInstance().mViewWidth;
   auto h = GameConfiguration::getInstance().mViewHeight;

   // draw layers
   auto windowView = sf::View(sf::FloatRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h)));
   windowView.move(-w + mTextureSize.x + 10, -h + mTextureSize.y + 10);
   window.setView(windowView);

   auto controller_bg = mLayers["controller_bg"];
   auto analog_l      = mLayers["analog_l"];
   auto analog_r      = mLayers["analog_r"];
   auto button_a      = mLayers["button_a"];
   auto button_x      = mLayers["button_x"];
   auto button_b      = mLayers["button_b"];
   auto button_y      = mLayers["button_y"];
   auto dp_down       = mLayers["dp_down"];
   auto dp_up         = mLayers["dp_up"];
   auto dp_left       = mLayers["dp_left"];
   auto dp_right      = mLayers["dp_right"];
   auto lb            = mLayers["lb"];
   auto rt            = mLayers["rt"];
   auto rb            = mLayers["rb"];
   auto lt            = mLayers["lt"];
   auto view          = mLayers["view"];
   auto menu          = mLayers["menu"];
   auto xbox          = mLayers["xbox"];


   controller_bg->draw(window, states);

   if (GameControllerData::getInstance().isControllerUsed())
   {
      auto controller = GameControllerIntegration::getInstance(0)->getController();
      auto joystickInfo = GameControllerData::getInstance().getJoystickInfo();

      auto pressed = [&](SDL_GameControllerButton button) -> bool {
         auto buttonValues = joystickInfo.getButtonValues();
         auto buttonPressed = buttonValues[static_cast<size_t>(button /*Id*/)];
         return buttonPressed;
      };

      auto axis = [&](SDL_GameControllerAxis axis) -> float {
         auto axisValues = joystickInfo.getAxisValues();
         auto axisId= controller->getAxisIndex(axis);
         return axisValues[static_cast<uint32_t>(axisId)] / 32767.0f;
      };

      auto hat = [&](int32_t hat) -> bool {
         auto buttonValues = joystickInfo.getHatValues();
         auto dpadPressed = buttonValues.empty() ? false : buttonValues.at(0) & hat;
         return dpadPressed;
      };

      // draw analog axes
      static const float analogFactor = 3.0f;
      auto x0 = axis(SDL_CONTROLLER_AXIS_LEFTX);
      auto y0 = axis(SDL_CONTROLLER_AXIS_LEFTY);
      auto x1 = axis(SDL_CONTROLLER_AXIS_RIGHTX);
      auto y1 = axis(SDL_CONTROLLER_AXIS_RIGHTY);
      auto tl = axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT);
      auto tr = axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
      analog_l->mSprite->setOrigin(-x0 * analogFactor, -y0 * analogFactor);
      analog_r->mSprite->setOrigin(-x1 * analogFactor, -y1 * analogFactor);
      analog_l->mSprite->setColor(pressed(SDL_CONTROLLER_BUTTON_LEFTSTICK) ? sf::Color::Red : sf::Color::White);
      analog_r->mSprite->setColor(pressed(SDL_CONTROLLER_BUTTON_RIGHTSTICK) ? sf::Color::Red : sf::Color::White);
      analog_l->draw(window, states);
      analog_r->draw(window, states);
      if (tr > -0.8f) rt->draw(window, states);
      if (tl > -0.8f) lt->draw(window, states);

      // draw buttons
      if (pressed(SDL_CONTROLLER_BUTTON_A)) button_a->draw(window, states);
      if (pressed(SDL_CONTROLLER_BUTTON_X)) button_x->draw(window, states);
      if (pressed(SDL_CONTROLLER_BUTTON_B)) button_b->draw(window, states);
      if (pressed(SDL_CONTROLLER_BUTTON_Y)) button_y->draw(window, states);
      if (pressed(SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) lb->draw(window, states);
      if (pressed(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) rb->draw(window, states);
      if (pressed(SDL_CONTROLLER_BUTTON_BACK)) view->draw(window, states);
      if (pressed(SDL_CONTROLLER_BUTTON_GUIDE)) xbox->draw(window, states);
      if (pressed(SDL_CONTROLLER_BUTTON_START)) menu->draw(window, states);

      // draw dpad
      if (hat(SDL_HAT_UP)) dp_up->draw(window, states);
      if (hat(SDL_HAT_DOWN)) dp_down->draw(window, states);
      if (hat(SDL_HAT_LEFT)) dp_left->draw(window, states);
      if (hat(SDL_HAT_RIGHT)) dp_right->draw(window, states);
   }
}
