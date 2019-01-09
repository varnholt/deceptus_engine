#ifndef ACTIONCONTROLLERMAP_H
#define ACTIONCONTROLLERMAP_H

#include "constants.h"
#include <array>
#include <map>
#include <vector>
#include <SDL/include/SDL.h>


struct ActionControllerMap
{
  ActionControllerMap();

  PlayerAction getAction(
    SDL_GameControllerButton button1,
    SDL_GameControllerButton button2 = SDL_CONTROLLER_BUTTON_INVALID,
    SDL_GameControllerButton button3 = SDL_CONTROLLER_BUTTON_INVALID,
    SDL_GameControllerButton button4 = SDL_CONTROLLER_BUTTON_INVALID
  );

  std::array<int, SDL_CONTROLLER_BUTTON_MAX> mButtonMap;
};

#endif // ACTIONCONTROLLERMAP_H

