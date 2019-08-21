#pragma once

#include <SFML/Graphics.hpp>
#include <cassert>
#include <string>


class Effect : public sf::Drawable
{

public:

  virtual ~Effect() override = default;

  const std::string& getName() const;

  void load();

  void update(const sf::Time& time, float x, float y);

  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;


protected:

  Effect(const std::string& name);


private:

  // Virtual functions to be implemented in derived effects
  virtual bool onLoad() = 0;
  virtual void onUpdate(const sf::Time& time, float x, float y) = 0;
  virtual void onDraw(sf::RenderTarget& target, sf::RenderStates states) const = 0;


private:

  std::string mName;
  bool mIsLoaded = false;
};

