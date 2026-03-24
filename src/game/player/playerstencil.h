#ifndef PLAYERSTENCIL_H
#define PLAYERSTENCIL_H

#include <memory>

namespace sf
{
class RenderTarget;
class RenderTexture;
}  // namespace sf

namespace PlayerStencil
{
/// \brief runs stencil setup and player stencil rendering at dedicated z-layers.
/// \param target render target used for player stencil drawing at the stop layer.
/// \param z_index current render layer index in the world draw pass.
void draw(sf::RenderTarget& target, int32_t z_index);
/// \brief returns the layer where stencil capture starts.
/// \return z-layer directly above the player layer.
int32_t getStartLayer();
/// \brief returns the layer where the player stencil mask is applied.
/// \return z-layer directly below the maximum foreground layer.
int32_t getStopLayer();
/// \brief clears the current opengl stencil buffer.
void clearStencilBuffer();
/// \brief configures stencil writes to replace all fragments with value 1.
void replaceAllWithOne();
/// \brief configures stencil testing to keep fragments only where stencil equals 1.
void keepIfOne();
/// \brief reports whether a layer should bypass stencil handling.
/// \param z_index render layer being queried.
/// \return false for all layers in the current implementation.
bool isIgnored(int32_t z_index);
/// \brief enables opengl stencil testing.
void enableTest();
/// \brief disables opengl stencil testing.
void disableTest();
/// \brief dumps the stencil buffer to a debug image every 60 calls.
/// \param texture render texture that provides output dimensions for the dump.
void dump(const std::shared_ptr<sf::RenderTexture>& texture);
};  // namespace PlayerStencil

#endif  // PLAYERSTENCIL_H
