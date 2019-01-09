#include "bitmapfont.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <stdio.h>


BitmapFont::BitmapFont()
{
}


void BitmapFont::load(
   const std::string& texturePath,
   const std::string& mapPath
)
{
   mTexture.loadFromFile(texturePath);
   mSprite.setTexture(mTexture);

   std::ifstream file(mapPath);

   auto i = 0;
   std::string line;
   std::string font;
   std::vector<int> vals;
   while (std::getline(file, line))
   {
      if (i == 0)
      {
         auto tmp = 0;
         std::stringstream ss(line);
         while (ss >> tmp)
         {
            vals.push_back(tmp);
            if (ss.peek() == ',' || ss.peek() == ';')
            {
               ss.ignore();
            }
         }
      }
      else
      {
         font += line;
      }
      i++;
   }

   if (vals.size() == 2)
   {
      mCharWidth = vals[0];
      mCharHeight = vals[1];
   }
   else
   {
      printf("font map fucked.\n");
      return;
   }

   i = 0;
   auto x = 0;
   auto y = 0;
   for (auto c : font)
   {
      std::shared_ptr<sf::IntRect> rect = std::make_shared<sf::IntRect>();
      rect->left = x;
      rect->top = y;
      rect->width = mCharWidth;
      rect->height = mCharHeight;
      mMap.insert(std::pair<char,std::shared_ptr<sf::IntRect>>(c, rect));

      x += mCharWidth;

      if (x == mTexture.getSize().x)
      {
         x = 0;
         y += mCharHeight;
      }

      i++;
   }
   // printf("loaded font: %s\n", font.c_str());
}


std::vector<std::shared_ptr<sf::IntRect>> BitmapFont::getCoords(const std::string &text)
{
   std::vector<std::shared_ptr<sf::IntRect>> coords;

   for (auto c : text)
   {
      auto it = mMap.find(c);

      if (it != mMap.end())
      {
        coords.push_back(it->second);
      }
   }

   return coords;
}


void BitmapFont::draw(
   sf::RenderTarget& window,
   const std::vector<std::shared_ptr<sf::IntRect> > &coords,
   int x,
   int y
)
{
   auto xOffset = 0;
   for (auto coord : coords)
   {
      mSprite.setTextureRect(
         sf::IntRect(
            coord->left,
            coord->top,
            coord->width,
            coord->height
         )
      );

      mSprite.setPosition(
         static_cast<float_t>(x + xOffset),
         static_cast<float_t>(y)
      );

      window.draw(mSprite);
      xOffset += mCharWidth;
   }
}
