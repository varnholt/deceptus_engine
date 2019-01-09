#include "tmxtools.h"

#include <algorithm>
#include <cctype>
#include <locale>
#include <sstream>

TmxTools::TmxTools()
{

}


std::vector<std::string> TmxTools::split(std::string points, char splitChar)
{
   std::vector<std::string> pairs;
   std::istringstream parseStream(points);
   std::string tmp;
   while (std::getline(parseStream, tmp, splitChar))
   {
      pairs.push_back(tmp);
   }

   return pairs;
}

void TmxTools::ltrim(std::string &s)
{
   s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
      return !std::isspace(ch);
   }));
}

void TmxTools::rtrim(std::string &s)
{
   s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
      return !std::isspace(ch);
   }).base(), s.end());
}

void TmxTools::trim(std::string &s)
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

std::array<int, 4> TmxTools::color(const std::string &c)
{
   int r,g,b,a;
   std::istringstream(c.substr(1,2)) >> std::hex >> a;
   std::istringstream(c.substr(3,2)) >> std::hex >> r;
   std::istringstream(c.substr(5,2)) >> std::hex >> g;
   std::istringstream(c.substr(7,2)) >> std::hex >> b;
   return {r,g,b,a};
}
