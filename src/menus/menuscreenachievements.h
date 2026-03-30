#pragma once

#include "menuscreen.h"

/// \brief achievements menu screen that currently only provides a return path to options.
class MenuScreenAchievements : public MenuScreen
{
public:
   /// \brief initializes the achievements screen with its PSD layout.
   MenuScreenAchievements();

   /// \brief routes navigation and cancel keys to achievements menu actions.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief refreshes layer visibility after PSD loading.
   void loadingFinished() override;

   /// \brief toggles controller or keyboard button prompts for the back action.
   void updateLayers();

   /// \brief processes upward navigation input.
   void up();

   /// \brief processes downward navigation input.
   void down();

   /// \brief processes confirm input.
   void select();

   /// \brief returns to the options menu.
   void back();
};
