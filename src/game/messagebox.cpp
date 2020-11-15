#include "messagebox.h"

#include "displaymode.h"
#include "framework/image/psd.h"
#include "framework/joystick/gamecontroller.h"
#include "game/gameconfiguration.h"
#include "game/gamecontrollerintegration.h"
#include "gamestate.h"
#include "player/player.h"
#include "tools/globalclock.h"

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


std::unique_ptr<MessageBox> MessageBox::sActive;
MessageBox::LayoutProperties MessageBox::sDefaultProperties;
bool MessageBox::sInitialized = false;

std::vector<std::shared_ptr<Layer>> MessageBox::sLayerStack;
std::map<std::string, std::shared_ptr<Layer>> MessageBox::sLayers;
sf::Font MessageBox::sFont;
sf::Text MessageBox::sText;


MessageBox::MessageBox(
   MessageBox::Type type,
   const std::string& message,
   MessageBox::MessageBoxCallback cb,
   const LayoutProperties& properties,
   int32_t buttons
)
 : mType(type),
   mMessage(message),
   mCallback(cb),
   mProperties(properties),
   mButtons(buttons)
{
   initializeLayers();
   initializeControllerCallbacks();
   mShowTime = GlobalClock::getInstance()->getElapsedTime();

   DisplayMode::getInstance().enqueueSet(Display::DisplayModal);

   Player::getCurrent()->getControls().setKeysPressed(0);

   //sText.setScale(0.25f, 0.25f);
   sText.setFont(sFont);
   sText.setCharacterSize(12);
   sText.setFillColor(mProperties.mTextColor);
   sText.setString("");

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
      gci->getController()->removeButtonPressedCallback(SDL_CONTROLLER_BUTTON_A, mButtonCallbackA);
      gci->getController()->removeButtonPressedCallback(SDL_CONTROLLER_BUTTON_B, mButtonCallbackB);
   }

   DisplayMode::getInstance().enqueueUnset(Display::DisplayModal);

   //   if (mPreviousMode == ExecutionMode::Running)
   //   {
   //      GameState::getInstance().enqueueResume();
   //   }
}


bool MessageBox::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (!sActive)
   {
      return false;
   }

   if (sActive->mDrawn)
   {
      MessageBox::Button button = MessageBox::Button::Invalid;

      // yay
      if (key == sf::Keyboard::Return)
      {
         if (sActive->mButtons & static_cast<int32_t>(Button::Yes))
         {
            button = Button::Yes;
         }
         else if (sActive->mButtons & static_cast<int32_t>(Button::Ok))
         {
            button = Button::Ok;
         }

         if (sActive->mProperties.mAnimate)
         {
            if (sActive->mCharsShown < sActive->mMessage.length())
            {
               sActive->mProperties.mAnimate = false;
               return true;
            }
         }
      }

      // nay
      if (key == sf::Keyboard::Escape)
      {
         if (sActive->mButtons & static_cast<int32_t>(Button::No))
         {
            button = Button::No;
         }
         else if (sActive->mButtons & static_cast<int32_t>(Button::Cancel))
         {
            button = Button::Cancel;
         }
      }

      // call callback and close message box
      if (button != MessageBox::Button::Invalid)
      {
         auto callback = sActive->mCallback;
         sActive.reset();

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
   if (!sInitialized)
   {
      PSD psd;
      psd.setColorFormat(PSD::ColorFormat::ABGR);
      psd.load("data/game/messagebox.psd");

      if (!sFont.loadFromFile("data/fonts/deceptum.ttf"))
      {
         std::cerr << "font load fuckup" << std::endl;
      }

      const_cast<sf::Texture&>(sFont.getTexture(12)).setSmooth(false);

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

         tmp->mTexture = texture;
         tmp->mSprite = sprite;

         sLayerStack.push_back(tmp);
         sLayers[layer.getName()] = tmp;
      }

      sInitialized = true;
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
      mButtonCallbackA = [](){keyboardKeyPressed(sf::Keyboard::Return);};
      mButtonCallbackB = [](){keyboardKeyPressed(sf::Keyboard::Escape);};
      gci->getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_A, mButtonCallbackA);
      gci->getController()->addButtonPressedCallback(SDL_CONTROLLER_BUTTON_B, mButtonCallbackB);
   }
}


