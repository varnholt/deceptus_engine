#ifndef EDITOR_H
#define EDITOR_H

#include <memory>
#include "game/animation/animation.h"
#include "game/animation/animationpool.h"
#include "game/animation/animationsettings.h"

class Editor
{
public:
   Editor() = default;

   bool init();

   void draw(sf::RenderTarget& window);
   void update(const sf::Time& delta_time);

private:
   void drawCheckerboardGrid(sf::RenderTarget& window, float cell_size);
   void drawAnimationSettings();
   void selectAnimation(int32_t index);
   void drawAnimation(sf::RenderTarget& window);
   void selectAnimByCursorKey();
   void populateComboBox();
   void assignCurrentAnimation(const std::shared_ptr<Animation>& animation);

   std::shared_ptr<Animation> _current_animation = nullptr;
   std::shared_ptr<AnimationSettings> _current_settings = nullptr;

   bool _playing = true;
   std::vector<std::string> _animation_names;
   std::unique_ptr<AnimationPool> _animation_pool;
   std::optional<int32_t> _selected_index;
   void drawControls();
   void loadAnimationNames();

   bool _reloaded{false};
};

#endif  // EDITOR_H
