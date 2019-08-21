#pragma once

#include "menuscreen.h"

#include <functional>

class MenuScreenPause : public MenuScreen
{
   public:

      enum class Selection {
         Resume,
         Options,
         Quit
      };

      using ExitCallback = std::function<void(void)>;

      MenuScreenPause();

      void update(const sf::Time& dt) override;

      Selection mSelection = Selection::Resume;

      void keyboardKeyPressed(sf::Keyboard::Key key) override;

      void loadingFinished() override;
      void updateLayers();

      void up();
      void down();
      void select();

      void setExitCallback(ExitCallback callback);


   private:

      ExitCallback mExitCallback;

      void resume();
};

