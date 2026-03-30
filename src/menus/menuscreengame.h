#pragma once

#include "menuscreen.h"

/// \brief gameplay settings screen for text speed, rumble, and automatic pause behavior.
class MenuScreenGame : public MenuScreen
{
public:
   /// \brief identifies selectable gameplay settings rows.
   enum class Selection
   {
      TextSpeed = 0,
      Rumble = 1,
      AutomaticPause = 2,
      Count = 3,
   };

   /// \brief initializes the gameplay settings screen with its PSD layout.
   MenuScreenGame();

protected:
   /// \brief routes navigation and adjustment keys to gameplay settings actions.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief refreshes screen layers after PSD loading.
   void loadingFinished() override;

private:
   /// \brief returns whether rumble is enabled in the persisted game configuration.
   /// \return true when rumble is enabled.
   bool isRumbleEnabled() const;

   /// \brief updates highlights, prompts, and value indicators for gameplay settings.
   void updateLayers();

   /// \brief moves selection to the previous settings row with wrap-around.
   void up();

   /// \brief moves selection to the next settings row with wrap-around.
   void down();

   /// \brief confirms the current row and plays apply feedback.
   void select();

   /// \brief returns to the options menu.
   void back();

   /// \brief modifies the currently selected gameplay setting and persists the configuration.
   /// \param x signed step used for text-speed changes and as toggle trigger for boolean settings.
   void set(int32_t x);

   Selection _selection = Selection::TextSpeed;
};
