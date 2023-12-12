#pragma once

#include <functional>
#include <memory>
#include <optional>

#include <SFML/Graphics.hpp>

#include "constants.h"

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

   enum class State
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
   };

   using MessageBoxCallback = std::function<void(Button)>;

   MessageBox(Type type, const std::string& message, const MessageBoxCallback& cb, const LayoutProperties& properties, int32_t buttons);
   virtual ~MessageBox();

   static void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
   static void update(const sf::Time& dt);
   static bool keyboardKeyPressed(sf::Keyboard::Key key);

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
   void updateContents();
   void updateButtonLayers();
   void animateText();

   Type _type;
   std::string _message;
   MessageBoxCallback _callback;
   LayoutProperties _properties;
   int32_t _buttons = 0;
   uint32_t _chars_shown = 0;
   bool _initialized = false;
   bool _closed = false;
   bool _reset_instance = false;
   std::function<void(void)> _button_callback_a;
   std::function<void(void)> _button_callback_b;
   sf::Time _show_time;
   sf::Time _hide_time;
   State _state{State::Hidden};
   sf::Text _text;
   sf::Vector2f _window_position;
   sf::Vector2f _background_position;
   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;
   std::vector<std::shared_ptr<Layer>> _box_content_layers;

   static LayoutProperties __default_properties;
};
