#include "messagebox.h"

#include "audio.h"
#include "displaymode.h"
#include "framework/easings/easings.h"
#include "framework/image/psd.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"
#include "game/gameconfiguration.h"
#include "game/gamecontrollerintegration.h"
#include "gamestate.h"
#include "player/player.h"

#include <math.h>
#include <algorithm>
#include <iostream>

namespace
{
std::string replaceAll(std::string str, const std::string& from, const std::string& to)
{
   size_t start_pos = 0;
   while ((start_pos = str.find(from, start_pos)) != std::string::npos)
   {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();
   }
   return str;
}

static constexpr auto x_offset_left_px = 110;
static constexpr auto x_offset_center_px = 160;
static constexpr auto x_offset_right_px = 270;
static constexpr auto y_offset_top_px = 82;
static constexpr auto y_offset_middle_px = 149;
static constexpr auto y_offset_bottom_px = 216;
static constexpr auto text_margin_x_px = 8;
static constexpr auto textbox_width_px = 324;

static const auto animation_scale_time_show = sf::seconds(0.7f);
static const auto animation_fade_time_show = sf::seconds(0.7f);
static const auto animation_fade_time_hide = sf::seconds(0.5f);
}  // namespace

std::unique_ptr<MessageBox> MessageBox::__active;
MessageBox::LayoutProperties MessageBox::__default_properties;
bool MessageBox::__initialized = false;

std::vector<std::shared_ptr<Layer>> MessageBox::__layer_stack;
std::map<std::string, std::shared_ptr<Layer>> MessageBox::__layers;
std::vector<std::shared_ptr<Layer>> MessageBox::__box_content_layers;
sf::Font MessageBox::__font;
sf::Text MessageBox::__text;
sf::Vector2f MessageBox::__window_position;

MessageBox::MessageBox(
   MessageBox::Type type,
   const std::string& message,
   const MessageBox::MessageBoxCallback& cb,
   const LayoutProperties& properties,
   int32_t buttons
)
    : _type(type), _message(message), _callback(cb), _properties(properties), _buttons(buttons)
{
   initializeLayers();
   initializeControllerCallbacks();
   _show_time = GlobalClock::getInstance().getElapsedTime();

   DisplayMode::getInstance().enqueueSet(Display::Modal);

   Player::getCurrent()->getControls()->setKeysPressed(0);

   __text.setFont(__font);
   __text.setCharacterSize(12);
   __text.setFillColor(_properties._text_color);
   __text.setString("");
}

MessageBox::~MessageBox()
{
   auto& gci = GameControllerIntegration::getInstance();
   if (gci.isControllerConnected())
   {
      gci.getController()->removeButtonPressedCallback(SDL_CONTROLLER_BUTTON_A, _button_callback_a);
      gci.getController()->removeButtonPressedCallback(SDL_CONTROLLER_BUTTON_B, _button_callback_b);
   }

   DisplayMode::getInstance().enqueueUnset(Display::Modal);

   //   if (mPreviousMode == ExecutionMode::Running)
   //   {
   //      GameState::getInstance().enqueueResume();
   //   }
}

void MessageBox::close(MessageBox::Button button)
{
   auto callback = __active->_callback;

   if (__active->_properties._animate_hide_event)
   {
      __active->_closed = true;
      __active->_hide_time = GlobalClock::getInstance().getElapsedTime();
   }
   else
   {
      __active.reset();
   }

   if (callback)
   {
      callback(button);
   }
}

