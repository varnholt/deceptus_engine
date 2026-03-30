#pragma once

#include "menuscreen.h"

/// \brief credits screen that exposes back navigation and device-specific prompts.
class MenuScreenCredits : public MenuScreen
{
public:
   /// \brief initializes the credits screen with its PSD layout.
   MenuScreenCredits();

   /// \brief refreshes prompt visibility after PSD layers are loaded.
   void loadingFinished() override;

   /// \brief updates controller or keyboard prompts for leaving the credits screen.
   void updateLayers();

   /// \brief processes upward navigation input and plays navigation feedback.
   void up();

   /// \brief processes downward navigation input and plays navigation feedback.
   void down();

   /// \brief processes confirm input and plays selection feedback.
   void select();

   /// \brief returns to the options menu.
   void back();

   /// \brief routes navigation and cancel keys to credits screen actions.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;
};
