#pragma once

#include "framework/joystick/gamecontrollerinfo.h"

#include "ingamemenuarchives.h"
#include "ingamemenuaudio.h"
#include "ingamemenuinventory.h"
#include "ingamemenumap.h"

#include <SFML/Graphics.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

struct InventoryItem;

/// \brief coordinates the in-game menu pages, input routing, transitions, and menu audio.
class InGameMenu
{
public:
   enum class SubMenu
   {
      Map = 0,
      Inventory = 1,
      Archives = 2,
   };

   using AudioCallback = std::function<void(InGameMenuAudio::SoundEffect)>;

   /// \brief constructs submenu pages, input bindings, and default menu state.
   InGameMenu();

   /// \brief draws the selected submenu and any still-animating previous submenu.
   /// \param window render target that receives the menu draw calls.
   void draw(sf::RenderTarget& window, const sf::RenderStates& = sf::RenderStates{});

   /// \brief advances controller state and active page animations.
   /// \param dt elapsed frame time passed to menu pages.
   void update(const sf::Time& dt);

   /// \brief handles keyboard input for page navigation, page switching, and closing.
   /// \param event key press event forwarded by the game loop.
   void processEvent(const sf::Event::KeyPressed* event);

   /// \brief opens the in-game menu if gameplay state allows it.
   void open();

   /// \brief starts closing the in-game menu when it is currently visible.
   void close();

   /// \brief forwards a left navigation action to the selected submenu.
   void left();

   /// \brief forwards a right navigation action to the selected submenu.
   void right();

   /// \brief forwards an up navigation action to the selected submenu.
   void up();

   /// \brief forwards a down navigation action to the selected submenu.
   void down();

   /// \brief starts the show animation on the selected submenu.
   void show();

   /// \brief starts the hide animation on the selected submenu.
   void hide();

   /// \brief stores the latest controller snapshot used for analog and d-pad navigation.
   /// \param joystickInfo controller state sampled from the active gamepad.
   void setJoystickInfo(const GameControllerInfo& joystickInfo);

   /// \brief sets a callback used to play menu sound effects.
   /// \param callback function invoked for menu audio events.
   void setAudioCallback(const AudioCallback&);

private:
   /// \brief registers gamepad button callbacks for opening, closing, and tab switching.
   void initializeController();

   /// \brief refreshes cached joystick data from the connected controller.
   void updateController();

   /// \brief switches to the next submenu and starts slide transition animations.
   void goToRightSubMenu();

   /// \brief switches to the previous submenu and starts slide transition animations.
   void goToLeftSubMenu();

   /// \brief prints the selected and previous submenu names for debugging.
   void debug();

   /// \brief converts joystick axis and hat input into directional menu actions.
   void updateControllerActions();

   /// \brief checks whether repeated controller navigation should be throttled.
   /// \return true when the repeat delay window is still active.
   bool isControllerActionSkipped() const;

   /// \brief dispatches a directional navigation call to the selected submenu unless it is hiding.
   /// \param action callable invoked with the current page when navigation is allowed.
   void navigate(std::function<void(InGameMenuPage&)> action);

   /// \brief rotates submenu ordering one step to the right and updates selection state.
   void rotateRight();

   /// \brief rotates submenu ordering one step to the left and updates selection state.
   void rotateLeft();

   GameControllerInfo _joystick_info;
   float _joystick_update_time = 0.0f;

   std::shared_ptr<InGameMenuArchives> _menu_archives;
   std::shared_ptr<InGameMenuInventory> _menu_inventory;
   std::shared_ptr<IngameMenuMap> _menu_map;

   SubMenu _selected_submenu{SubMenu::Inventory};
   std::optional<SubMenu> _previous_submenu;

   std::array<SubMenu, 3> _submenu_selection;
   std::array<std::shared_ptr<InGameMenuPage>, 3> _submenu_type_map;
   std::array<std::string, 3> _submenu_type_names;

   InGameMenuAudio _audio;
   AudioCallback _audio_callback;
};
