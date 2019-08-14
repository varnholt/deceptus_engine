#include "forestscene.h"


#include "image/psd.h"

#include <iostream>


ForestScene::ForestScene()
{

   // load ingame psd
   PSD psd;
   psd.setColorFormat(PSD::ColorFormat::ABGR);
   psd.load("data/scenes/forest.psd");

   // std::cout << mFilename << std::endl;

   for (const auto& layer : psd.getLayers())
   {
      // skip groups
      if (layer.getSectionDivider() != PSD::Layer::SectionDivider::None)
      {
         continue;
      }

      std::cout << layer.getName() << std::endl;

      auto tmp = std::make_shared<Layer>();
      tmp->mVisible = layer.isVisible();

   }
}


void ForestScene::draw(sf::RenderTarget& window, sf::RenderStates)
{

}


void ForestScene::update(const sf::Time& time)
{

}
