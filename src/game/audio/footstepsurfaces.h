#pragma once

#include <optional>
#include <string>
#include <vector>

/// \brief footstep sample sets, one per walkable surface, loaded once from a JSON file.
namespace FootstepSurfaces
{

/// \brief describes the samples played while the player walks over one surface.
struct Definition
{
   std::string _surface;             //!< surface identifier as it is written in the level data
   float _volume{1.0f};              //!< volume the samples of this surface are played at
   std::vector<std::string> _left;   //!< samples to pick from for the left foot
   std::vector<std::string> _right;  //!< samples to pick from for the right foot

   /// \brief picks one of the samples of the given foot at random.
   /// \param left true for the left foot, false for the right one.
   /// \return sample filename, or std::nullopt when the foot has no sample configured.
   std::optional<std::string> pickSample(bool left) const;
};

/// \brief loads the surface definitions and registers all their samples with the audio system.
///
/// safe to call multiple times; only the first call reads the file.
/// \param filename path to the footsteps JSON definition file.
void loadDefinitions(const std::string& filename);

/// \brief returns the definition for a given surface when found.
/// \param surface surface identifier to look up.
/// \return matching definition, or std::nullopt when the surface is not configured.
std::optional<Definition> findDefinition(const std::string& surface);

/// \brief returns the surface used for levels that do not name one.
/// \return default surface identifier as read from the JSON file.
const std::string& getDefaultSurface();

}  // namespace FootstepSurfaces
