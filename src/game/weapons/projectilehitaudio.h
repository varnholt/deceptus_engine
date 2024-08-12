#ifndef PROJECTILEHITAUDIO_H
#define PROJECTILEHITAUDIO_H

#include <string>
#include <vector>

namespace ProjectileHitAudio
{

struct ProjectileHitSample
{
   std::string _sample;
   float _volume = 1.0f;
};

std::vector<ProjectileHitSample> getReferenceSamples(const std::string& id);
void addReferenceSamples(const std::string& id, const std::vector<ProjectileHitSample>& reference);
};  // namespace ProjectileHitAudio

#endif  // PROJECTILEHITAUDIO_H
