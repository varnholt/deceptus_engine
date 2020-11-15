#pragma once

#include <memory>
#include <string>
#include <vector>

#include "framework/image/layer.h"


class MenuScreen
{

public:

   MenuScreen() = default;
   virtual ~MenuScreen() = default;

   virtual void update(const sf::Time& dt);
   virtual void draw(sf::RenderTarget& window, sf::RenderStates states);
   virtual void showEvent();
   virtual void hideEvent();

   const std::string& getFilename();
   void setFilename(const std::string& filename);

   void load();
   virtual void loadingFinished();

   virtual void keyboardKeyPressed(sf::Keyboard::Key key);
   virtual void keyboardKeyReleased(sf::Keyboard::Key key);
   virtual void controllerButtonX();
   virtual void controllerButtonY();

   bool isControllerUsed() const;


protected:

   std::string mFilename;
   std::vector<std::shared_ptr<Layer>> mLayerStack;
   std::map<std::string, std::shared_ptr<Layer>> mLayers;
};

