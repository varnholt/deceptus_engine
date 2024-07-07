#include "console.h"

#include "framework/tools/log.h"
#include "game/bow.h"
#include "game/eventserializer.h"
#include "game/level.h"
#include "game/mechanisms/checkpoint.h"
#include "game/player/player.h"
#include "game/player/playerinfo.h"
#include "game/savestate.h"
#include "game/tweaks.h"
#include "game/weaponfactory.h"
#include "game/weaponsystem.h"

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
   _log.emplace_back("help:");
   _log.emplace_back("");
   _log.emplace_back("cp <n>: jump to checkpoint");
   _log.emplace_back("   example: cp 0");
   _log.emplace_back("");
   _log.emplace_back("cpanlimitoff: disable cpan maximum radius");
   _log.emplace_back("   example: cpanlimitoff");
   _log.emplace_back("");
   _log.emplace_back("damage <n>: cause damage to player");
   _log.emplace_back("   example: damage 100");
   _log.emplace_back("");
   _log.emplace_back("extra <name>: give extra to player");
   _log.emplace_back("   available extras: climb, dash, wallslide, walljump, doublejump, invulnerable, crouch, all, none");
   _log.emplace_back("");
   _log.emplace_back("give <item name>: give item to player");
   _log.emplace_back("   example: give key_skull");
   _log.emplace_back("");
   _log.emplace_back("playback <command>: game playback");
   _log.emplace_back("   commands: enable, disable, load, save, replay, reset");
   _log.emplace_back("");
   _log.emplace_back("take <item name>: take item from player");
   _log.emplace_back("   example: take key_skull");
   _log.emplace_back("");
   _log.emplace_back("tp <x>,<y>: teleport to position");
   _log.emplace_back("   example: tp 100, 330");
   _log.emplace_back("");
   _log.emplace_back("start: go to start position");
   _log.emplace_back("");
   _log.emplace_back("weapon <weapon>: give weapon to player");
   _log.emplace_back("   available weapons: bow, gun, sword");
}

namespace
{
void giveWeaponToPlayer(const std::shared_ptr<Weapon>& weapon)
{
   Player::getCurrent()->getWeaponSystem()->_weapons.push_back(weapon);
   Player::getCurrent()->getWeaponSystem()->_selected = weapon;
}
}  // namespace

void Console::giveWeaponBow()
{
   auto bow = WeaponFactory::create(WeaponType::Bow);
   bow->initialize();
   std::dynamic_pointer_cast<Bow>(bow)->setLauncherBody(Player::getCurrent()->getBody());
   giveWeaponToPlayer(bow);
   _log.emplace_back("given bow to player");
}

void Console::giveWeaponGun()
{
   auto gun = WeaponFactory::create(WeaponType::Gun);
   gun->initialize();
   giveWeaponToPlayer(gun);
   _log.emplace_back("given gun to player");
}

void Console::giveWeaponSword()
{
   auto sword = WeaponFactory::create(WeaponType::Sword);
   sword->initialize();
   giveWeaponToPlayer(sword);
   _log.emplace_back("given sword to player");
}

