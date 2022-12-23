#include "projectilehitaudio.h"

#include <map>

namespace
{
std::map<std::string, std::vector<ProjectileHitAudio::ProjectileHitSample>> __reference_animations;
};

void ProjectileHitAudio::addReferenceSamples(const std::string& id, const std::vector<ProjectileHitAudio::ProjectileHitSample>& reference)
{
   __reference_animations[id] = reference;
}

std::vector<ProjectileHitAudio::ProjectileHitSample> ProjectileHitAudio::getReferenceSamples(const std::string& id)
{
   return __reference_animations[id];
}
