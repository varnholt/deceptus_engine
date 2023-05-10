#pragma once

#include "gamemechanism.h"
#include "gamenode.h"

#include <optional>
#include <memory>
#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "constants.h"
#include "gamedeserializedata.h"

struct TmxObject;

class Dialogue : public GameMechanism, public GameNode
{

public:

   struct DialogueItem
   {
      std::string _title;
      std::string _message;
      sf::Color _text_color = sf::Color{232, 219, 243};
      sf::Color _background_color = sf::Color{47, 12, 75};
      MessageBoxLocation _location = MessageBoxLocation::MiddleCenter;
      std::optional<sf::Vector2i> _pos;
      bool _animate_text = true;
      float _animate_text_speed = 10.0f;
   };

   Dialogue(GameNode* parent = nullptr);
   static std::shared_ptr<Dialogue> deserialize(GameNode* parent, const GameDeserializeData& data);

   void update(const sf::Time& dt) override;
   std::optional<sf::FloatRect> getBoundingBoxPx() override;

   bool isActive() const;
   void setActive(bool active);
   void showNext();

private:

   void replaceTags(std::string& str);
   void replace(std::string& str, const std::string& what, const std::string& with);

   std::vector<DialogueItem> _dialogue_items;
   uint32_t _index = 0;

   sf::FloatRect _pixel_rect;
   bool _repeated = false;
   bool _played = false;
   bool _active = false;
   bool _button_required = true;
   std::optional<int32_t> _consumed_counter;
};


