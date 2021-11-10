#pragma once

#include <functional>
#include <memory>

#include <SFML/Graphics.hpp>

#include "constants.h"
#include "framework/image/layer.h"


class MessageBox
{
   public:

      enum class Button {
         Invalid = 0x00,
         Ok      = 0x01,
         Cancel  = 0x02,
         Yes     = 0x04,
         No      = 0x08,
      };

      enum class Type {
         Info,
         Warning,
         Error
      };

      struct LayoutProperties {
         MessageBoxLocation _location = MessageBoxLocation::MiddleCenter;
         sf::Color _background_color = sf::Color{47, 12, 75};
         sf::Color _text_color = sf::Color{232, 219, 243};
         bool _animate_text = false;
         bool _centered = true;
         bool _animate_show_event = true;
         bool _animate_hide_event = true;
      };

      using MessageBoxCallback = std::function<void(Button)>;

      MessageBox(
         Type type,
         const std::string& message,
         const MessageBoxCallback& cb,
         const LayoutProperties& properties,
         int32_t buttons
      );

      virtual ~MessageBox();

      static void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
      static bool keyboardKeyPressed(sf::Keyboard::Key key);

      static void info(
         const std::string& message,
         const MessageBoxCallback& callback,
         const LayoutProperties& properties = __default_properties,
         int buttons = static_cast<int32_t>(Button::Ok)
      );

      static void question(
         const std::string& message,
         const MessageBoxCallback& callback,
         const LayoutProperties& properties = __default_properties,
         int buttons = (static_cast<int32_t>(Button::Yes) | static_cast<int32_t>(Button::No))
      );

      static void showAnimation();
      static void hideAnimation();

   private:

      static void messageBox(
         Type type,
         const std::string& message,
         const MessageBoxCallback& callback,
         const LayoutProperties& properties,
         int32_t buttons
      );

      static void initializeLayers();
      static sf::Vector2i pixelLocation(MessageBoxLocation);
      static void close(Button button);

      void initializeControllerCallbacks();

      Type _type;
      std::string _title; // still unsupported
      std::string _message;
      MessageBoxCallback _callback;
      LayoutProperties _properties;
      int32_t _buttons = 0;
      uint32_t _chars_shown = 0;
      bool _drawn = false;
      bool _closed = false;
      bool _reset_instance = false;
      std::function<void(void)> _button_callback_a;
      std::function<void(void)> _button_callback_b;
      sf::Time _show_time;
      sf::Time _hide_time;
      ExecutionMode _previous_mode = ExecutionMode::None;

      static LayoutProperties __default_properties;
      static std::unique_ptr<MessageBox> __active;
      static std::vector<std::shared_ptr<Layer>> __layer_stack;
      static std::map<std::string, std::shared_ptr<Layer>> __layers;
      static std::vector<std::shared_ptr<Layer>> __box_content_layers;
      static sf::Font __font;
      static sf::Text __text;
      static sf::Vector2f __window_position;
      static bool __initialized;
};