bool MessageBox::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (!__active)
   {
      return false;
   }

   if (__active->_closed)
   {
      return false;
   }

   if (__active->_drawn)
   {
      MessageBox::Button button = MessageBox::Button::Invalid;

      // yay
      if (key == sf::Keyboard::Return)
      {
         Audio::getInstance().playSample({"messagebox_confirm.wav"});

         if (__active->_buttons & static_cast<int32_t>(Button::Yes))
         {
            button = Button::Yes;
         }
         else if (__active->_buttons & static_cast<int32_t>(Button::Ok))
         {
            button = Button::Ok;
         }

         if (__active->_properties._animate_text)
         {
            if (__active->_chars_shown < __active->_message.length())
            {
               __active->_properties._animate_text = false;
               return true;
            }
         }
      }

      // nay
      if (key == sf::Keyboard::Escape)
      {
         Audio::getInstance().playSample({"messagebox_cancel.wav"});

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
         close(button);
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
         Log::Error() << "font load fuckup";
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

      __window_position = __layers["window"]->_sprite->getPosition();

      __box_content_layers.push_back(__layers["yes_xbox_1"]);
      __box_content_layers.push_back(__layers["no_xbox_1"]);
      __box_content_layers.push_back(__layers["yes_pc_1"]);
      __box_content_layers.push_back(__layers["no_pc_1"]);

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
         pos.x = x_offset_left_px;
         break;
      }
      case MessageBoxLocation::TopCenter:
      case MessageBoxLocation::MiddleCenter:
      case MessageBoxLocation::BottomCenter:
      {
         pos.x = x_offset_center_px;
         break;
      }
      case MessageBoxLocation::TopRight:
      case MessageBoxLocation::MiddleRight:
      case MessageBoxLocation::BottomRight:
      {
         pos.x = x_offset_right_px;
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
         pos.y = y_offset_top_px;
         break;
      }

      case MessageBoxLocation::MiddleLeft:
      case MessageBoxLocation::MiddleCenter:
      case MessageBoxLocation::MiddleRight:
      {
         pos.y = y_offset_middle_px;
         break;
      }

      case MessageBoxLocation::BottomLeft:
      case MessageBoxLocation::BottomCenter:
      case MessageBoxLocation::BottomRight:
      {
         pos.y = y_offset_bottom_px;
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
   auto& gci = GameControllerIntegration::getInstance();
   if (gci.isControllerConnected())
   {
      _button_callback_a = []() { keyboardKeyPressed(sf::Keyboard::Return); };
      _button_callback_b = []() { keyboardKeyPressed(sf::Keyboard::Escape); };
      gci.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_A, _button_callback_a);
      gci.getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_B, _button_callback_b);
   }
}

void MessageBox::showAnimation()
{
   if (__active->_closed)
   {
      return;
   }

   auto contents_alpha = 1.0f;
   const auto visible_time = GlobalClock::getInstance().getElapsedTime() - __active->_show_time;
   if (visible_time < animation_scale_time_show)
   {
      contents_alpha = 0.0f;

      const auto t_normalized = visible_time.asSeconds() / animation_scale_time_show.asSeconds();

      const auto scale_x = Easings::easeOutBack<float>(t_normalized);
      const auto scale_y = scale_x;

      const auto scale_offset = (textbox_width_px - textbox_width_px * scale_x) * 0.5f;

      __layers["window"]->_sprite->setColor(sf::Color{255, 255, 255, static_cast<uint8_t>(t_normalized * 255)});
      __layers["window"]->_sprite->setScale(scale_x, scale_y);
      __layers["window"]->_sprite->setPosition(__window_position.x + scale_offset, __window_position.y);
   }
   else
   {
      __layers["window"]->_sprite->setColor(sf::Color{255, 255, 255, 255});
      __layers["window"]->_sprite->setScale(1.0f, 1.0f);
      __layers["window"]->_sprite->setPosition(__window_position);

      if (visible_time < animation_scale_time_show + animation_fade_time_show)
      {
         const auto t_normalized =
            (visible_time.asSeconds() - animation_scale_time_show.asSeconds()) / animation_fade_time_show.asSeconds();
         contents_alpha = t_normalized;
      }
   }

   // fade in text and buttons
   const auto alpha = static_cast<uint8_t>(contents_alpha * 255);
   const auto color = sf::Color{255, 255, 255, alpha};
   auto text_color = __active->_properties._text_color;
   text_color.a = alpha;

   for (const auto& layer : __box_content_layers)
   {
      layer->_sprite->setColor(color);
   }

   __text.setFillColor(text_color);
}

void MessageBox::hideAnimation()
{
   if (!__active->_closed)
   {
      return;
   }

   const auto elapsed_time = GlobalClock::getInstance().getElapsedTime() - __active->_hide_time;

   const auto t_normalized = elapsed_time.asSeconds() / animation_fade_time_hide.asSeconds();
   const auto contents_alpha = 1.0f - t_normalized;

   if (contents_alpha < 0.0f)
   {
      __active->_reset_instance = true;
   }
   else
   {
      const auto alpha = static_cast<uint8_t>(contents_alpha * 255);
      const auto color = sf::Color{255, 255, 255, alpha};
      auto text_color = __active->_properties._text_color;
      text_color.a = alpha;

      __layers["window"]->_sprite->setColor(color);
      for (const auto& layer : __box_content_layers)
      {
         layer->_sprite->setColor(color);
      }

      __text.setFillColor(text_color);
   }
}

