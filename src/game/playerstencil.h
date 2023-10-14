#ifndef PLAYERSTENCIL_H
#define PLAYERSTENCIL_H

#include <memory>

namespace sf
{
class RenderTexture;
}

namespace PlayerStencil
{
int32_t getStartLayer();
int32_t getStopLayer();
void clearStencilBuffer();
void replaceAllWithOne();
void keepIfOne();
bool isIgnored(int32_t z_index);
void enableTest();
void disableTest();
void dump(const std::shared_ptr<sf::RenderTexture>& texture);
};  // namespace PlayerStencil

#endif  // PLAYERSTENCIL_H
