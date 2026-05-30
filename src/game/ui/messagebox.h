#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>

#include <SFML/Graphics.hpp>

#include "game/constants.h"
#include "game/ui/richtextparser.h"

struct Layer;

/// \brief modal message box controller that renders one active dialog at a time.
struct MessageBox
{
   /// \brief button flags that define available user responses.
   enum class Button
   {
      Invalid = 0x00,
      Ok = 0x01,
      Cancel = 0x02,
      Yes = 0x04,
      No = 0x08,
   };

   /// \brief semantic message category selected by callers.
   enum class Type
   {
      Info,
      Warning,
      Error
   };

   /// \brief internal visibility and animation state for one message box instance.
   enum class DisplayState
   {
      ShowAnimation,
      HideAnimation,
      Visible,
      Hidden,
   };

   /// \brief per-dialog visual and animation configuration.
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

   /// \brief creates a dialog instance, loads visual layers, parses text, and enables modal display mode.
   /// \param type semantic message category.
   /// \param message dialog text, including optional rich-text tags.
   /// \param cb callback invoked with the selected button when the dialog closes.
   /// \param properties layout, color, and animation settings for this dialog.
   /// \param buttons bitmask of enabled buttons from MessageBox::Button.
   MessageBox(Type type, const std::string& message, const MessageBoxCallback& cb, const LayoutProperties& properties, int32_t buttons);

   /// \brief removes controller callbacks and leaves modal display mode.
   virtual ~MessageBox();

   /// \brief draws the active and fading-out previous dialogs.
   /// \param window render target used for message box rendering.
   static void draw(sf::RenderTarget& window, const sf::RenderStates& = sf::RenderStates::Default);

   /// \brief updates animations, button prompts, and deferred controller input.
   /// \param dt frame delta time.
   static void update(const sf::Time& dt);

   /// \brief handles confirm or cancel key input for the active dialog.
   /// \param key keyboard key to process.
   /// \return true when the key was handled by the message box system.
   static bool keyboardKeyPressed(sf::Keyboard::Key key);

   /// \brief clears the active dialog instance immediately.
   static void reset();

   /// \brief opens an info dialog when no other dialog is active.
   /// \param message dialog text, including optional rich-text tags.
   /// \param callback callback invoked with the selected button.
   /// \param properties layout, color, and animation settings.
   /// \param buttons enabled button bitmask, defaulting to ok.
   static void info(
      const std::string& message,
      const MessageBoxCallback& callback,
      const LayoutProperties& properties = __default_properties,
      int32_t buttons = static_cast<int32_t>(Button::Ok)
   );

   /// \brief opens a question dialog when no other dialog is active.
   /// \param message dialog text, including optional rich-text tags.
   /// \param callback callback invoked with the selected button.
   /// \param properties layout, color, and animation settings.
   /// \param buttons enabled button bitmask, defaulting to yes and no.
   static void question(
      const std::string& message,
      const MessageBoxCallback& callback,
      const LayoutProperties& properties = __default_properties,
      int32_t buttons = (static_cast<int32_t>(Button::Yes) | static_cast<int32_t>(Button::No))
   );

   /// \brief draws all visible image layers of this dialog instance.
   /// \param window render target used for drawing.
   /// \param states render states forwarded to layer drawing.
   void drawLayers(sf::RenderTarget& window, const sf::RenderStates& states);

   /// \brief updates text animation and draws all parsed text segments.
   /// \param window render target used for text drawing.
   /// \param states render states forwarded to sf::Text drawing.
   void drawText(sf::RenderTarget& window, const sf::RenderStates& states);

   /// \brief registers gamepad button callbacks that emulate enter and escape input.
   void initializeControllerCallbacks();

   /// \brief loads PSD layers, creates sprites, and caches key layer positions.
   void initializeLayers();

   /// \brief runs the show animation and updates alpha, scale, and state.
   void showAnimation();

   /// \brief runs the hide animation and marks instance reset when finished.
   void hideAnimation();

   /// \brief applies fully visible transforms when show and hide animations are disabled.
   void noAnimation();

   /// \brief updates button prompt visibility based on input device and button mask.
   void updateBoxContentLayers();

   /// \brief reveals text characters over time when text animation is enabled.
   void updateTextAnimation();

   /// \brief animates the next-page icon with a vertical sine offset.
   void updateNextPageIcon();

   /// \brief applies alpha to text segments and content button layers.
   /// \param contents_alpha normalized alpha factor in range 0.0 to 1.0.
   void updateTextAndButtonColor(float contents_alpha);

   Type _type;

   /// \brief cached render segment with original color and plain-text payload.
   struct TextSegment
   {
      sf::Text text;
      sf::Color color;
      sf::String plain_text;
   };

   std::vector<TextSegment> _segments;
   sf::String _plain_text;
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