void MessageBox::animateText()
{
   static const std::array<float, 5> text_speeds = {0.5f, 0.75f, 1.0f, 1.5f, 2.0f};

   auto x =
      (GlobalClock::getInstance().getElapsedTime().asSeconds() - __active->_show_time.asSeconds() -
       (__active->_properties._animate_show_event ? animation_scale_time_show.asSeconds() : 0.0f));

   x *= __active->_properties._animate_text_speed;
   x *= text_speeds[GameConfiguration::getInstance()._text_speed];

   // if the thing is animated we want to wait for the animation_scale_time to pass
   // so x might go into negative for that duration.
   x = std::max(0.0f, x);

   auto to = std::min(static_cast<uint32_t>(x), static_cast<uint32_t>(__active->_message.size()));

   if (__active->_chars_shown != to)
   {
      __active->_chars_shown = to;
      __text.setString(__active->_message.substr(0, to));
   }
}

void MessageBox::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   if (!__active)
   {
      return;
   }

   __active->_drawn = true;

   const auto xbox = (GameControllerIntegration::getInstance().isControllerConnected());
   const auto buttons = __active->_buttons;

   // background layer is unused for now
   // bool menu_shown = (DisplayMode::getInstance().isSet(Display::MainMenu));
   // __layers["temp_bg"]->_visible = menu_shown;
   __layers["temp_bg"]->_visible = false;

   // init button layers
   __layers["yes_xbox_1"]->_visible = xbox && buttons & static_cast<int32_t>(Button::Yes);
   __layers["no_xbox_1"]->_visible = xbox && buttons & static_cast<int32_t>(Button::No);
   __layers["yes_pc_1"]->_visible = !xbox && buttons & static_cast<int32_t>(Button::Yes);
   __layers["no_pc_1"]->_visible = !xbox && buttons & static_cast<int32_t>(Button::No);

   if (__active->_properties._animate_show_event)
   {
      showAnimation();
   }

   if (__active->_properties._animate_hide_event)
   {
      hideAnimation();
   }

   // set up an ortho view with screen dimensions
   sf::View pixelOrtho(sf::FloatRect(
      0.0f,
      0.0f,
      static_cast<float>(GameConfiguration::getInstance()._view_width),
      static_cast<float>(GameConfiguration::getInstance()._view_height)
   ));

   window.setView(pixelOrtho);

   for (auto& layer : __layer_stack)
   {
      if (layer->_visible)
      {
         layer->draw(window, states);
      }
   }

   __active->_message = replaceAll(__active->_message, "[br]", "\n");

   if (__active->_properties._animate_text)
   {
      animateText();
   }
   else
   {
      __text.setString(__active->_message);
   }

   // text alignment
   const auto pos = __active->_properties._pos.value_or(pixelLocation(__active->_properties._location));
   auto x = 0;
   if (__active->_properties._centered)
   {
      const auto rect = __text.getGlobalBounds();
      const auto left = pos.x;
      x = static_cast<int32_t>(left + (textbox_width_px - rect.width) * 0.5f);
   }
   else
   {
      x = pos.x + text_margin_x_px;
   }

   __text.setPosition(static_cast<float>(x), static_cast<float>(pos.y));

   window.draw(__text, states);

   // not particularly nice to delete the instance from inside the draw call
   if (__active->_reset_instance)
   {
      __active.reset();
   }
}

void MessageBox::messageBox(
   Type type,
   const std::string& message,
   const MessageBoxCallback& callback,
   const LayoutProperties& properties,
   int32_t buttons
)
{
   Audio::getInstance().playSample({"messagebox_open_01.wav"});
   __active = std::make_unique<MessageBox>(type, message, callback, properties, buttons);
}

void MessageBox::info(const std::string& message, const MessageBoxCallback& callback, const LayoutProperties& properties, int32_t buttons)
{
   if (__active)
   {
      return;
   }

   messageBox(MessageBox::Type::Info, message, callback, properties, buttons);
}

void MessageBox::question(
   const std::string& message,
   const MessageBox::MessageBoxCallback& callback,
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
