#ifndef INGAMEMENUARCHIVES_H
#define INGAMEMENUARCHIVES_H

#include "game/image/layerdata.h"
#include "ingamemenupage.h"

#include <SFML/Graphics.hpp>

/// \brief renders and animates the archives submenu with category selection states.
class InGameMenuArchives : public InGameMenuPage
{
public:
   /// \brief loads archive menu layers and initializes panel groups and animation timings.
   InGameMenuArchives();

   /// \brief draws the archives page using the base layered page renderer.
   /// \param window render target that receives archive layer rendering.
   /// \param states render states used for drawing.
   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates::Default) override;
   /// \brief advances show/hide or slide animations for archive panels.
   /// \param dt elapsed frame time, currently unused by this page.
   void update(const sf::Time& dt) override;

   /// \brief moves archive category selection one entry up.
   void up() override;
   /// \brief moves archive category selection one entry down.
   void down() override;

private:
   /// \brief advances horizontal slide transitions for all archive panel groups.
   void updateMove();
   /// \brief applies show and hide offsets plus fade alpha for archives layers.
   void updateShowHide();
   /// \brief applies layer visibility for the currently selected archive category.
   void updateButtons();

   std::vector<LayerData> _panel_header;
   std::vector<LayerData> _panel_left;
   std::vector<LayerData> _panel_right;
   std::vector<LayerData> _panel_background;

   int32_t _selected_index = 0;

   FloatSeconds _duration_show;
   FloatSeconds _duration_hide;
};

#endif  // INGAMEMENUARCHIVES_H
