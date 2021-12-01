#include "cutscene.h"

#include "framework/tools/log.h"


std::map<std::string, std::shared_ptr<Cutscene>> Cutscene::_cutscenes;


void Cutscene::play(const std::string& id)
{
   _cutscenes[id]->play();
}


void Cutscene::deserialize()
{
   // load json
}


void Cutscene::update(const sf::Time& /*dt*/)
{
}


void Cutscene::play()
{
   Log::Info() << "playing cutscene " << _id << std::endl;
}
