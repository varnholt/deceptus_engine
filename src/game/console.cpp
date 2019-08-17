#include "console.h"

#include "player.h"

#include <iostream>
#include <ostream>
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

   // not sure what's the best behavior, probably just staying active until deactivated
   // mActive = false;

   // parse command
   std::istringstream iss(mCommand);
   std::vector<std::string> results((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

   if (results.empty())
   {
      return;
   }

   mLog.push_back(mCommand);

   if (results.at(0) == "/help")
   {
      mLog.push_back("help:");
      mLog.push_back("/extra <name> | give extra | available extras: climb");
      mLog.push_back("/tp <x>,<y> | teleport to position | example: /tp 100,330");
   }

   else if (results.at(0) == "/extra" && results.size() == 2)
   {
      if (results.at(1) == "climb")
      {
         Player::getPlayer(0)->updateClimb();
         mLog.push_back("given climb extra to player");
      }
   }

   else if (results.at(0) == "/tp" && results.size() == 3)
   {
      auto x = std::atoi(results.at(1).c_str());
      auto y = std::atoi(results.at(2).c_str());

      std::ostringstream os;
      os << "teleport to " << x << ", " <<  y << std::endl;
      mLog.push_back(os.str());

      Player::getPlayer(0)->setBodyViaPixelPosition(static_cast<float>(x * PIXELS_PER_TILE), static_cast<float>(y * PIXELS_PER_TILE));
   }
   else
   {
      std::ostringstream os;
      os << "unknown command: " << mCommand << std::endl;
      mLog.push_back(os.str());
   }

   while (mLog.size() > 20)
   {
      mLog.pop_front();
   }

   mCommand.clear();
}


Console& Console::getInstance()
{
   return mConsole;
}


const std::string& Console::getCommand() const
{
   return mCommand;
}


const std::deque<std::string>& Console::getLog() const
{
   return mLog;
}

