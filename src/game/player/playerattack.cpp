#include "playerattack.h"

#include "constants.h"
#include "luainterface.h"
#include "worldquery.h"

#include "Box2D/Box2D.h"

#include <vector>


//      +-----+-----+
//      |     |     |
//      |     |     |
//      +-----+-----+
//      |     |     |
//      |     |     |
//      +-----+-----+

//      +-----+--+
//      |     |__|___
//      |     |  |  |
//      +-----+--+--+
//      |     |     |
//      |     |     |
//      +-----+-----+


namespace
{

sf::Vector2f vecB2S(const b2Vec2 &vector)
{
   return{vector.x * PPM, vector.y * PPM};
}


b2Vec2 vecS2B(const sf::Vector2f& vector)
{
   return{vector.x * MPP, vector.y * MPP};
}


std::vector<b2Body*> retrieveBodiesOnScreen(const std::shared_ptr<b2World>& world, const sf::FloatRect& screen)
{
   b2AABB aabb;

   const auto l = screen.left;
   const auto r = screen.left + screen.width;
   const auto t = screen.top;
   const auto b = screen.top + screen.height;

   aabb.upperBound = vecS2B({std::max(l, r), std::max(b, t)});
   aabb.lowerBound = vecS2B({std::min(l, r), std::min(b, t)});

   return WorldQuery::queryBodies(world, aabb);
}


std::vector<std::shared_ptr<LuaNode>> findNodes(const sf::FloatRect& attack_rect)
{
   const auto& nodes = LuaInterface::instance().getObjectList();

   std::vector<std::shared_ptr<LuaNode>> hit_nodes;
   std::copy_if(nodes.begin(), nodes.end(), std::back_inserter(hit_nodes), [attack_rect](const auto& node){
         const auto& hit_boxes = node->_hit_boxes_px;
         return std::any_of(hit_boxes.begin(), hit_boxes.end(), [attack_rect](const auto& hit_box){
               return hit_box.intersects(attack_rect);
            }
         );
      }
   );

   return hit_nodes;
}


std::vector<std::shared_ptr<LuaNode>> findNodes(const std::vector<sf::FloatRect>& attack_rects)
{
   const auto& nodes = LuaInterface::instance().getObjectList();

   std::vector<std::shared_ptr<LuaNode>> hit_nodes;
   std::copy_if(nodes.begin(), nodes.end(), std::back_inserter(hit_nodes), [attack_rects](const auto& node){
         const auto& hit_boxes = node->_hit_boxes_px;
         return std::any_of(hit_boxes.begin(), hit_boxes.end(), [attack_rects](const auto& hit_box){
               return std::any_of(attack_rects.begin(), attack_rects.end(), [hit_box](const auto& attack_rect){
                     return hit_box.intersects(attack_rect);
                  }
               );
            }
         );
      }
   );

   return hit_nodes;
}

}


void PlayerAttack::attack(const Attack& settings)
{
   auto nodes = findNodes(settings._collision_rect);

   for (auto& node : nodes)
   {
      node->luaHit(settings._damage);
   }
}

