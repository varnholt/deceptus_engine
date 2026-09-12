#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

#include "json/json.hpp"

#include "game/state/savestate.h"
#include "menus/menu.h"

/// \brief attract mode: replays a recorded session when the menu has been idle, ends it on user input.
///
/// the demo is a regular game session driven by the global event serializer, so nothing about the
/// level, the player or the mechanisms knows that it is running. two things make it a demo rather
/// than a game: the player's save slot is swapped out for a throwaway one while it runs, and
/// Display::Demo is set so that everything which must not happen during a demo - writing the save
/// file, opening the pause menu - has a flag to test.
///
/// \code
///   menu idle ------ idle_timeout_s ------> demo plays ------ recording ends -----> menu
///        ^                                      |                                    |
///        |                                      | key / button                       |
///        +--------------------------------------+------------------------------------+
/// \endcode
class DemoMode
{
public:
   /// \brief one entry of the demo playlist.
   struct DemoItem
   {
      std::filesystem::path _recording_path;      //!< event recording replayed for this demo
      int32_t _level_index = 0;                   //!< index into levels.json the recording was made in
      std::optional<nlohmann::json> _save_state;  //!< save slot the demo starts from, default-constructed when absent
   };

   /// \brief returns the global demo mode singleton.
   /// \return reference to the shared demo mode instance.
   static DemoMode& getInstance();

   /// \brief reads the demo playlist and the idle timeout from the demo configuration file.
   /// \param config_path path of the demo configuration json.
   void initialize(const std::filesystem::path& config_path = "data/demo/demo.json");

   /// \brief advances the menu idle timer, starts a demo when it expires, and ends one that ran out.
   /// \param delta_time elapsed frame time.
   void update(const sf::Time& delta_time);

   /// \brief resets the idle timer and ends a running demo.
   /// \return true when a demo was ended and the triggering input should be swallowed.
   bool notifyUserInput();

   /// \brief puts the player on the spot the running demo's recording was captured at.
   ///
   /// called once the demo's level has finished loading, which is the first moment the player body
   /// exists and the last one before the level is stepped or drawn.
   void positionPlayerAtRecordedStart();

   /// \brief indicates whether a demo is currently replaying.
   /// \return true while a demo session is running.
   bool isActive() const;

   /// \brief filters input events to those that count as a user being at the keyboard.
   /// \param event event to test.
   /// \return true for key, mouse button and joystick button presses.
   static bool isUserInput(const sf::Event& event);

private:
   /// \brief swaps in the demo save slot, starts the configured level and begins replaying its recording.
   void start();

   /// \brief stops replaying, restores the player's save slot and returns to the menu the demo started from.
   void stop();

   /// \brief accumulates idle time while an idle menu is on screen and starts a demo when it expires.
   /// \param delta_time elapsed frame time.
   void updateIdleTimer(const sf::Time& delta_time);

   /// \brief ends a running demo when a controller button goes down.
   void updateControllerInput();

   std::vector<DemoItem> _demo_items;
   size_t _next_demo_index = 0;

   sf::Time _idle_time;
   sf::Time _idle_timeout = sf::seconds(60.0f);

   bool _active = false;

   //! the player's save slot, put back when the demo ends so a demo never shows up in the save file
   SaveState _save_state_backup;

   //! menu the demo was started from; the one to return to when it ends
   Menu::MenuType _menu_type_before = Menu::MenuType::Main;

   //! controller buttons are polled rather than delivered as events, so a press needs an edge
   bool _controller_button_pressed_previously = false;
};
