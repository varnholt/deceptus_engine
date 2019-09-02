#include "checkpoint.h"

#include "player.h"

#include "tmxparser/tmxobject.h"


std::vector<Checkpoint> Checkpoint::sCheckpoints;


Checkpoint* Checkpoint::getCheckpoint(int32_t index)
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


void Checkpoint::add(TmxObject* tmxObject)
{
    Checkpoint cp;

    sCheckpoints.push_back(cp);
}


void Checkpoint::update()
{
    auto playerRect = Player::getPlayer(0)->getPlayerPixelRect();

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
    // check if level is completed
    if (mIndex == static_cast<int32_t>(sCheckpoints.size()) - 1)
    {
        if (mCallback != nullptr)
        {
            mCallback();
        }
    }
}
