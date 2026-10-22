#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <vector>

/// \brief breaks a texture, or a set of tiles, into tile-sized pieces that tumble down and fade out.
/// \details used by a blocking rect that collapses when it is disabled.
class TextureCollapse
{
public:
   /// \brief one piece the collapse starts from.
   struct Fragment
   {
      std::shared_ptr<sf::Texture> _texture_map;  //!< texture the fragment is cut from
      std::shared_ptr<sf::Texture> _normal_map;   //!< optional normal map drawn for the fragment
      sf::IntRect _texture_rect;                  //!< the part of the texture the fragment shows
      sf::Vector2f _top_left_px;                  //!< where the top left corner of the fragment is drawn
   };

   /// \brief splits the texture into pieces and starts the collapse, replacing one that is still running.
   /// \param texture_map texture the pieces are cut from; nothing collapses without it.
   /// \param normal_map optional normal map drawn for each piece.
   /// \param top_left_px where the top left corner of the texture is drawn.
   void
   start(const std::shared_ptr<sf::Texture>& texture_map, const std::shared_ptr<sf::Texture>& normal_map, const sf::Vector2f& top_left_px);

   /// \brief starts the collapse from the given fragments, replacing one that is still running.
   /// \param fragments the pieces that fall; fragments without a texture are skipped.
   void start(const std::vector<Fragment>& fragments);

   /// \brief lets the pieces fall and ends the collapse once they have faded out.
   /// \param dt elapsed frame time.
   void update(const sf::Time& dt);

   /// \brief draws the falling pieces and their normals.
   /// \param target color render target.
   /// \param normal normal-map render target.
   /// \param states render states to apply.
   void draw(sf::RenderTarget& target, sf::RenderTarget& normal, const sf::RenderStates& states);

   /// \brief reports whether pieces are still falling or fading.
   /// \return true while the collapse runs.
   bool isRunning() const;

private:
   /// \brief one tile-sized piece of the texture that falls away when the rectangle collapses.
   struct CollapsePiece
   {
      std::shared_ptr<sf::Texture> _texture_map;  //!< texture this piece is cut from
      std::shared_ptr<sf::Texture> _normal_map;   //!< optional normal map of this piece
      sf::IntRect _texture_rect;                  //!< the part of the texture this piece shows
      sf::Vector2f _position_px;                  //!< current centre of the piece
      float _velocity_y_px_s{0.0f};               //!< current falling speed
      float _delay_s{0.0f};                       //!< time before the piece starts to fall
      float _rotation_degrees{0.0f};              //!< current tilt of the piece
      float _rotation_speed_degrees_s{0.0f};      //!< how fast the piece tumbles
   };

   float _collapse_elapsed_s{0.0f};  //!< time since the collapse started
   std::vector<CollapsePiece> _collapse_pieces;
   std::shared_ptr<sf::Sprite> _collapse_sprite;
};
