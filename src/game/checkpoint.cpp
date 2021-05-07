#include "checkpoint.h"

#include "framework/tools/callbackmap.h"
#include "player/player.h"

#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxproperty.h"

#include <iostream>


std::vector<Checkpoint> Checkpoint::sCheckpoints;


Checkpoint* Checkpoint::getCheckpoint(uint32_t index)
{
    Checkpoint* cp = nullptr;

    auto it =
       std::find_if(
          sCheckpoints.begin(),
          sCheckpoints.end(),
          [index](const auto& cp) {return cp.mIndex == index;}
       );

    if (it != sCheckpoints.end())
    {
       cp = &(*it);
    }

    return cp;
}


uint32_t Checkpoint::add(TmxObject* tmxObject)
{
   Checkpoint cp;

   cp.mRect = sf::IntRect{
      static_cast<int32_t>(tmxObject->_x_px),
      static_cast<int32_t>(tmxObject->_y_px),
      static_cast<int32_t>(tmxObject->_width_px),
      static_cast<int32_t>(tmxObject->_height_px)
   };

   cp.mName = tmxObject->_name;

   if (tmxObject->_properties)
   {
      auto it = tmxObject->_properties->_map.find("index");
      if (it != tmxObject->_properties->_map.end())
      {
         cp.mIndex = static_cast<uint32_t>(it->second->_value_int.value());
      }
   }

   // std::cout << "registering checkpoint: " << cp.mIndex << std::endl;

   sCheckpoints.push_back(cp);
   return cp.mIndex;
}


void Checkpoint::update()
{
    auto playerRect = Player::getCurrent()->getPlayerPixelRect();

    for (auto& cp : sCheckpoints)
    {
        if (playerRect.intersects(cp.mRect))
        {
            cp.reached();
        }
    }
}


void Checkpoint::resetAll()
{
    sCheckpoints.clear();
}


void Checkpoint::reached()
{
   if (mReached)
   {
      return;
   }

   std::cout << "[-] reached checkpoint: " << mIndex << std::endl;

   mReached = true;

   for (auto& callback : mCallbacks)
   {
      callback();
   }

   // check if level is completed
   if (mName == "end")
   {
      CallbackMap::getInstance().call(CallbackMap::CallbackType::EndGame);
   }
}


void Checkpoint::addCallback(Checkpoint::CheckpointCallback cb)
{
   mCallbacks.push_back(cb);
}


sf::Vector2i Checkpoint::calcCenter() const
{
   // that y offset is a litte dodgy, could have something cleaner in the future
   sf::Vector2i pos{mRect.left + mRect.width / 2, mRect.top + mRect.height - 10};
   return pos;
}
