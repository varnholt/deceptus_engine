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
   static const sf::Color color_label_normal;    //!< unselected menu item or label text
   static const sf::Color color_label_selected;  //!< selected menu item or label text
   static const sf::Color color_help_text;       //!< description / help line text
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
   /// \brief loads data/fonts/deceptum.ttf at sizes 12 and 14 with smoothing disabled.
   void ensureFontLoaded();

   /// \brief positions text so it is centered horizontally and vertically inside reference_rect.
   static void placeTextCentered(sf::Text& text, const sf::FloatRect& reference_rect);

   /// \brief positions text left-aligned at reference_rect.position.x, vertically centered.
   static void placeTextLeft(sf::Text& text, const sf::FloatRect& reference_rect);

   /// \brief positions text so its left edge sits button_text_x_offset pixels right of reference_rect's right edge, vertically centered.
   static void placeTextRightOf(sf::Text& text, const sf::FloatRect& reference_rect);

   static constexpr float button_text_x_offset = 8.0f;  //!< pixel gap between button sprite right edge and button label

   /// \brief positions deco_left flush against the left edge of reference_rect and deco_right flush against the right edge, both vertically
   /// centered.
   static void placeDecorators(sf::Sprite& deco_left, sf::Sprite& deco_right, const sf::FloatRect& reference_rect);

   /// \brief convenience overload that resolves deco_l and deco_r from the screen's own layer map.
   void placeDecorators(const sf::FloatRect& reference_rect);

   /// \brief returns a copy of base_rect shifted down by row_index * _row_stride.
   sf::FloatRect rowRect(const sf::FloatRect& base_rect, int32_t row_index) const;

   sf::FloatRect _row_label_base_rect;  //!< reference rect for the label column at row 0
   float _row_stride = 0.0f;            //!< vertical pixel distance between consecutive rows

   sf::Font _font;

   std::string _filename;
   std::vector<std::shared_ptr<Layer>> _layer_stack;
   std::map<std::string, std::shared_ptr<Layer>> _layers;
};
