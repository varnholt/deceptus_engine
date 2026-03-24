#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "framework/image/layer.h"

/// \brief base class for PSD-driven menu screens with shared input and layer rendering behavior.
class MenuScreen
{
public:
   /// \brief creates an empty menu screen.
   MenuScreen() = default;

   /// \brief destroys the menu screen.
   virtual ~MenuScreen() = default;

   /// \brief updates per-frame screen logic.
   /// \param dt frame delta time.
   virtual void update(const sf::Time& dt);

   /// \brief draws all visible layers in stack order.
   /// \param window render target that receives layer sprites.
   /// \param states render states forwarded to layer drawing.
   virtual void draw(sf::RenderTarget& window, sf::RenderStates states);

   /// \brief hook called when the screen becomes active.
   virtual void showEvent();

   /// \brief hook called when the screen is hidden.
   virtual void hideEvent();

   /// \brief returns the PSD filename used when loading layer data.
   /// \return configured PSD path.
   const std::string& getFilename();

   /// \brief sets the PSD filename used by load().
   /// \param filename path to the menu PSD file.
   void setFilename(const std::string& filename);

   /// \brief loads image layers from the configured PSD file into the internal layer containers.
   void load();

   /// \brief hook invoked after PSD layers are loaded and registered.
   virtual void loadingFinished();

   /// \brief processes keyboard press input for the screen.
   /// \param key key that was pressed.
   virtual void keyboardKeyPressed(sf::Keyboard::Key key);

   /// \brief processes keyboard release input for the screen.
   /// \param key key that was released.
   virtual void keyboardKeyReleased(sf::Keyboard::Key key);

   /// \brief handles controller X input, defaulting to a synthetic keyboard D press.
   virtual void controllerButtonX();

   /// \brief handles controller Y input.
   virtual void controllerButtonY();

   /// \brief reports whether a game controller is currently connected.
   /// \return true when the controller integration reports an active controller.
   bool isControllerUsed() const;

protected:
   std::string _filename;
   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;
};
