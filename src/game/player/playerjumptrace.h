#ifndef PLAYERJUMPTRACE_H
#define PLAYERJUMPTRACE_H

#include <SFML/Graphics.hpp>

struct PlayerJumpTrace
{
   bool _jump_started = false;
   sf::Time _jump_start_time;
   float _jump_start_y = 0.0f;
   float _jump_epsilon = 0.00001f;
   float _jump_prev_y = 0.0f;
};

#endif // PLAYERJUMPTRACE_H
