#include "messagebox.h"

#include "displaymode.h"
#include "framework/image/psd.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "game/gameconfiguration.h"
#include "game/gamecontrollerintegration.h"
#include "gamestate.h"
#include "player/player.h"

#include <algorithm>
#include <iostream>
#include <math.h>

namespace
{
   std::string replaceAll(std::string str, const std::string& from, const std::string& to)
   {
      size_t start_pos = 0;
      while((start_pos = str.find(from, start_pos)) != std::string::npos)
      {
         str.replace(start_pos, from.length(), to);
         start_pos += to.length();
      }
      return str;
   }
}


std::unique_ptr<MessageBox> MessageBox::__active;
MessageBox::LayoutProperties MessageBox::__default_properties;
bool MessageBox::__initialized = false;

std::vector<std::shared_ptr<Layer>> MessageBox::__layer_stack;
std::map<std::string, std::shared_ptr<Layer>> MessageBox::__layers;
sf::Font MessageBox::__font;
sf::Text MessageBox::__text;


MessageBox::MessageBox(
   MessageBox::Type type,
   const std::string& message,
   MessageBox::MessageBoxCallback cb,
   const LayoutProperties& properties,
   int32_t buttons
)
 : _type(type),
   _message(message),
   _callback(cb),
   _properties(properties),
   _buttons(buttons)
{
   initializeLayers();
   initializeControllerCallbacks();
   _show_time = GlobalClock::getInstance()->getElapsedTime();

   DisplayMode::getInstance().enqueueSet(Display::Modal);

   Player::getCurrent()->getControls().setKeysPressed(0);

   //sText.setScale(0.25f, 0.25f);
   __text.setFont(__font);
   __text.setCharacterSize(12);
   __text.setFillColor(_properties._text_color);
   __text.setString("");

   //   mPreviousMode = GameState::getInstance().getMode();
   //   if (mPreviousMode == ExecutionMode::Running)
   //   {
   //      GameState::getInstance().enqueuePause();
   //   }
}


MessageBox::~MessageBox()
{
   auto gci = GameControllerIntegration::getInstance(0);
   if (gci)
   {
      gci->getController()->removeButtonPressedCallback(SDL_CONTROLLER_BUTTON_A, _button_callback_a);
      gci->getController()->removeButtonPressedCallback(SDL_CONTROLLER_BUTTON_B, _button_callback_b);
   }

   DisplayMode::getInstance().enqueueUnset(Display::Modal);

   //   if (mPreviousMode == ExecutionMode::Running)
   //   {
   //      GameState::getInstance().enqueueResume();
   //   }
}


bool MessageBox::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (!__active)
   {
      return false;
   }

   if (__active->_drawn)
   {
      MessageBox::Button button = MessageBox::Button::Invalid;

      // yay
      if (key == sf::Keyboard::Return)
      {
         if (__active->_buttons & static_cast<int32_t>(Button::Yes))
         {
            button = Button::Yes;
         }
         else if (__active->_buttons & static_cast<int32_t>(Button::Ok))
         {
            button = Button::Ok;
         }

         if (__active->_properties._animate)
         {
            if (__active->_chars_shown < __active->_message.length())
            {
               __active->_properties._animate = false;
               return true;
            }
         }
      }

      // nay
      if (key == sf::Keyboard::Escape)
      {
         if (__active->_buttons & static_cast<int32_t>(Button::No))
         {
            button = Button::No;
         }
         else if (__active->_buttons & static_cast<int32_t>(Button::Cancel))
         {
            button = Button::Cancel;
         }
      }

      // call callback and close message box
      if (button != MessageBox::Button::Invalid)
      {
         auto callback = __active->_callback;
         __active.reset();

         if (callback)
         {
            callback(button);
         }
      }
   }

   return true;
}


void MessageBox::initializeLayers()
{
   if (!__initialized)
   {
      PSD psd;
      psd.setColorFormat(PSD::ColorFormat::ABGR);
      psd.load("data/game/messagebox.psd");

      if (!__font.loadFromFile("data/fonts/deceptum.ttf"))
      {
         std::cerr << "font load fuckup" << std::endl;
      }

      const_cast<sf::Texture&>(__font.getTexture(12)).setSmooth(false);

      // load layers
      for (const auto& layer : psd.getLayers())
      {
         // skip groups
         if (layer.getSectionDivider() != PSD::Layer::SectionDivider::None)
         {
            continue;
         }

         auto tmp = std::make_shared<Layer>();

         auto texture = std::make_shared<sf::Texture>();
         auto sprite = std::make_shared<sf::Sprite>();

         texture->create(static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight()));
         texture->update(reinterpret_cast<const sf::Uint8*>(layer.getImage().getData().data()));

         sprite->setTexture(*texture, true);
         sprite->setPosition(static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop()));

         tmp->_texture = texture;
         tmp->_sprite = sprite;

         __layer_stack.push_back(tmp);
         __layers[layer.getName()] = tmp;
      }

      __initialized = true;
   }
}


