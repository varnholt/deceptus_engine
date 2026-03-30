#ifndef LEVERMECHANISMMERGER_H
#define LEVERMECHANISMMERGER_H

#include <vector>

#include "framework/tmxparser/tmxobject.h"
#include "game/mechanisms/gamemechanism.h"

namespace LeverMechanismMerger
{

/// \brief stores a legacy TMX search rectangle for rectangle-based lever linking.
/// \param rect TMX rectangle object defining a search area.
void addSearchRect(const std::shared_ptr<TmxObject>& rect);

// requires a unified datastructure/mechanism in the future!
/// \brief connects levers to target mechanisms using target ids and legacy rectangle matching.
/// \param levers lever mechanism list.
/// \param lasers laser mechanism list.
/// \param platforms moving-platform mechanism list.
/// \param fans fan mechanism list.
/// \param belts conveyor-belt mechanism list.
/// \param spikes spikes mechanism list.
/// \param spike_blocks spike-block mechanism list.
/// \param on_off_blocks on-off-block mechanism list.
/// \param rotating_blades rotating-blade mechanism list.
/// \param doors door mechanism list.
void merge(
   const std::vector<std::shared_ptr<GameMechanism>>& levers,
   const std::vector<std::shared_ptr<GameMechanism>>& lasers,
   const std::vector<std::shared_ptr<GameMechanism>>& platforms,
   const std::vector<std::shared_ptr<GameMechanism>>& fans,
   const std::vector<std::shared_ptr<GameMechanism>>& belts,
   const std::vector<std::shared_ptr<GameMechanism>>& spikes,
   const std::vector<std::shared_ptr<GameMechanism>>& spike_blocks,
   const std::vector<std::shared_ptr<GameMechanism>>& on_off_blocks,
   const std::vector<std::shared_ptr<GameMechanism>>& rotating_blades,
   const std::vector<std::shared_ptr<GameMechanism>>& doors
);
};  // namespace LeverMechanismMerger

#endif  // LEVERMECHANISMMERGER_H
