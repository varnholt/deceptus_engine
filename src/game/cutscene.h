#pragma once

#include <map>
#include <memory>
#include <string>

#include <SFML/Graphics.hpp>


// the idea is that this class drives 'small levels', as in cutscenes
// in those cutscenes the characters and camera is controlled by the this class

class Cutscene
{
   public:

      Cutscene() = default;

      static void play(const std::string& id);

      static void deserialize();

      void update(const sf::Time& dt);


   private:

      void play();

      static std::map<std::string, std::shared_ptr<Cutscene>> _cutscenes;

      std::string _id;
};

