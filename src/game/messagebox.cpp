#include "messagebox.h"

#include "game/gameconfiguration.h"
#include "image/psd.h"

#include <iostream>


std::deque<MessageBox> MessageBox::mQueue;
bool MessageBox::sInitialized = false;

std::vector<std::shared_ptr<Layer>> MessageBox::sLayerStack;
std::map<std::string, std::shared_ptr<Layer>> MessageBox::sLayers;
sf::Font MessageBox::sFont;


bool MessageBox::empty()
{
   return mQueue.empty();
}


bool MessageBox::keyboardKeyPressed(sf::Keyboard::Key key)
{
   if (empty())
   {
      return false;
   }

   auto& box = mQueue.front();

   if (box.mDrawn)
   {
      MessageBox::Button button = MessageBox::Button::Invalid;

      // yay
      if (key == sf::Keyboard::Return)
      {
         if (box.mButtons & static_cast<int32_t>(Button::Yes))
         {
            button = Button::Yes;
         }
         else if (box.mButtons & static_cast<int32_t>(Button::Ok))
         {
            button = Button::Ok;
         }
      }

      // nay
      if (key == sf::Keyboard::Escape)
      {
         if (box.mButtons & static_cast<int32_t>(Button::No))
         {
            button = Button::No;
         }
         else if (box.mButtons & static_cast<int32_t>(Button::Cancel))
         {
            button = Button::Cancel;
         }
      }

      if (button != MessageBox::Button::Invalid)
      {
         box.mCallback(button);
         mQueue.pop_front();
      }
   }

   return true;
}


MessageBox::MessageBox()
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

         texture->create(layer.getWidth(), layer.getHeight());
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


void MessageBox::draw(sf::RenderTarget& window, sf::RenderStates states)
{
   if (empty())
   {
      return;
   }

   auto& box = mQueue.front();
   box.mDrawn = true;

   const auto xbox = false;
   const auto buttons = box.mButtons;

   sLayers["msg-copyssYN"]->mVisible = false;
   sLayers["msg-overwritessYN"]->mVisible = false;
   sLayers["msg-deletessYN"]->mVisible = false;
   sLayers["msg-defaultsYN"]->mVisible = false;
   sLayers["msg-quitYN"]->mVisible = false;

   sLayers["yes_xbox_1"]->mVisible = xbox && buttons & static_cast<int32_t>(Button::Yes);
   sLayers["no_xbox_1"]->mVisible = xbox && buttons & static_cast<int32_t>(Button::No);

   sLayers["yes_pc_1"]->mVisible = !xbox && buttons & static_cast<int32_t>(Button::Yes);
   sLayers["no_pc_1"]->mVisible = !xbox && buttons & static_cast<int32_t>(Button::No);

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

   sf::Text text;
   text.setScale(0.25f, 0.25f);
   text.setFont(sFont);
   text.setCharacterSize(48);
   text.setString(box.mMessage);
   text.setFillColor(sf::Color{232, 219, 243});

   // box top/left: 137 x 94
   // box dimensions: 202 x 71
   // box left: 143
   const auto rect = text.getGlobalBounds();
   const auto left = 143;
   const auto x = left + (202 - rect.width) * 0.5f;

   text.setPosition(floor(x), 112);
   window.draw(text, states);
}


void MessageBox::messageBox(Type type, const std::string& message, MessageBoxCallback callback, int32_t buttons)
{
   MessageBox m;
   m.mMessage = message;
   m.mType = type;
   m.mCallback = callback;
   m.mButtons = buttons;

   mQueue.push_back(m);
}


void MessageBox::info(const std::string& message, MessageBoxCallback callback, int32_t buttons)
{
   messageBox(MessageBox::Type::Info, message, callback, buttons);
}


void MessageBox::question(const std::string& message, MessageBox::MessageBoxCallback callback, int32_t buttons)
{
   messageBox(MessageBox::Type::Info, message, callback, buttons);
}


// https://www.sfml-dev.org/tutorials/2.5/graphics-text.php


