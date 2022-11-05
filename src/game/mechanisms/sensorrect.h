#pragma once

#include "gamedeserializedata.h"
#include "gamemechanism.h"
#include "gamenode.h"

#include "SFML/Graphics.hpp"

class SensorRect : public GameMechanism, public GameNode
{

public:

   SensorRect(GameNode* parent = nullptr);;

   enum class Action
   {
      Enable,
      Disable,
      Toggle,
   };

   enum class Event
   {
      OnEnter,
      OnLeave,
   };

   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   void setup(const GameDeserializeData& data);
   void findReference(const std::vector<std::shared_ptr<GameMechanism>>& mechanisms);


private:

   void action();

   sf::FloatRect _rect;
   bool _player_intersects = false;
   Event _event = Event::OnEnter;
   Action _action = Action::Enable;
   std::string _reference_id;
   std::vector<std::shared_ptr<GameMechanism>> _references;
};

