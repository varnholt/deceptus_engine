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
         MessageBoxLocation mLocation = MessageBoxLocation::MiddleCenter;
         sf::Color mBackgroundColor = sf::Color{47, 12, 75};
         sf::Color mTextColor = sf::Color{232, 219, 243};
         bool mAnimate = false;
         bool mCentered = true;
      };

      using MessageBoxCallback = std::function<void(Button)>;

      MessageBox(
         Type type,
         const std::string& message,
         MessageBoxCallback cb,
         const LayoutProperties& properties,
         int32_t buttons
      );

      virtual ~MessageBox();

      static void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default);
      static bool keyboardKeyPressed(sf::Keyboard::Key key);

      static void info(
         const std::string& message,
         MessageBoxCallback callback,
         const LayoutProperties& properties = sDefaultProperties,
         int buttons = static_cast<int32_t>(Button::Ok)
      );

      static void question(
         const std::string& message,
         MessageBoxCallback callback,
         const LayoutProperties& properties = sDefaultProperties,
         int buttons = (static_cast<int32_t>(Button::Yes) | static_cast<int32_t>(Button::No))
      );


   private:

      static void messageBox(
         Type type,
         const std::string& message,
         MessageBoxCallback callback,
         const LayoutProperties& properties,
         int32_t buttons
      );

      static void initializeLayers();
      static sf::Vector2i pixelLocation(MessageBoxLocation);

      void initializeControllerCallbacks();

      Type mType;
      std::string mTitle; // still unsupported
      std::string mMessage;
      MessageBoxCallback mCallback;
      LayoutProperties mProperties;
      int32_t mButtons = 0;
      uint32_t mCharsShown = 0;
      bool mDrawn = false;
      std::function<void(void)> mButtonCallbackA;
      std::function<void(void)> mButtonCallbackB;
      sf::Time mShowTime;
      ExecutionMode mPreviousMode = ExecutionMode::None;

      static LayoutProperties sDefaultProperties;
      static std::unique_ptr<MessageBox> sActive;
      static std::vector<std::shared_ptr<Layer>> sLayerStack; // SLAYER!
      static std::map<std::string, std::shared_ptr<Layer>> sLayers;
      static sf::Font sFont;
      static sf::Text sText;
      static bool sInitialized;
};

