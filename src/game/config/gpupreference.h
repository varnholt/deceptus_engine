#pragma once

#include <cstdint>

/// \brief reads and writes the per application GPU preference windows keeps for this executable.
///
/// On a laptop with an integrated and a discrete GPU, windows decides which one a process gets. The
/// executable asks for the discrete one by exporting `NvOptimusEnablement` (see main.cpp), and the
/// preference below overrides that per user, which is what lets the player pick the integrated GPU
/// to save power. The driver reads it while the process is loaded, so a change only takes effect on
/// the next start.
///
/// Every other platform reports the preference as unsupported and the video options hide the row.
namespace GpuPreference
{

/// \brief the GPU a player can ask for.
enum class Preference : int32_t
{
   Automatic = 0,        //!< no preference stored, windows decides (which the exported symbol steers)
   HighPerformance = 1,  //!< the discrete GPU
   PowerSaving = 2       //!< the integrated GPU
};

/// \brief tells whether this platform has a GPU preference to read or write.
/// \return true on windows, false everywhere else.
bool isSupported();

/// \brief reads the preference currently stored for this executable.
/// \return the stored preference, or `Automatic` when none is stored or the platform has none.
Preference read();

/// \brief stores the preference for this executable, removing it again for `Automatic`.
/// \param preference the preference to store.
void write(Preference preference);

}  // namespace GpuPreference
