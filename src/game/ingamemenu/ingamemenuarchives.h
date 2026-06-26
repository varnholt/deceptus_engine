#ifndef INGAMEMENUARCHIVES_H
#define INGAMEMENUARCHIVES_H

#include "framework/tools/localization.h"
#include "game/animation/animation.h"
#include "game/animation/animationpool.h"
#include "game/image/layerdata.h"
#include "ingamemenupage.h"

#include <SFML/Graphics.hpp>
#include <map>
#include <memory>
#include <string>

using SpriteAnimation = Animation;

/// \brief renders and animates the archives submenu with category selection states.
class InGameMenuArchives : public InGameMenuPage
{
public:
   /// \brief loads archive menu layers and initializes panel groups and animation timings.
   InGameMenuArchives();

   /// \brief draws the archives page using the base layered page renderer.
   /// \param window render target that receives archive layer rendering.
   /// \param states render states used for drawing.
   void draw(sf::RenderTarget& window, sf::RenderStates = sf::RenderStates{}) override;

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

   /// \brief ensures an animation exists for every collected treasure and advances all of them.
   /// \param dt elapsed frame time.
   void updateTreasureAnimations(const sf::Time& dt);

   /// \brief draws collected treasure animations and their name and description texts.
   /// \param window render target that receives treasure rendering.
   /// \param states render states used for drawing.
   void drawTreasures(sf::RenderTarget& window, sf::RenderStates states);

   std::vector<LayerData> _panel_header;
   std::vector<LayerData> _panel_left;
   std::vector<LayerData> _panel_right;
   std::vector<LayerData> _panel_background;

   int32_t _selected_index = 0;

   FloatSeconds _duration_show;
   FloatSeconds _duration_hide;
   float _content_alpha{1.0f};  //!< current fade alpha for treasure text and animations

   std::unique_ptr<AnimationPool> _animation_pool;
   std::map<std::string, std::shared_ptr<SpriteAnimation>> _treasure_animations;

   const sf::Font* _font_treasure = &getFont();
   std::unique_ptr<sf::Text> _text_treasure_name;
   std::unique_ptr<sf::Text> _text_treasure_description;
};

#endif  // INGAMEMENUARCHIVES_H
