#pragma once

#include <array>
#include <string>
#include <vector>


class TmxTools
{
public:
   TmxTools();

   static std::vector<std::string> split(std::string points, char splitChar);

   static void ltrim(std::string &s);

   // trim from end (in place)
   static void rtrim(std::string &s);

   // trim from both ends (in place)
   static void trim(std::string &s);

   // trim from start (copying)
   static std::string ltrim_copy(std::string s);

   // trim from end (copying)
   static std::string rtrim_copy(std::string s);

   // trim from both ends (copying)
   static std::string trim_copy(std::string s);

   // get color from string
   static std::array<int, 4> color(const std::string& c);
};

