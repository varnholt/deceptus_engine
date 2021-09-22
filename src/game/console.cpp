#include "console.h"

#include "bow.h"
#include "checkpoint.h"
#include "eventserializer.h"
#include "extramanager.h"
#include "player/player.h"
#include "player/playerinfo.h"
#include "savestate.h"
#include "weaponfactory.h"
#include "weaponsystem.h"

#include <iostream>
#include <ostream>
#include <sstream>


bool Console::isActive() const
{
   return _active;
}


void Console::setActive(bool active)
{
   _active = active;

   if (_active && _log.empty())
   {
      showHelp();
   }
}


void Console::append(char c)
{
   _command.push_back(c);
}


void Console::chop()
{
   if (_command.empty())
   {
      return;
   }

   _command.pop_back();
}


void Console::showHelp()
{
    _log.push_back("help:");
    _log.push_back("");
    _log.push_back("/cp <n>: jump to checkpoint");
    _log.push_back("   example: /cp 0");
    _log.push_back("");
    _log.push_back("/extra <name>: give extra");
    _log.push_back("   available extras: climb, dash, wallslide, walljump, doublejump, invulnerable, crouch, all");
    _log.push_back("");
    _log.push_back("/playback <command>: game playback");
    _log.push_back("   commands: enable, disable, load, save, replay, reset");
    _log.push_back("");
    _log.push_back("/tp <x>,<y>: teleport to position");
    _log.push_back("   example: /tp 100, 330");
    _log.push_back("");
    _log.push_back("/weapon <weapon>: give weapon to player");
    _log.push_back("   available weapons: bow");
}


void Console::giveWeaponBow()
{
   auto bow = std::make_shared<Bow>();
   bow->initialize();
   bow->setLauncherBody(Player::getCurrent()->getBody());
   Player::getCurrent()->getWeaponSystem()->_weapons.push_back(bow);
   Player::getCurrent()->getWeaponSystem()->_selected = bow;
   _log.push_back("given bow to player");
}


void Console::giveWeaponDefault()
{
   auto weapon = std::make_shared<Weapon>();
   weapon->initialize();
   Player::getCurrent()->getWeaponSystem()->_weapons.push_back(weapon);
   Player::getCurrent()->getWeaponSystem()->_selected = weapon;
   _log.push_back("given default weapon to player");
}


void Console::execute()
{
   std::cout << "[i] process command: " << _command << std::endl;

   // not sure what's the best behavior, probably just staying active until deactivated
   // mActive = false;

   // parse command
   std::istringstream iss(_command);
   std::vector<std::string> results((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

   if (results.empty())
   {
      return;
   }

   _log.push_back(_command);

   if (results.at(0) == "/help")
   {
      showHelp();
   }

   if (results.at(0) == "/weapon" && results.size() == 2)
   {
      if (results.at(1) == "default")
      {
         giveWeaponDefault();
      }
      if (results.at(1) == "bow")
      {
         giveWeaponBow();
      }
   }
   else if (results.at(0) == "/extra" && results.size() == 2)
   {
      auto& skills = SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills;
      if (results.at(1) == "climb")
      {
         skills |= ExtraSkill::SkillWallClimb;
         _log.push_back("given climb extra to player");
      }
      if (results.at(1) == "crouch")
      {
         skills |= ExtraSkill::SkillCrouch;
         _log.push_back("given crouch extra to player");
      }
      else if (results.at(1) == "dash")
      {
         skills |= ExtraSkill::SkillDash;
         _log.push_back("given dash extra to player");
      }
      else if (results.at(1) == "wallslide")
      {
         skills |= ExtraSkill::SkillWallSlide;
         _log.push_back("given wallslide extra to player");
      }
      else if (results.at(1) == "walljump")
      {
         skills |= ExtraSkill::SkillWallJump;
         _log.push_back("given walljump extra to player");
      }
      else if (results.at(1) == "doublejump")
      {
         skills |= ExtraSkill::SkillDoubleJump;
         _log.push_back("given doublejump extra to player");
      }
      else if (results.at(1) == "invulnerable")
      {
         skills |= ExtraSkill::SkillInvulnerable;
         _log.push_back("given invulnerable extra to player");
      }
      else if (results.at(1) == "all")
      {
         skills = 0xffffffff;
         _log.push_back("given all extras to player");
      }
   }
   else if (results.at(0) == "/tp" && results.size() == 3)
   {
      auto x = std::atoi(results.at(1).c_str());
      auto y = std::atoi(results.at(2).c_str());

      std::ostringstream os;
      os << "teleport to " << x << ", " <<  y << std::endl;
      _log.push_back(os.str());

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

         Player::getCurrent()->setBodyViaPixelPosition(
            static_cast<float>(pos.x),
            static_cast<float>(pos.y)
         );
      }
      else
      {
         os << "invalid checkpoint " << std::endl;
      }

      _log.push_back(os.str());
   }
   else if (results.at(0) == "/playback" && results.size() == 2)
   {
      if (results[1] == "enable")
      {
         EventSerializer::getInstance().setEnabled(true);
         _log.push_back("playback enabled");
      }
      else if (results[1] == "disable")
      {
         EventSerializer::getInstance().setEnabled(false);
         _log.push_back("playback disabled");
      }
      else if (results[1] == "save")
      {
         EventSerializer::getInstance().serialize();
         _log.push_back("playback saved");
      }
      else if (results[1] == "load")
      {
         EventSerializer::getInstance().deserialize();
         _log.push_back("playback loaded");
      }
      else if (results[1] == "replay")
      {
         EventSerializer::getInstance().play();
         _log.push_back("playback started");
      }
      else if (results[1] == "reset")
      {
         EventSerializer::getInstance().clear();
         _log.push_back("playback reset");
      }
   }
   else if (results.at(0) == "/iddqd")
   {
      SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills |= ExtraSkill::SkillInvulnerable;
      _log.push_back("invulnerable");
   }
   else if (results.at(0) == "/idkfa")
   {
      SaveState::getPlayerInfo().mInventory.giveAllKeys();
      _log.push_back("all keys");
   }
   else
   {
      std::ostringstream os;
      os << "unknown command: " << _command << std::endl;
      _log.push_back(os.str());
   }

   while (_log.size() > 20)
   {
      _log.pop_front();
   }

   _history.push_back(_command);
   _history_index = static_cast<int32_t>(_history.size()); // n + 1 is intentional
   _command.clear();
}


void Console::previousCommand()
{
   if (_history.empty())
   {
      return;
   }

   _history_index--;
   if (_history_index < 0)
   {
      _history_index = 0;
   }
   _command = _history[static_cast<size_t>(_history_index)];
}


void Console::nextCommand()
{
   if (_history.empty())
   {
      return;
   }

   _history_index++;
   if (_history_index == static_cast<int32_t>(_history.size()))
   {
      _history_index = static_cast<int32_t>(_history.size() - 1);
   }
   _command = _history[static_cast<size_t>(_history_index)];
}


Console& Console::getInstance()
{
   static Console __instance;
   return __instance;
}


const std::string& Console::getCommand() const
{
   return _command;
}


const std::deque<std::string>& Console::getLog() const
{
   return _log;
}

