#include "messagebox.h"

#include "framework/easings/easings.h"
#include "framework/image/layer.h"
#include "framework/image/psd.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/globalclock.h"
#include "framework/tools/log.h"
#include "game/audio/audio.h"
#include "game/config/gameconfiguration.h"
#include "game/controller/gamecontrollerintegration.h"
#include "game/state/displaymode.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace
{
constexpr auto x_offset_left_px = 110.0f;
constexpr auto x_offset_center_px = 160.0f;
constexpr auto x_offset_right_px = 270.0f;
constexpr auto y_offset_top_px = 82.0f;
constexpr auto y_offset_middle_px = 149.0f;
constexpr auto y_offset_bottom_px = 216.0f;
constexpr auto text_margin_x_px = 8.0f;
constexpr auto textbox_width_px = 324.0f;
constexpr auto background_width_px = 318.0f;

static const auto animation_scale_time_show = sf::seconds(0.7f);
static const auto animation_fade_time_show = sf::seconds(0.7f);
static const auto animation_fade_time_hide = sf::seconds(0.5f);

std::unique_ptr<MessageBox> __active;
std::unique_ptr<MessageBox> __previous;

sf::Font __font;

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

sf::Vector2f pixelLocation(MessageBoxLocation location)
{
   sf::Vector2f pos;

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

void close(MessageBox::Button button)
{
   auto callback = __active->_callback;

   if (__active->_properties._animate_hide_event)
   {
      __previous = std::move(__active);
      __previous->_closed = true;
      __previous->_hide_time = GlobalClock::getInstance().getElapsedTime();
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

void messageBox(
   MessageBox::Type type,
   const std::string& message,
   const MessageBox::MessageBoxCallback& callback,
   const MessageBox::LayoutProperties& properties,
   int32_t buttons
)
{
   if (properties._animate_show_event)
   {
      Audio::getInstance().playSample({"messagebox_open_01.wav"});
   }

   __active = std::make_unique<MessageBox>(type, message, callback, properties, buttons);
}

}  // namespace

MessageBox::LayoutProperties MessageBox::__default_properties;

MessageBox::MessageBox(
   MessageBox::Type type,
   const std::string& message,
   const MessageBox::MessageBoxCallback& cb,
   const LayoutProperties& properties,
   int32_t buttons
)
    : _type(type), _callback(cb), _properties(properties), _buttons(buttons)
{
   initializeLayers();
   initializeControllerCallbacks();
   _show_time = GlobalClock::getInstance().getElapsedTime();

   DisplayMode::getInstance().enqueueSet(Display::Modal);

   // text alignment
   const auto pos = pixelLocation(_properties._location) + _properties._pos.value_or(sf::Vector2f{0.0f, 0.0f}) +
                    sf::Vector2f{properties._centered ? 0.0f : text_margin_x_px, 0.0f};

   const auto segments = RichTextParser::parseRichText(
      message,
      __font,
      _properties._text_color,
      properties._centered ? RichTextParser::Alignment::Centered : RichTextParser::Alignment::Left,
      textbox_width_px,
      pos,
      12
   );

   _plain_text = RichTextParser::toString(segments);

   std::transform(
      segments.begin(),
      segments.end(),
      std::back_inserter(_segments),
      [](const RichTextParser::Segment& segment)
      { return TextSegment{*segment.text, segment.text->getFillColor(), segment.text->getString()}; }
   );

   // can maybe be removed
   if (properties._animate_text)
   {
      for (auto& segment : _segments)
      {
         segment.text.setString("");
      }
   }

   showAnimation();
}

MessageBox::~MessageBox()
{
   auto& gci = GameControllerIntegration::getInstance();
   if (gci.isControllerConnected())
   {
      gci.getController()->removeButtonPressedCallback(SDL_GAMEPAD_BUTTON_SOUTH, _button_callback_a);
      gci.getController()->removeButtonPressedCallback(SDL_GAMEPAD_BUTTON_EAST, _button_callback_b);
   }

   DisplayMode::getInstance().enqueueUnset(Display::Modal);
}

void MessageBox::initializeLayers()
{
   static bool __psd_loaded = false;
   static PSD psd;

   if (!__psd_loaded)
   {
      __psd_loaded = true;
      psd.setColorFormat(PSD::ColorFormat::ABGR);
      psd.load("data/game/messagebox.psd");

      if (!__font.openFromFile("data/fonts/deceptum.ttf"))
      {
         Log::Error() << "font load fuckup";
      }
      const_cast<sf::Texture&>(__font.getTexture(12)).setSmooth(false);
   }

   // load layers
   for (const auto& layer : psd.getLayers())
   {
      // skip groups
      if (!layer.isImageLayer())
      {
         continue;
      }

      auto tmp = std::make_shared<Layer>();

      try
      {
         auto texture =
            std::make_shared<sf::Texture>(sf::Vector2u{static_cast<uint32_t>(layer.getWidth()), static_cast<uint32_t>(layer.getHeight())});
         texture->update(reinterpret_cast<const uint8_t*>(layer.getImage().getData().data()));

         auto sprite = std::make_shared<sf::Sprite>(*texture);

         sprite->setPosition({static_cast<float>(layer.getLeft()), static_cast<float>(layer.getTop())});

         tmp->_texture = texture;
         tmp->_sprite = sprite;
      }
      catch (...)
      {
         Log::Fatal() << "failed to create texture: " << layer.getName();
      }

      _layer_stack.push_back(tmp);
      _layers[layer.getName()] = tmp;
   }

   _box_content_layers.push_back(_layers["yes_xbox_1"]);
   _box_content_layers.push_back(_layers["no_xbox_1"]);
   _box_content_layers.push_back(_layers["yes_pc_1"]);
   _box_content_layers.push_back(_layers["no_pc_1"]);
   _box_content_layers.push_back(_layers["next_page"]);

   // background layer is unused for now
   _layers["temp_bg"]->_visible = false;

   // initialize positions
   _window_position_px = _layers["window"]->_sprite->getPosition();
   _background_position_px = _layers["background"]->_sprite->getPosition();
   _next_page_position_px = _layers["next_page"]->_sprite->getPosition();
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

   if (__active->_state != DisplayState::Visible && __active->_state != DisplayState::ShowAnimation)
   {
      return false;
   }

   MessageBox::Button button = MessageBox::Button::Invalid;

   // yay
   if (key == sf::Keyboard::Key::Enter)
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
         if (__active->_char_animate_index < __active->_plain_text.length())
         {
            __active->_properties._animate_text = false;
            return true;
         }
      }
   }

   // nay
   if (key == sf::Keyboard::Key::Escape)
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

   return true;
}

void MessageBox::reset()
{
   __active.reset();
}

void MessageBox::initializeControllerCallbacks()
{
   auto& gci = GameControllerIntegration::getInstance();
   if (gci.isControllerConnected())
   {
      _button_callback_a = [this]() { _button_callback_key = sf::Keyboard::Key::Enter; };
      _button_callback_b = [this]() { _button_callback_key = sf::Keyboard::Key::Escape; };

      gci.getController()->addButtonPressedCallback(SDL_GAMEPAD_BUTTON_SOUTH, _button_callback_a);
      gci.getController()->addButtonPressedCallback(SDL_GAMEPAD_BUTTON_EAST, _button_callback_b);
   }
}

void MessageBox::drawLayers(sf::RenderTarget& window, const sf::RenderStates& states)
{
   for (auto& layer : _layer_stack)
   {
      if (layer->_visible)
      {
         layer->draw(window, states);
      }
   }
}

void MessageBox::drawText(sf::RenderTarget& window, const sf::RenderStates& states)
{
   updateTextAnimation();

   for (const auto& segment : _segments)
   {
      window.draw(segment.text, states);
   }
}

void MessageBox::draw(sf::RenderTarget& window, const sf::RenderStates& states)
{
   const auto messageboxes = {__active.get(), __previous.get()};
   const auto has_valid_ptr = std::ranges::any_of(messageboxes, [](const auto ptr) { return ptr != nullptr; });

   if (!has_valid_ptr)
   {
      return;
   }

   // set up an ortho view with screen dimensions
   sf::View pixel_ortho(sf::FloatRect(
      {0.0f, 0.0f},
      {static_cast<float>(GameConfiguration::getInstance()._view_width), static_cast<float>(GameConfiguration::getInstance()._view_height)}
   ));

   window.setView(pixel_ortho);

   for (auto messagebox : messageboxes)
   {
      if (messagebox)
      {
         messagebox->drawLayers(window, states);
         messagebox->drawText(window, states);
      }
   }
}

void MessageBox::updateTextAnimation()
{
   static const std::array<float, 5> text_speeds = {0.5f, 0.75f, 1.0f, 1.5f, 2.0f};

   auto x =
      (GlobalClock::getInstance().getElapsedTime().asSeconds() - _show_time.asSeconds() -
       (_properties._animate_show_event ? animation_scale_time_show.asSeconds() : 0.0f));

   x *= _properties._animate_text_speed;
   x *= text_speeds[GameConfiguration::getInstance()._text_speed];

   // if the thing is animated we want to wait for the animation_scale_time to pass
   // so x might go into negative for that duration.
   x = std::max(0.0f, x);

   const auto to =
      !_properties._animate_text ? _plain_text.size() : std::min(static_cast<uint32_t>(x), static_cast<uint32_t>(_plain_text.size()));

   int32_t accumulated_chars_from_segments = 0;
   if (_char_animate_index != to)
   {
      _char_animate_index = to;
      for (auto& segment : _segments)
      {
         accumulated_chars_from_segments += segment.plain_text.size();
         if (to < accumulated_chars_from_segments)
         {
            // draw only subset of segment
            const auto chars_to_draw = segment.plain_text.size() - (accumulated_chars_from_segments - to);
            segment.text.setString(segment.plain_text.substr(0, chars_to_draw));
            break;
         }

         segment.text.setString(segment.plain_text);
      }
   }
}

void MessageBox::update(const sf::Time& dt)
{
   if (__active)
   {
      // handle emulated keypress event in update call to make sure it's processed before the next render call
      if (__active->_button_callback_key.has_value())
      {
         const auto key = __active->_button_callback_key.value();
         __active->_button_callback_key.reset();
         __active->keyboardKeyPressed(key);
      }

      // keyboard press event might actually delete the active messagebox
      if (__active)
      {
         __active->_elapsed += dt;
         __active->updateBoxContentLayers();

         // default to visible until adjusted by either showAnimation or hideAnimation
         __active->_state = DisplayState::Visible;

         if (!__active->_closed)
         {
            if (__active->_properties._animate_show_event)
            {
               __active->showAnimation();
            }
            else
            {
               // the position and alpha must be updated regardless of show/hide events
               __active->noAnimation();
            }
         }
      }
   }

   if (__previous)
   {
      if (__previous->_properties._animate_hide_event && __previous->_closed)
      {
         __previous->hideAnimation();
      }

      if (__previous->_reset_instance)
      {
         __previous.reset();
      }
   }
}

void MessageBox::updateBoxContentLayers()
{
   const auto xbox = (GameControllerIntegration::getInstance().isControllerConnected());
   const auto buttons = __active->_buttons;

   // init button layers
   _layers["yes_xbox_1"]->_visible = xbox && (buttons & static_cast<int32_t>(MessageBox::Button::Yes));
   _layers["no_xbox_1"]->_visible = xbox && (buttons & static_cast<int32_t>(MessageBox::Button::No));
   _layers["yes_pc_1"]->_visible = !xbox && (buttons & static_cast<int32_t>(MessageBox::Button::Yes));
   _layers["no_pc_1"]->_visible = !xbox && (buttons & static_cast<int32_t>(MessageBox::Button::No));

   // next page arrow
   _layers["next_page"]->_visible = _properties._show_next;
}

void MessageBox::updateNextPageIcon()
{
   const auto offset_px = _properties._pos.value_or(sf::Vector2f{0.0f, 0.0f});

   constexpr auto animation_speed = 8.0f;
   constexpr auto animation_amplitude = 3.0f;

   auto next_page_layer = _layers["next_page"];
   next_page_layer->_sprite->setPosition(
      _next_page_position_px + offset_px + sf::Vector2f{0.0f, std::sin(_elapsed.asSeconds() * animation_speed) * animation_amplitude}
   );
}

void MessageBox::noAnimation()
{
   auto background_color = _properties._background_color;
   auto window_layer = _layers["window"];
   auto background_layer = _layers["background"];

   const auto offset_px = _properties._pos.value_or(sf::Vector2f{0.0f, 0.0f});

   window_layer->_sprite->setColor(sf::Color::White);
   window_layer->_sprite->setScale({1.0f, 1.0f});
   window_layer->_sprite->setPosition(_window_position_px + offset_px);

   background_layer->_sprite->setColor(background_color);
   background_layer->_sprite->setScale({1.0f, 1.0f});
   background_layer->_sprite->setPosition(_background_position_px + offset_px);

   updateNextPageIcon();
   updateTextAndButtonColor(1.0f);
}

void MessageBox::updateTextAndButtonColor(float contents_alpha)
{
   const auto contents_alpha_scaled = contents_alpha * 255;
   const auto alpha = static_cast<uint8_t>(contents_alpha_scaled);
   const auto color = sf::Color{255, 255, 255, alpha};

   for (auto& segment : _segments)
   {
      auto text_color = segment.color;
      text_color.a = alpha;
      segment.text.setFillColor(text_color);
   }

   for (const auto& layer : _box_content_layers)
   {
      layer->_sprite->setColor(color);
   }
}

void MessageBox::showAnimation()
{
   auto contents_alpha = 1.0f;
   const auto visible_time = GlobalClock::getInstance().getElapsedTime() - _show_time;

   auto background_color = _properties._background_color;

   auto window_layer = _layers["window"];
   auto background_layer = _layers["background"];

   const auto offset = _properties._pos.value_or(sf::Vector2f{0.0f, 0.0f});

   if (visible_time < animation_scale_time_show)  // zoom effect
   {
      contents_alpha = 0.0f;

      const auto t_normalized = visible_time.asSeconds() / animation_scale_time_show.asSeconds();
      const auto scale_x = Easings::easeOutBack<float>(t_normalized);
      const auto scale_y = scale_x;

      background_color.a = static_cast<uint8_t>(t_normalized * 255);

      const auto background_scale_offset = (background_width_px - background_width_px * scale_x) * 0.5f;
      const auto background_pos_x_px = _background_position_px.x + background_scale_offset + offset.x;
      const auto background_pos_y_px = _background_position_px.y + offset.y;

      const auto window_scale_offset = (textbox_width_px - textbox_width_px * scale_x) * 0.5f;
      const auto window_pos_x_px = _window_position_px.x + window_scale_offset + offset.x;
      const auto window_pos_y_px = _window_position_px.y + offset.y;
      const auto window_color = sf::Color{255, 255, 255, static_cast<uint8_t>(t_normalized * 255)};

      window_layer->_sprite->setColor(window_color);
      window_layer->_sprite->setScale({scale_x, scale_y});
      window_layer->_sprite->setPosition({window_pos_x_px, window_pos_y_px});

      background_layer->_sprite->setColor(background_color);
      background_layer->_sprite->setScale({scale_x, scale_y});
      background_layer->_sprite->setPosition({background_pos_x_px, background_pos_y_px});
   }
   else  // fade in
   {
      window_layer->_sprite->setColor(sf::Color::White);
      window_layer->_sprite->setScale({1.0f, 1.0f});
      window_layer->_sprite->setPosition(_window_position_px + offset);

      background_layer->_sprite->setColor(background_color);
      background_layer->_sprite->setScale({1.0f, 1.0f});
      background_layer->_sprite->setPosition(_background_position_px + offset);

      if (visible_time < animation_scale_time_show + animation_fade_time_show)
      {
         const auto t_normalized =
            (visible_time.asSeconds() - animation_scale_time_show.asSeconds()) / animation_fade_time_show.asSeconds();

         contents_alpha = t_normalized;
      }
   }

   updateNextPageIcon();
   updateTextAndButtonColor(contents_alpha);

   // update state
   _state = (contents_alpha * 255 < 255) ? MessageBox::DisplayState::ShowAnimation : MessageBox::DisplayState::Visible;
}

void MessageBox::hideAnimation()
{
   const auto elapsed_time = GlobalClock::getInstance().getElapsedTime() - __previous->_hide_time;

   const auto t_normalized = elapsed_time.asSeconds() / animation_fade_time_hide.asSeconds();
   const auto contents_alpha = 1.0f - t_normalized;
   const auto contents_alpha_scaled = static_cast<uint8_t>(contents_alpha * 255);

   if (contents_alpha < 0.0f)
   {
      _reset_instance = true;
   }
   else
   {
      const auto window_color = sf::Color{255, 255, 255, contents_alpha_scaled};
      auto background_color = _properties._background_color;
      background_color.a = contents_alpha_scaled;

      _layers["window"]->_sprite->setColor(window_color);
      _layers["background"]->_sprite->setColor(background_color);

      for (const auto& layer : _box_content_layers)
      {
         layer->_sprite->setColor(window_color);
      }

      for (auto& segment : _segments)
      {
         auto text_color = segment.color;
         text_color.a = contents_alpha_scaled;
         segment.text.setFillColor(text_color);
      }
   }

   // update state
   _state = (contents_alpha_scaled > 0) ? MessageBox::DisplayState::HideAnimation : MessageBox::DisplayState::Hidden;
}

void MessageBox::info(const std::string& message, const MessageBoxCallback& callback, const LayoutProperties& properties, int32_t buttons)
{
   if (__active)
   {
      Log::Info() << "messagebox blocked" << std::endl;
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
      Log::Info() << "messagebox blocked" << std::endl;
      return;
   }

   messageBox(MessageBox::Type::Info, message, callback, properties, buttons);
}
