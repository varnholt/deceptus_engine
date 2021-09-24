#pragma once

#include <SFML/Graphics.hpp>
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

  virtual bool onLoad() = 0;
  virtual void onUpdate(const sf::Time& time, float x, float y) = 0;
  virtual void onDraw(sf::RenderTarget& target, sf::RenderStates states) const = 0;


protected:

  std::string _name;
  bool _is_loaded = false;
};

