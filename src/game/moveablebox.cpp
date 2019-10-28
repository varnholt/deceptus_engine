#include "moveablebox.h"

#include "texturepool.h"
#include "tmxparser/tmxobject.h"

#include <iostream>


MoveableBox::MoveableBox(GameNode* node)
 : GameNode(node)
{
   mTexture = TexturePool::getInstance().get("data/level-malte/tilesets/crypts.png");
   mSprite.setTexture(*mTexture.get());
}


//-----------------------------------------------------------------------------
void MoveableBox::draw(sf::RenderTarget& window)
{
   window.draw(mSprite);
}


//-----------------------------------------------------------------------------
void MoveableBox::update(const sf::Time& /*dt*/)
{

}


// box: pos: 5160 x 1056 size: 48 x 48
// box: pos: 5376 x 1080 size: 24 x 24



/*

   +----+
   | __ |
   |//\\|
   +----+
   |####|
   |####|
   +----+

   +----+----+
   | ___|___ |
   |////|\\\\|
   +----+----+
   |####|####|
   |####|####|
   +----+----+
   |####|####|
   |####|####|
   +----+----+


*/

// 1128, 24
// 1176, 0


//-----------------------------------------------------------------------------
void MoveableBox::setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& /*world*/)
{
   std::cout
      << "box: pos: " << tmxObject->mX << " x " << tmxObject->mY
      << " size: " << tmxObject->mWidth << " x " << tmxObject->mHeight
      << std::endl;

   mSize.x = tmxObject->mWidth;
   mSize.y = tmxObject->mHeight;

   mSprite.setPosition(tmxObject->mX, tmxObject->mY - 24);

   switch (static_cast<int32_t>(mSize.x))
   {
      case 24:
      {
         mSprite.setTextureRect(sf::IntRect(1176, 0, 24, 2 * 24));
         break;
      }

      case 48:
      {
         mSprite.setTextureRect(sf::IntRect(1128, 24, 24 * 2, 3 * 24));
         break;
      }

      default:
      {
         break;
      }
   }
}


int32_t MoveableBox::getZ() const
{
   return mZ;
}


