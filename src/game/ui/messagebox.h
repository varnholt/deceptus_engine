#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>

#include <SFML/Graphics.hpp>

#include "game/constants.h"
#include "game/ui/richtextparser.h"

struct Layer;

/*! \brief Implements a simple message box
 *         The MessageBox class is implemented in a way that only a single messagebox is shown at a time (modal).
 *
 *  To show a messagebox, the functions 'info' and 'question' can be called; the message box output
 *  is retrieved by passing in a MessageBoxCallback. The message box design is configured by passing in
 *  an instance of LayoutProperties.
 */
struct MessageBox
{
   enum class Button
   {
      Invalid = 0x00,
      Ok = 0x01,
      Cancel = 0x02,
      Yes = 0x04,
      No = 0x08,
   };

   enum class Type
   {
      Info,
      Warning,
      Error
   };

   enum class DisplayState
   {
      ShowAnimation,
      HideAnimation,
      Visible,
      Hidden,
   };

   struct LayoutProperties
   {
      MessageBoxLocation _location = MessageBoxLocation::MiddleCenter;
      std::optional<sf::Vector2f> _pos;
      sf::Color _background_color = sf::Color{27, 59, 151};
      sf::Color _text_color = sf::Color{232, 219, 243};
      bool _animate_text = false;
      float _animate_text_speed = 10.0f;
      bool _centered = true;
      bool _animate_show_event = true;
      bool _animate_hide_event = true;
      int32_t _show_next = false;
   };

   using MessageBoxCallback = std::function<void(Button)>;

   MessageBox(Type type, const std::string& message, const MessageBoxCallback& cb, const LayoutProperties& properties, int32_t buttons);
   virtual ~MessageBox();

   static void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   static void update(const sf::Time& dt);
   static bool keyboardKeyPressed(sf::Keyboard::Key key);
   static void reset();

   static void info(
      const std::string& message,
      const MessageBoxCallback& callback,
      const LayoutProperties& properties = __default_properties,
      int32_t buttons = static_cast<int32_t>(Button::Ok)
   );

   static void question(
      const std::string& message,
      const MessageBoxCallback& callback,
      const LayoutProperties& properties = __default_properties,
      int32_t buttons = (static_cast<int32_t>(Button::Yes) | static_cast<int32_t>(Button::No))
   );

   void drawLayers(sf::RenderTarget& window, sf::RenderStates states);
   void drawText(sf::RenderStates states, sf::RenderTarget& window);

   void initializeControllerCallbacks();
   void initializeLayers();

   void showAnimation();
   void hideAnimation();

   void noAnimation();
   void updateBoxContentLayers();
   void updateTextAnimation();
   void updateNextPageIcon();
   void updateTextAndButtonColor(float contents_alpha);

   Type _type;

   struct TextSegment
   {
      sf::Text text;
      sf::Color color;
      std::string plain_text;
   };

   std::vector<TextSegment> _segments;
   std::string _plain_text;
   uint32_t _char_animate_index = 0;

   MessageBoxCallback _callback;
   LayoutProperties _properties;
   int32_t _buttons = 0;
   bool _closed = false;
   bool _reset_instance = false;
   std::function<void(void)> _button_callback_a;
   std::function<void(void)> _button_callback_b;
   std::optional<sf::Keyboard::Key> _button_callback_key;
   sf::Time _show_time;
   sf::Time _hide_time;
   DisplayState _state{DisplayState::Hidden};
   sf::Time _elapsed;

   sf::Vector2f _window_position_px;
   sf::Vector2f _background_position_px;
   sf::Vector2f _next_page_position_px;

   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;
   std::vector<std::shared_ptr<Layer>> _box_content_layers;

   static LayoutProperties __default_properties;
};
