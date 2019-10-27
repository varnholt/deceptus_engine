#include "moveablebox.h"

#include "texturepool.h"

#include "tmxparser/tmxobject.h"


MoveableBox::MoveableBox(GameNode* node)
 : GameNode(node)
{
   mTexture = TexturePool::getInstance().get("data/level-malte/crypts.png");
}



//-----------------------------------------------------------------------------
void MoveableBox::draw(sf::RenderTarget& window)
{
   for (const auto& sprite : mSprites)
   {
      window.draw(sprite);
   }
}


//-----------------------------------------------------------------------------
void MoveableBox::update(const sf::Time& /*dt*/)
{

}

//-----------------------------------------------------------------------------
void MoveableBox::setup(TmxObject* /*tmxObject*/, const std::shared_ptr<b2World>& /*world*/)
{

}

