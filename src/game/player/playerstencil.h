#ifndef PLAYERSTENCIL_H
#define PLAYERSTENCIL_H

#include <cstdint>
#include <memory>

namespace sf
{
class RenderTarget;
class RenderTexture;
}  // namespace sf

namespace PlayerStencil
{
/// \brief returns the layer where stencil capture starts.
/// \return z-layer directly above the player layer.
int32_t getStartLayer();

/// \brief returns the layer where the player stencil mask is applied.
/// \return z-layer directly below the maximum foreground layer.
int32_t getStopLayer();

/// \brief reports whether a layer should bypass stencil handling.
/// \param z_index render layer being queried.
/// \return false for all layers in the current implementation.
bool isIgnored(int32_t z_index);

/// \brief dumps the stencil buffer to a debug image every 60 calls.
/// \param texture render texture that provides output dimensions for the dump.
void dump(const std::shared_ptr<sf::RenderTexture>& texture);
};  // namespace PlayerStencil

#endif  // PLAYERSTENCIL_H
