#ifndef PROJECTILEHITAUDIO_H
#define PROJECTILEHITAUDIO_H

#include <string>
#include <vector>

namespace ProjectileHitAudio
{

/// \brief describes one hit sound sample and its playback volume multiplier.
struct ProjectileHitSample
{
   std::string _sample;
   float _volume = 1.0f;
};

/// \brief returns registered hit samples for a projectile identifier.
/// \param id projectile identifier key.
/// \return copy of the sample list stored for the identifier.
std::vector<ProjectileHitSample> getReferenceSamples(const std::string& id);

/// \brief registers hit sound samples for a projectile identifier.
/// \param id projectile identifier key.
/// \param reference sample list used when this projectile hits something.
void addReferenceSamples(const std::string& id, const std::vector<ProjectileHitSample>& reference);
};  // namespace ProjectileHitAudio

#endif  // PROJECTILEHITAUDIO_H
