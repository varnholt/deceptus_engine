#include "tmxtools.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <locale>
#include <sstream>


std::vector<std::string> TmxTools::split(const std::string& points, char splitChar)
{
   std::vector<std::string> elements;
   std::istringstream parseStream(points);
   std::string tmp;
   while (std::getline(parseStream, tmp, splitChar))
   {
      elements.push_back(tmp);
   }

   return elements;
}


void TmxTools::ltrim(std::string& s)
{
   s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
      return !std::isspace(ch);
   }));
}


void TmxTools::rtrim(std::string& s)
{
   s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
      return !std::isspace(ch);
   }).base(), s.end());
}


void TmxTools::trim(std::string& s)
{
   ltrim(s);
   rtrim(s);
}


std::string TmxTools::ltrim_copy(std::string s)
{
   ltrim(s);
   return s;
}


std::string TmxTools::rtrim_copy(std::string s)
{
   rtrim(s);
   return s;
}


std::string TmxTools::trim_copy(std::string s)
{
   trim(s);
   return s;
}


std::array<uint8_t, 4> TmxTools::color(const std::string& c)
{
   uint8_t r, g, b, a;
   uint64_t value = stoul(c.substr(1, 9), nullptr, 16);

   a = (value >> 24) & 0xff;
   r = (value >> 16) & 0xff;
   g = (value >> 8) & 0xff;
   b = (value >> 0) & 0xff;

   return {r,g,b,a};
}


std::array<std::string, 2> TmxTools::splitPair(const std::string& points, char splitChar)
{
   std::array<std::string, 2> pairs;
   std::istringstream parseStream(points);
   std::string tmp;
   auto i = 0;
   while (std::getline(parseStream, tmp, splitChar))
   {
      pairs[i++] = tmp;

      if (i == 2)
      {
         break;
      }
   }

   return pairs;
}
