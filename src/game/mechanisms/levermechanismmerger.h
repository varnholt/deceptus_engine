#ifndef LEVERMECHANISMMERGER_H
#define LEVERMECHANISMMERGER_H

#include <vector>

#include "framework/tmxparser/tmxobject.h"
#include "game/mechanisms/gamemechanism.h"

namespace LeverMechanismMerger
{

void addSearchRect(const std::shared_ptr<TmxObject>& rect);

// requires a unified datastructure/mechanism in the future!
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
};

#endif  // LEVERMECHANISMMERGER_H
