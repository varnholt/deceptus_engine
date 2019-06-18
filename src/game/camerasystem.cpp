#include "camerasystem.h"


/*

Camera system

   Camera X position

      1) dist = distance between old camera x position and the current x player position
      2) dist /= 32
      3) dist = min(dist, max_camera_speed)
      4) have a focus zone which is a rectangle around the player
         if the player walks out of the focus zone, apply the offset
      5) that rectangle is moved further to the left if the player looks to the right and vice versa.
      6) if the player accelerates in a direction that rectangle is moved even further into the opposite direction.


   Camera Y position

      1) dist = distance between old camera y position and the current player y position
      2) dist /= 16
      3) the camera is only moved when the player touches the ground
      4) have two 'panic lines' at the top and bottom of the screen; if the player falls through those lines,
         move the camera

*/


void CameraSystem::update()
{

}


