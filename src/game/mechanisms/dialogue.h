#pragma once

#include "gamemechanism.h"

#include <vector>
#include <string>
#include <memory>

#include <SFML/Graphics.hpp>

#include "constants.h"

struct TmxObject;

class Dialogue : public GameMechanism
{

public:

   struct DialogueItem
   {
      std::string mTitle;
      std::string mMessage;
      sf::Color mTextColor = sf::Color{232, 219, 243};
      sf::Color mBackgroundColor = sf::Color{47, 12, 75};
      MessageBoxLocation mLocation = MessageBoxLocation::MiddleCenter;
   };

   Dialogue() = default;
   static std::shared_ptr<Dialogue> deserialize(TmxObject* tmxObject);

   void update(const sf::Time& dt) override;

   bool isActive() const;
   void setActive(bool active);
   void showNext();

private:

   void replaceTags(std::string& str);
   void replace(std::string& str, const std::string& what, const std::string& with);

   std::vector<DialogueItem> _dialogue_items;
   uint32_t _index = 0;

   sf::IntRect _pixel_rect;
   bool _repeated = false;
   bool _played = false;
   bool _active = false;
};


