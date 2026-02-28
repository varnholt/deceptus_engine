#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

/*! \brief holds all render targets used for level rendering
 *
 *  this struct manages the collection of render textures needed for deferred rendering.
 *  textures are owned by the Game class and recreated only when window dimensions change.
 *  Level receives a const reference to access them for rendering.
 */
struct RenderTargets
{
   std::shared_ptr<sf::RenderTexture> level;             //!< main level texture (foreground layers, mechanisms, player)
   std::shared_ptr<sf::RenderTexture> level_background;  //!< background layers (z=background min..max)
   std::shared_ptr<sf::RenderTexture> lighting;          //!< light map from raycast lights
   std::shared_ptr<sf::RenderTexture> normal;            //!< normal map for lighting calculations
   std::shared_ptr<sf::RenderTexture> normal_tmp;        //!< temporary normal map (before atmosphere distortion)
   std::shared_ptr<sf::RenderTexture> deferred;          //!< final deferred rendering result
   std::shared_ptr<sf::RenderTexture> atmosphere;        //!< atmosphere distortion texture
   std::shared_ptr<sf::RenderTexture> blur;              //!< blur/glow effect texture
   std::shared_ptr<sf::RenderTexture> blur_scaled;       //!< scaled blur texture (for glow effect)

   float view_to_texture_scale = 1.0f;  //!< scale factor from view coordinates to texture coordinates

   /*! \brief create all render textures with given dimensions
    *  \param video_mode_width window width in pixels
    *  \param video_mode_height window height in pixels
    *  \param view_width view width (logical game area)
    *  \param view_height view height (logical game area)
    */
   void create(uint32_t video_mode_width, uint32_t video_mode_height, float view_width, float view_height);

   /*! \brief recreate all render textures after window resize
    *  \param video_mode_width new window width in pixels
    *  \param video_mode_height new window height in pixels
    *  \param view_width view width (logical game area)
    *  \param view_height view height (logical game area)
    */
   void recreateOnResize(uint32_t video_mode_width, uint32_t video_mode_height, float view_width, float view_height);

   /*! \brief get vector of all render textures
    *  \return const reference to vector containing all textures
    */
   const std::vector<std::shared_ptr<sf::RenderTexture>>& getAll() const;

private:
   std::vector<std::shared_ptr<sf::RenderTexture>> _all_textures;  //!< internal storage for iteration
};
