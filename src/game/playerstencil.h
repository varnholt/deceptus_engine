#ifndef PLAYERSTENCIL_H
#define PLAYERSTENCIL_H

#include <memory>

namespace sf
{
class RenderTexture;
}

namespace PlayerStencil
{
void clearStencilBuffer();
void setupForeground();
void setupPlayer();
void enable();
void disable();
void dump(const std::shared_ptr<sf::RenderTexture>& texture);
};  // namespace PlayerStencil

#endif  // PLAYERSTENCIL_H
