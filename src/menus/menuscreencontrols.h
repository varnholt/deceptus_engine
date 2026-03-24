#pragma once

#include "menuscreen.h"

/// \brief controls reference screen with prompts for defaults, rebinding, and back navigation.
class MenuScreenControls : public MenuScreen
{
public:
   /// \brief initializes the controls screen with its PSD layout.
   MenuScreenControls();

   /// \brief routes navigation and confirm keys to controls screen actions.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief refreshes prompt visibility after PSD layers are loaded.
   void loadingFinished() override;

   /// \brief updates button prompt layers based on current input device.
   void updateLayers();

   /// \brief processes upward navigation input and plays navigation feedback.
   void up();

   /// \brief processes downward navigation input and plays navigation feedback.
   void down();

   /// \brief processes confirm input and plays selection feedback.
   void select();

   /// \brief returns to the options menu.
   void back();
};