void Console::execute()
{
   Log::Info() << "process command: " << _command;

   // parse command
   std::istringstream iss(_command);
   std::vector<std::string> results((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

   if (results.empty())
   {
      return;
   }

   _log.push_back(_command);

   if (results.at(0) == "help")
   {
      showHelp();
   }
   else if (results.at(0) == "weapon" && results.size() == 2)
   {
      if (results.at(1) == "gun")
      {
         giveWeaponGun();
      }
      else if (results.at(1) == "bow")
      {
         giveWeaponBow();
      }
      else if (results.at(1) == "sword")
      {
         giveWeaponSword();
      }
   }
   else if (results.at(0) == "extra" && results.size() == 2)
   {
      auto& skills = SaveState::getPlayerInfo()._extra_table._skills._skills;
      if (results.at(1) == "climb")
      {
         skills |= static_cast<int32_t>(Skill::SkillType::WallClimb);
         _log.emplace_back("given climb extra to player");
      }
      if (results.at(1) == "crouch")
      {
         skills |= static_cast<int32_t>(Skill::SkillType::Crouch);
         _log.emplace_back("given crouch extra to player");
      }
      else if (results.at(1) == "dash")
      {
         skills |= static_cast<int32_t>(Skill::SkillType::Dash);
         _log.emplace_back("given dash extra to player");
      }
      else if (results.at(1) == "wallslide")
      {
         skills |= static_cast<int32_t>(Skill::SkillType::WallSlide);
         _log.emplace_back("given wallslide extra to player");
      }
      else if (results.at(1) == "walljump")
      {
         skills |= static_cast<int32_t>(Skill::SkillType::WallJump);
         _log.emplace_back("given walljump extra to player");
      }
      else if (results.at(1) == "doublejump")
      {
         skills |= static_cast<int32_t>(Skill::SkillType::DoubleJump);
         _log.emplace_back("given doublejump extra to player");
      }
      else if (results.at(1) == "invulnerable")
      {
         skills |= static_cast<int32_t>(Skill::SkillType::Invulnerable);
         _log.emplace_back("given invulnerable extra to player");
      }
      else if (results.at(1) == "all")
      {
         skills = 0xffffffff;
         _log.emplace_back("given all extras to player");
      }
      else if (results.at(1) == "none")
      {
         skills = 0;
         _log.emplace_back("reset all player extras");
      }
   }
   else if (results.at(0) == "give" && results.size() == 2)
   {
      const auto item = results.at(1);
      SaveState::getPlayerInfo()._inventory.add(item);
      _log.emplace_back("given item to player");
   }
   else if (results.at(0) == "take" && results.size() == 2)
   {
      const auto item = results.at(1);
      SaveState::getPlayerInfo()._inventory.remove(item);
      _log.emplace_back("removed item from player");
   }
   else if (results.at(0) == "tp" && results.size() == 3)
   {
      auto x = std::atoi(results.at(1).c_str());
      auto y = std::atoi(results.at(2).c_str());

      std::ostringstream os;
      os << "teleport to " << x << ", " << y << std::endl;
      _log.push_back(os.str());

      Player::getCurrent()->setBodyViaPixelPosition(static_cast<float>(x * PIXELS_PER_TILE), static_cast<float>(y * PIXELS_PER_TILE));
   }
   else if (results.at(0) == "cp" && results.size() == 2)
   {
      const auto checkpoint_index = std::atoi(results.at(1).c_str());

      std::ostringstream os;

      auto checkpoint = Checkpoint::getCheckpoint(checkpoint_index, Level::getCurrentLevel()->getCheckpoints());
      if (checkpoint)
      {
         const auto pos = checkpoint->spawnPoint();
         os << "jumped to checkpoint " << checkpoint_index << std::endl;

         Player::getCurrent()->setBodyViaPixelPosition(static_cast<float>(pos.x), static_cast<float>(pos.y));
      }
      else
      {
         os << "invalid checkpoint " << std::endl;
      }

      _log.push_back(os.str());
   }
   else if (results.at(0) == "cpanlimitoff")
   {
      Tweaks::instance()._cpan_unlimited = true;
      _log.emplace_back("disabled cpan limit");
   }
   else if (results.at(0) == "playback" && results.size() == 2)
   {
      if (results[1] == "enable")
      {
         EventSerializer::getInstance().setEnabled(true);
         _log.emplace_back("playback enabled");
      }
      else if (results[1] == "disable")
      {
         EventSerializer::getInstance().setEnabled(false);
         _log.emplace_back("playback disabled");
      }
      else if (results[1] == "save")
      {
         EventSerializer::getInstance().serialize();
         _log.emplace_back("playback saved");
      }
      else if (results[1] == "load")
      {
         EventSerializer::getInstance().deserialize();
         _log.emplace_back("playback loaded");
      }
      else if (results[1] == "replay")
      {
         EventSerializer::getInstance().play();
         _log.emplace_back("playback started");
      }
      else if (results[1] == "reset")
      {
         EventSerializer::getInstance().clear();
         _log.emplace_back("playback reset");
      }
   }
   else if (results.at(0) == "iddqd")
   {
      SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(Skill::SkillType::Invulnerable);
      _log.emplace_back("invulnerable");
   }
   else if (results.at(0) == "damage" && results.size() == 2)
   {
      const auto damage = std::atoi(results.at(1).c_str());
      Player::getCurrent()->damage(damage);

      std::ostringstream os;
      os << "damage player " << damage << std::endl;
      _log.push_back(os.str());
   }
   else if (results[0] == "start")
   {
      auto* level = Level::getCurrentLevel();
      level->loadStartPosition();
      const auto pos = level->getStartPosition();
      Player::getCurrent()->setBodyViaPixelPosition(static_cast<float>(pos.x), static_cast<float>(pos.y));
   }
   else
   {
      const auto command_it = _registered_commands.find(results.at(0));
      if (command_it != _registered_commands.end())
      {
         command_it->second();
         _log.push_back(command_it->first + " executed");
      }
      else
      {
         std::ostringstream os;
         os << "unknown command: " << _command << std::endl;
         _log.push_back(os.str());
      }
   }

   while (_log.size() > 50)
   {
      _log.pop_front();
   }

   _history.push_back(_command);
   _history_index = static_cast<int32_t>(_history.size());  // n + 1 is intentional
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
   if (_history_index >= static_cast<int32_t>(_history.size()))
   {
      _history_index = static_cast<int32_t>(_history.size() - 1);
   }
   _command = _history[static_cast<size_t>(_history_index)];
}

void Console::registerCallback(const std::string& command, const std::string& description, CommandFunction callback)
{
   _registered_commands[command] = callback;
   _registered_command_help.emplace_back(command, description);
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
