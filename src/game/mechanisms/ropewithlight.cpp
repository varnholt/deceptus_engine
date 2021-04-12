#include "ropewithlight.h"

#include "level.h"


RopeWithLight::RopeWithLight(GameNode* parent)
 : Rope(parent)
{
}


void RopeWithLight::draw(sf::RenderTarget& color, sf::RenderTarget& normal)
{
   Rope::draw(color, normal);
}


void RopeWithLight::update(const sf::Time& dt)
{
   Rope::update(dt);

   _light->_pos_m = _chain_elements.back()->GetPosition();
   _light->updateSpritePosition();
}


void RopeWithLight::setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world)
{
   Rope::setup(tmxObject, world);

   // add raycast light
   _light = LightSystem::createLightInstance(nullptr);
   _light->_sprite.setColor(sf::Color(255, 255, 255, 100));
   Level::getCurrentLevel()->getLightSystem()->_lights.push_back(_light);
}
