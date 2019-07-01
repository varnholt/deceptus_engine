#include "console.h"

#include "player.h"

#include <iostream>
#include <sstream>


Console Console::mConsole;


bool Console::isActive() const
{
   return mActive;
}


void Console::setActive(bool active)
{
   mActive = active;
}


void Console::append(char c)
{
   mCommand.push_back(c);
}


void Console::chop()
{
   if (mCommand.empty())
   {
      return;
   }

   mCommand.pop_back();
}


void Console::execute()
{
   std::cout << "process command: " << mCommand << std::endl;

   mActive = false;

   // parse command
   std::istringstream iss(mCommand);
   std::vector<std::string> results((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());
   mCommand.clear();

   if (results.empty())
   {
      return;
   }

   if (results.at(0) == "/tp" && results.size() == 3)
   {
      auto x = std::atoi(results.at(1).c_str());
      auto y = std::atoi(results.at(2).c_str());
      std::cout << "teleport to " << x << ", " <<  y << std::endl;

      Player::getPlayer(0)->setBodyViaPixelPosition(x * TILE_WIDTH, y * TILE_HEIGHT);
   }
}


Console& Console::getInstance()
{
   return mConsole;
}


std::string Console::getCommand() const
{
   return mCommand;
}
