#pragma once

#include <SFML/System/Clock.hpp>

///
/// \brief Provides a singleton clock for global elapsed-time queries.
///
class GlobalClock
{
public:
   ///
   /// \brief Creates the clock and starts measuring time immediately.
   ///
   GlobalClock();

   ///
   /// \brief Returns the single process-wide GlobalClock instance.
   /// \return GlobalClock singleton.
   ///
   static GlobalClock& getInstance();

   ///
   /// \brief Returns elapsed time in milliseconds since clock creation.
   /// \return Elapsed milliseconds.
   ///
   int getElapsedTimeInMs();

   ///
   /// \brief Returns elapsed time in seconds since clock creation.
   /// \return Elapsed seconds.
   ///
   float getElapsedTimeInS();

   ///
   /// \brief Returns SFML time since clock creation.
   /// \return Elapsed `sf::Time`.
   ///
   sf::Time getElapsedTime();

private:
   sf::Clock _clock;
};
