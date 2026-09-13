#pragma once

#include <cstdint>
#include <string>

#include <SFML/System/Time.hpp>

/// \brief logs the player position on an interval, for scripts driving the game from outside.
///
/// a script has only screenshots to go on otherwise, and a screenshot cannot tell a player who has
/// stopped from one who is walking into a wall. the log line carries the position in tiles and in
/// pixels, the health, and whether a dialogue is up, so two runs can be compared as numbers.
namespace PlayerWatch
{

/// \brief sets how often the player position is logged.
/// \param interval_ms milliseconds between log lines; zero or less turns the watch off.
void setIntervalInMs(int32_t interval_ms);

/// \brief returns the interval the watch is running at.
/// \return milliseconds between log lines, or 0 when the watch is off.
int32_t getIntervalInMs();

/// \brief describes the current watch state for the console log.
/// \return human readable state, e.g. "player position watch: every 100ms".
std::string describe();

/// \brief writes a log line when the interval has elapsed.
/// \param delta_time frame time used to space out the log lines.
void update(const sf::Time& delta_time);

}  // namespace PlayerWatch
