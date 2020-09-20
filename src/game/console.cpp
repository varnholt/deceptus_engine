#include "console.h"

#include "checkpoint.h"
#include "extramanager.h"
#include "player.h"
#include "playerinfo.h"
#include "savestate.h"

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

   if (mActive && mHistory.empty())
   {
       showHelp();
   }
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


void Console::showHelp()
{
    mLog.push_back("help:");
    mLog.push_back("/extra <name> | give extra | available extras: climb, dash");
    mLog.push_back("/tp <x>,<y> | teleport to position | example: /tp 100,330");
    mLog.push_back("/cp <n> | jump to checkpoint | example: /cp 0");
}


void Console::execute()
{
   std::cout << "[i] process command: " << mCommand << std::endl;

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
      showHelp();
   }

   else if (results.at(0) == "/extra" && results.size() == 2)
   {
      auto& skills = SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills;
      if (results.at(1) == "climb")
      {
         skills |= ExtraSkill::SkillWallClimb;
         mLog.push_back("given climb extra to player");
      }
      else if (results.at(1) == "dash")
      {
         skills |= ExtraSkill::SkillDash;
         mLog.push_back("given dash extra to player");
      }
      else if (results.at(1) == "wallslide")
      {
         skills |= ExtraSkill::SkillWallSlide;
         mLog.push_back("given wallslide extra to player");
      }
      else if (results.at(1) == "walljump")
      {
         skills |= ExtraSkill::SkillWallJump;
         mLog.push_back("given walljump extra to player");
      }
      else if (results.at(1) == "doublejump")
      {
         skills |= ExtraSkill::SkillDoubleJump;
         mLog.push_back("given doublejump extra to player");
      }
      else if (results.at(1) == "invulnerable")
      {
         skills |= ExtraSkill::SkillInvulnerable;
         mLog.push_back("given invulnerable extra to player");
      }
      else if (results.at(1) == "all")
      {
         skills = 0xffffffff;
         mLog.push_back("given all extras to player");
      }
   }
   else if (results.at(0) == "/tp" && results.size() == 3)
   {
      auto x = std::atoi(results.at(1).c_str());
      auto y = std::atoi(results.at(2).c_str());

      std::ostringstream os;
      os << "teleport to " << x << ", " <<  y << std::endl;
      mLog.push_back(os.str());

      Player::getCurrent()->setBodyViaPixelPosition(static_cast<float>(x * PIXELS_PER_TILE), static_cast<float>(y * PIXELS_PER_TILE));
   }
   else if (results.at(0) == "/cp" && results.size() == 2)
   {
      auto n = std::atoi(results.at(1).c_str());

      std::ostringstream os;

      auto checkpoint = Checkpoint::getCheckpoint(n);
      if (checkpoint)
      {
         auto pos = checkpoint->calcCenter();
         os << "jumped to checkpoint " << n << std::endl;
         Player::getCurrent()->setBodyViaPixelPosition(pos.x,  pos.y);
      }
      else
      {
         os << "invalid checkpoint " << std::endl;
      }

      mLog.push_back(os.str());
   }
   else if (results.at(0) == "/iddqd")
   {
      SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills |= ExtraSkill::SkillInvulnerable;
      mLog.push_back("invulnerable");
   }
   else if (results.at(0) == "/idkfa")
   {
      SaveState::getPlayerInfo().mInventory.giveAllKeys();
      mLog.push_back("all keys");
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

   mHistory.push_back(mCommand);
   mHistoryIndex = static_cast<int32_t>(mHistory.size()); // n + 1 is intentional
   mCommand.clear();
}


void Console::previousCommand()
{
   if (mHistory.empty())
   {
      return;
   }

   mHistoryIndex--;
   if (mHistoryIndex < 0)
   {
      mHistoryIndex = 0;
   }
   mCommand = mHistory[static_cast<size_t>(mHistoryIndex)];
}


void Console::nextCommand()
{
   if (mHistory.empty())
   {
      return;
   }

   mHistoryIndex++;
   if (mHistoryIndex == static_cast<int32_t>(mHistory.size()))
   {
      mHistoryIndex = static_cast<int32_t>(mHistory.size() - 1);
   }
   mCommand = mHistory[static_cast<size_t>(mHistoryIndex)];
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

