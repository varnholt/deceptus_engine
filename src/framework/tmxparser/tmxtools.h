#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>


namespace TmxTools
{
   std::vector<std::string> split(const std::string& points, char splitChar);

   std::array<std::string, 2> splitPair(const std::string& points, char splitChar);

   void ltrim(std::string &s);

   // trim from end (in place)
   void rtrim(std::string &s);

   // trim from both ends (in place)
   void trim(std::string &s);

   // trim from start (copying)
   std::string ltrim_copy(std::string s);

   // trim from end (copying)
   std::string rtrim_copy(std::string s);

   // trim from both ends (copying)
   std::string trim_copy(std::string s);

   // get color from string
   std::array<uint8_t, 4> color(const std::string& c);
}

