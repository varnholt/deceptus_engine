#ifndef BITMAPFONT_H
#define BITMAPFONT_H

#include <memory>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>


class BitmapFont
{

public:

   sf::Texture mTexture;
   sf::Sprite mSprite;
   std::map<char, std::shared_ptr<sf::IntRect>> mMap;
   int mCharWidth = 0;
   int mCharHeight = 0;


public:

   BitmapFont();
   void load(const std::string& texture, const std::string &map);
   std::vector<std::shared_ptr<sf::IntRect>> getCoords(const std::string& text);

   void draw(
      sf::RenderTarget& window,
      const std::vector<std::shared_ptr<sf::IntRect>>& coords,
      int x = 0,
      int y = 0
   );
};

#endif // BITMAPFONT_H
