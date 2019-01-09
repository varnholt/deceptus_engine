#ifndef EFFECT_HPP
#define EFFECT_HPP

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include <SFML/Graphics.hpp>
#include <cassert>
#include <string>


////////////////////////////////////////////////////////////
// Base class for effects
////////////////////////////////////////////////////////////
class Effect : public sf::Drawable
{

public:

  virtual ~Effect() override
  {
  }

  const std::string& getName() const
  {
    return m_name;
  }

  void load()
  {
    mIsLoaded = /*sf::Shader::isAvailable() && */ onLoad();
  }

  void update(float time, float x, float y)
  {
    if (mIsLoaded)
    {
      onUpdate(time, x, y);
    }
  }

  void draw(sf::RenderTarget& target, sf::RenderStates states) const override
  {
    if (mIsLoaded)
    {
      onDraw(target, states);
    }
  }

protected:

  Effect(const std::string& name) :
  m_name(name),
  mIsLoaded(false)
  {
  }


private:

  // Virtual functions to be implemented in derived effects
  virtual bool onLoad() = 0;
  virtual void onUpdate(float time, float x, float y) = 0;
  virtual void onDraw(sf::RenderTarget& target, sf::RenderStates states) const = 0;


private:

  std::string m_name;
  bool mIsLoaded;
};

#endif // EFFECT_HPP