sf::Vector2i MessageBox::pixelLocation(MessageBoxLocation location)
{
   sf::Vector2i pos;

   switch (location)
   {
      case MessageBoxLocation::TopLeft:
      case MessageBoxLocation::MiddleLeft:
      case MessageBoxLocation::BottomLeft:
      {
         pos.x = 110;
         break;
      }
      case MessageBoxLocation::TopCenter:
      case MessageBoxLocation::MiddleCenter:
      case MessageBoxLocation::BottomCenter:
      {
         pos.x = 225;
         break;
      }
      case MessageBoxLocation::TopRight:
      case MessageBoxLocation::MiddleRight:
      case MessageBoxLocation::BottomRight:
      {
         pos.x = 270;
         break;
      }
      case MessageBoxLocation::Invalid:
      {
         break;
      }
   }

   switch (location)
   {
      case MessageBoxLocation::TopLeft:
      case MessageBoxLocation::TopCenter:
      case MessageBoxLocation::TopRight:
      {
         pos.y = 82;
         break;
      }

      case MessageBoxLocation::MiddleLeft:
      case MessageBoxLocation::MiddleCenter:
      case MessageBoxLocation::MiddleRight:
      {
         pos.y = 149;
         break;
      }

      case MessageBoxLocation::BottomLeft:
      case MessageBoxLocation::BottomCenter:
      case MessageBoxLocation::BottomRight:
      {
         pos.y = 216;
         break;
      }
      case MessageBoxLocation::Invalid:
      {
         break;
      }
   }

   return pos;
}


void MessageBox::initializeControllerCallbacks()
{
   auto gci = GameControllerIntegration::getInstance(0);
   if (gci)
   {
      _button_callback_a = [](){keyboardKeyPressed(sf::Keyboard::Return);};
      _button_callback_b = [](){keyboardKeyPressed(sf::Keyboard::Escape);};
      gci->getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_A, _button_callback_a);
      gci->getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_B, _button_callback_b);
   }
}


void MessageBox::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   if (!__active)
   {
      return;
   }

   __active->_drawn = true;

   const auto xbox = (GameControllerIntegration::getInstance(0) != nullptr);
   const auto buttons = __active->_buttons;
   bool menu_shown = (DisplayMode::getInstance().isSet(Display::MainMenu));

   __layers["msg-copyssYN"]->_visible = false;
   __layers["msg-overwritessYN"]->_visible = false;
   __layers["msg-deletessYN"]->_visible = false;
   __layers["msg-defaultsYN"]->_visible = false;
   __layers["msg-quitYN"]->_visible = false;
   __layers["temp_bg"]->_visible = menu_shown;
   __layers["yes_xbox_1"]->_visible = xbox && buttons & static_cast<int32_t>(Button::Yes);
   __layers["no_xbox_1"]->_visible = xbox && buttons & static_cast<int32_t>(Button::No);
   __layers["yes_pc_1"]->_visible = !xbox && buttons & static_cast<int32_t>(Button::Yes);
   __layers["no_pc_1"]->_visible = !xbox && buttons & static_cast<int32_t>(Button::No);
   __layers["temp_bg"]->_visible = false;

   // set up an ortho view with screen dimensions
   sf::View pixelOrtho(
      sf::FloatRect(
         0.0f,
         0.0f,
         static_cast<float>(GameConfiguration::getInstance()._view_width),
         static_cast<float>(GameConfiguration::getInstance()._view_height)
      )
   );

   window.setView(pixelOrtho);

   for (auto& layer : __layer_stack)
   {
      if (layer->_visible)
      {
         layer->draw(window, states);
      }
   }

   __active->_message = replaceAll(__active->_message, "[br]", "\n");

   if (__active->_properties._animate)
   {
      auto x = (GlobalClock::getInstance()->getElapsedTime().asSeconds() - __active->_show_time.asSeconds()) * 10.0f;
      auto to = std::min(static_cast<uint32_t>(x), static_cast<uint32_t>(__active->_message.size()));
      if (__active->_chars_shown != to)
      {
         __active->_chars_shown = to;
         __text.setString(__active->_message.substr(0, to));
      }
   }
   else
   {
      __text.setString(__active->_message);
   }

   // text alignment
   const auto pos = pixelLocation(__active->_properties._location);
   auto x = 0;
   if (__active->_properties._centered)
   {
      // box top/left: 137 x 94
      // box dimensions: 202 x 71
      // box left: 143
      const auto rect = __text.getGlobalBounds();
      const auto left = pos.x;
      x = static_cast<int32_t>(left + (202 - rect.width) * 0.5f);
   }
   else
   {
      x = pos.x;
   }

   __text.setPosition(
      static_cast<float>(x),
      static_cast<float>(pos.y)
   );

   window.draw(__text, states);
}


void MessageBox::messageBox(
   Type type,
   const std::string& message,
   MessageBoxCallback callback,
   const LayoutProperties& properties,
   int32_t buttons
)
{
   __active = std::make_unique<MessageBox>(type, message, callback, properties, buttons);
}


void MessageBox::info(
   const std::string& message,
   MessageBoxCallback callback,
   const LayoutProperties& properties,
   int32_t buttons
)
{
   if (__active)
   {
      return;
   }

   messageBox(MessageBox::Type::Info, message, callback, properties, buttons);
}


void MessageBox::question(
   const std::string& message,
   MessageBox::MessageBoxCallback callback,
   const LayoutProperties& properties,
   int32_t buttons
)
{
   if (__active)
   {
      return;
   }

   messageBox(MessageBox::Type::Info, message, callback, properties, buttons);
}