void MessageBox::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   if (!sActive)
   {
      return;
   }

   sActive->mDrawn = true;

   const auto xbox = (GameControllerIntegration::getInstance(0) != nullptr);
   const auto buttons = sActive->mButtons;
   bool menuShown = (DisplayMode::getInstance().isSet(Display::DisplayMainMenu));

   sLayers["msg-copyssYN"]->mVisible = false;
   sLayers["msg-overwritessYN"]->mVisible = false;
   sLayers["msg-deletessYN"]->mVisible = false;
   sLayers["msg-defaultsYN"]->mVisible = false;
   sLayers["msg-quitYN"]->mVisible = false;
   sLayers["temp_bg"]->mVisible = menuShown;
   sLayers["yes_xbox_1"]->mVisible = xbox && buttons & static_cast<int32_t>(Button::Yes);
   sLayers["no_xbox_1"]->mVisible = xbox && buttons & static_cast<int32_t>(Button::No);
   sLayers["yes_pc_1"]->mVisible = !xbox && buttons & static_cast<int32_t>(Button::Yes);
   sLayers["no_pc_1"]->mVisible = !xbox && buttons & static_cast<int32_t>(Button::No);
   sLayers["temp_bg"]->mVisible = false;

   // set up an ortho view with screen dimensions
   sf::View pixelOrtho(
      sf::FloatRect(
         0.0f,
         0.0f,
         static_cast<float>(GameConfiguration::getInstance().mViewWidth),
         static_cast<float>(GameConfiguration::getInstance().mViewHeight)
      )
   );

   window.setView(pixelOrtho);

   for (auto& layer : sLayerStack)
   {
      if (layer->mVisible)
      {
         layer->draw(window, states);
      }
   }

   sActive->mMessage = replaceAll(sActive->mMessage, "[br]", "\n");

   if (sActive->mProperties.mAnimate)
   {
      auto x = (GlobalClock::getInstance()->getElapsedTime().asSeconds() - sActive->mShowTime.asSeconds()) * 10.0f;
      auto to = std::min(static_cast<uint32_t>(x), static_cast<uint32_t>(sActive->mMessage.size()));
      if (sActive->mCharsShown != to)
      {
         sActive->mCharsShown = to;
         sText.setString(sActive->mMessage.substr(0, to));
      }
   }
   else
   {
      sText.setString(sActive->mMessage);
   }

   // text alignment
   const auto pos = pixelLocation(sActive->mProperties.mLocation);
   auto x = 0;
   if (sActive->mProperties.mCentered)
   {
      // box top/left: 137 x 94
      // box dimensions: 202 x 71
      // box left: 143
      const auto rect = sText.getGlobalBounds();
      const auto left = pos.x;
      x = static_cast<int32_t>(left + (202 - rect.width) * 0.5f);
   }
   else
   {
      x = pos.x;
   }

   sText.setPosition(
      static_cast<float>(x),
      static_cast<float>(pos.y)
   );

   window.draw(sText, states);
}


void MessageBox::messageBox(
   Type type,
   const std::string& message,
   MessageBoxCallback callback,
   const LayoutProperties& properties,
   int32_t buttons
)
{
   sActive = std::make_unique<MessageBox>(type, message, callback, properties, buttons);
}


void MessageBox::info(
   const std::string& message,
   MessageBoxCallback callback,
   const LayoutProperties& properties,
   int32_t buttons
)
{
   if (sActive)
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
   if (sActive)
   {
      return;
   }

   messageBox(MessageBox::Type::Info, message, callback, properties, buttons);
}


