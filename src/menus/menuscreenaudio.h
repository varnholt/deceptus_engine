#pragma once

#include "menuscreen.h"

#include <cstdint>
#include <vector>

/// \brief audio settings screen for master, music, and sfx volume sliders.
class MenuScreenAudio : public MenuScreen
{
public:
   /// \brief identifies configurable rows in the audio settings screen.
   enum class Selection
   {
      Master = 0,
      Music = 1,
      SFX = 2,
      Count = 3,
   };

   /// \brief initializes the audio settings screen with its PSD layout.
   MenuScreenAudio();

   /// \brief routes navigation and adjustment keys to audio menu actions.
   /// \param key key that was pressed.
   void keyboardKeyPressed(sf::Keyboard::Key key) override;

   /// \brief caches layer groups used to render discrete volume values.
   void loadingFinished() override;

   /// \brief refreshes current selection and prompt layers when the screen becomes visible.
   void showEvent() override;

   /// \brief updates highlights, prompts, and value indicators for all audio rows.
   void updateLayers();

   /// \brief moves selection to the previous audio row with wrap-around.
   void up();

   /// \brief moves selection to the next audio row with wrap-around.
   void down();

   /// \brief confirms the currently selected row and plays selection feedback.
   void select();

   /// \brief returns to the options menu.
   void back();

   /// \brief adjusts the selected volume setting and clamps resulting values to 0..100.
   /// \param x signed volume delta applied to the selected setting.
   void set(int32_t x);

   /// \brief restores default audio configuration values and refreshes the screen.
   void setDefaults();

   /// \brief draws the audio screen layers and label text.
   /// \param window render target receiving the audio screen.
   /// \param states render states forwarded to draw calls.
   void draw(sf::RenderTarget& window, sf::RenderStates states) override;

   Selection _selection = Selection::Master;

   std::vector<std::shared_ptr<Layer>> _volume_layers_master;
   std::vector<std::shared_ptr<Layer>> _volume_layers_music;
   std::vector<std::shared_ptr<Layer>> _volume_layers_sfx;

private:
   sf::FloatRect _row_help_base_rect;  //!< help text reference rect for row 0 (Master)

   std::unique_ptr<sf::Text> _master_label;
   std::unique_ptr<sf::Text> _master_help_text;
   std::unique_ptr<sf::Text> _music_label;
   std::unique_ptr<sf::Text> _music_help_text;
   std::unique_ptr<sf::Text> _sfx_label;
   std::unique_ptr<sf::Text> _sfx_help_text;
};
