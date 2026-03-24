#pragma once

#include "menuscreen.h"

#include <functional>

/// \brief pause menu screen for resuming gameplay, opening options, or quitting to main menu.
class MenuScreenPause : public MenuScreen
{
public:
   /// \brief identifies selectable rows in the pause menu.
   enum class Selection
   {
      Resume,
      Options,
      Quit
   };

   /// \brief initializes the pause screen with its PSD layout.
   MenuScreenPause();

   /// \brief updates pause-screen logic; currently no per-frame behavior is needed.
   /// \param dt frame delta time.
   void update(const sf::Time& dt) override;

   /// \brief routes navigation, confirm, and cancel keys to pause menu actions.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief refreshes pause menu layers after PSD loading.
   void loadingFinished() override;

   /// \brief updates highlighted rows and controller or keyboard prompts.
   void updateLayers();

   /// \brief moves selection one row up with wrap-around.
   void up();

   /// \brief moves selection one row down with wrap-around.
   void down();

   /// \brief activates the currently selected pause action.
   void select();

   /// \brief resets selection to resume whenever the pause menu is shown.
   void showEvent() override;

   Selection _selection = Selection::Resume;

private:
   /// \brief closes the pause menu, resumes gameplay, and restores level music playback.
   void resume();
};
