#include "console.h"

#include "framework/tools/log.h"
#include "game/config/tweaks.h"
#include "game/io/eventserializer.h"
#include "game/level/level.h"
#include "game/mechanisms/checkpoint.h"
#include "game/player/player.h"
#include "game/player/playerinfo.h"
#include "game/player/weaponsystem.h"
#include "game/state/savestate.h"
#include "game/weapons/bow.h"
#include "game/weapons/weaponfactory.h"

#include <iostream>
#include <ostream>
#include <sstream>

Console::Console()
{
   _help.registerCommand("leveldesign", "playback <enable/disable/load/save/replay/reset>: use game playback", {"playback enable"});
   _help.registerCommand("leveldesign", "cpanlimitoff: disable cpan maximum radius");
   _help.registerCommand("teleportation", "tpp <x>,<y>: teleport to tile position", {"tpp 100, 330"});
   _help.registerCommand("teleportation", "tps: teleport to start position");
   _help.registerCommand("teleportation", "tpc <n>: teleport to checkpoint", {"tpc 0"});
   _help.registerCommand(
      "inventory",
      "extra <add/clear> <climb/dash/wallslide/walljump/doublejump/invulnerable/crouch/all>: toggle extras",
      {"extra add doublejump", "extra clear"}
   );
   _help.registerCommand(
      "inventory",
      "item <add/clear/list/remove> <item name>: add/clear/list/remove items",
      {"item add key_skull", "item remove key_skull", "item list", "item clear"}
   );
   _help.registerCommand("inventory", "weapon <add/clear> <sword/bow/gun>: add/clear weapons", {"weapon add sword", "weapon clear"});
   _help.registerCommand("cheats", "damage <n>: cause damage to player", {"damage 100"});
   _help.registerCommand("cheats", "iddqd: make player invulnerable");
   _help.registerCommand("cheats", "pgravity <gravity>: set player gravity scale", {"pgravity 0.1"});
}

bool Console::isActive() const
{
   return _active;
}

void Console::setActive(bool active)
{
   _active = active;
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

namespace
{
void giveWeaponToPlayer(const std::shared_ptr<Weapon>& weapon)
{
   auto& weapons = SaveState::getPlayerInfo()._weapons;
   weapons._weapons.push_back(weapon);
   weapons._selected = weapon;
}
}  // namespace

void Console::giveWeaponBow()
{
   auto bow = WeaponFactory::create(WeaponType::Bow);
   std::dynamic_pointer_cast<Bow>(bow)->setLauncherBody(Player::getCurrent()->getBody());
   giveWeaponToPlayer(bow);
}

void Console::giveWeaponGun()
{
   giveWeaponToPlayer(WeaponFactory::create(WeaponType::Gun));
}

void Console::giveWeaponSword()
{
   giveWeaponToPlayer(WeaponFactory::create(WeaponType::Sword));
}

void Console::teleportToStartPosition()
{
   auto* level = Level::getCurrentLevel();
   level->loadStartPosition();
   const auto pos_px = level->getStartPosition();
   Player::getCurrent()->setBodyViaPixelPosition(static_cast<float>(pos_px.x), static_cast<float>(pos_px.y));
}

void Console::teleportToCheckpoint(int32_t checkpoint_index)
{
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

void Console::teleportToTile(int32_t x_tl, int32_t y_tl)
{
   std::ostringstream os;
   os << "teleport to " << x_tl << ", " << y_tl << std::endl;
   _log.push_back(os.str());

   Player::getCurrent()->setBodyViaPixelPosition(static_cast<float>(x_tl * PIXELS_PER_TILE), static_cast<float>(y_tl * PIXELS_PER_TILE));
}

const Console::Help& Console::help() const
{
   return _help;
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

   // weapon system
   if (results.at(0) == "weapon" && results.size() >= 2)
   {
      if (results.at(1) == "add" && results.size() == 3)
      {
         if (results.at(2) == "gun")
         {
            giveWeaponGun();
            _log.emplace_back("given gun to player");
         }
         else if (results.at(2) == "bow")
         {
            giveWeaponBow();
            _log.emplace_back("given bow to player");
         }
         else if (results.at(2) == "sword")
         {
            giveWeaponSword();
            _log.emplace_back("given sword to player");
         }
         else
         {
            _log.emplace_back("unknown weapon");
         }
      }
      else if (results.at(1) == "clear")
      {
         SaveState::getPlayerInfo()._weapons._weapons.clear();
         SaveState::getPlayerInfo()._weapons._selected.reset();
         _log.emplace_back("cleared all weapons");
      }
   }

   // extras
   else if (results.at(0) == "extra" && results.size() >= 2)
   {
      if (results.at(1) == "add" && results.size() == 3)
      {
         auto& skills = SaveState::getPlayerInfo()._extra_table._skills._skills;
         if (results.at(2) == "climb")
         {
            skills |= static_cast<int32_t>(Skill::SkillType::WallClimb);
            _log.emplace_back("given climb extra to player");
         }
         if (results.at(2) == "crouch")
         {
            skills |= static_cast<int32_t>(Skill::SkillType::Crouch);
            _log.emplace_back("given crouch extra to player");
         }
         else if (results.at(2) == "dash")
         {
            skills |= static_cast<int32_t>(Skill::SkillType::Dash);
            _log.emplace_back("given dash extra to player");
         }
         else if (results.at(2) == "wallslide")
         {
            skills |= static_cast<int32_t>(Skill::SkillType::WallSlide);
            _log.emplace_back("given wallslide extra to player");
         }
         else if (results.at(2) == "walljump")
         {
            skills |= static_cast<int32_t>(Skill::SkillType::WallJump);
            _log.emplace_back("given walljump extra to player");
         }
         else if (results.at(2) == "doublejump")
         {
            skills |= static_cast<int32_t>(Skill::SkillType::DoubleJump);
            _log.emplace_back("given doublejump extra to player");
         }
         else if (results.at(2) == "invulnerable")
         {
            skills |= static_cast<int32_t>(Skill::SkillType::Invulnerable);
            _log.emplace_back("given invulnerable extra to player");
         }
         else if (results.at(2) == "all")
         {
            skills = 0xffffffff;
            _log.emplace_back("given all extras to player");
         }
      }
      else if (results.at(1) == "clear")
      {
         auto& skills = SaveState::getPlayerInfo()._extra_table._skills._skills;
         skills = 0;
         _log.emplace_back("cleared all player extras");
      }
   }

   // inventory
   else if (results.at(0) == "item" && results.size() >= 2)
   {
      if (results.at(1) == "add" && results.size() == 3)
      {
         const auto item = results.at(2);
         SaveState::getPlayerInfo()._inventory.add(item);
         _log.emplace_back("added item to player");
      }
      else if (results.at(1) == "remove" && results.size() == 3)
      {
         const auto item = results.at(2);
         SaveState::getPlayerInfo()._inventory.remove(item);
         _log.emplace_back("removed item from player");
      }
      else if (results.at(1) == "list")
      {
         for (const auto& item : SaveState::getPlayerInfo()._inventory._items)
         {
            _log.emplace_back(item);
         }
      }
      else if (results.at(1) == "clear")
      {
         SaveState::getPlayerInfo()._inventory._items.clear();
         _log.emplace_back("removed all items");
      }
   }

   // teleportation
   else if (results.at(0) == "tpp" && results.size() == 3)
   {
      const auto x_tl = std::atoi(results.at(1).c_str());
      const auto y_tl = std::atoi(results.at(2).c_str());
      teleportToTile(x_tl, y_tl);
   }
   else if (results.at(0) == "tpc" && results.size() == 2)
   {
      const auto checkpoint_index = std::atoi(results.at(1).c_str());
      teleportToCheckpoint(checkpoint_index);
   }
   else if (results[0] == "tps")
   {
      teleportToStartPosition();
   }

   // playback
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

   // tweaks
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
   else if (results[0] == "pgravity" && results.size() == 2)
   {
      const auto scale = std::atof(results.at(1).c_str());
      Player::getCurrent()->getBody()->SetGravityScale(scale);
      std::ostringstream os;
      os << "player gravity " << scale << std::endl;
      _log.push_back(os.str());
   }
   else if (results.at(0) == "cpanlimitoff")
   {
      Tweaks::instance()._cpan_unlimited = true;
      _log.emplace_back("disabled cpan limit");
   }

   // generic
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

void Console::registerCallback(
   const std::string& command,
   CommandFunction callback,
   const std::string& topic,
   const std::string& description,
   const std::vector<std::string>& examples
)
{
   _registered_commands[command] = callback;
   _help.registerCommand(topic, description, examples);
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

void Console::Help::registerCommand(const std::string& topic, const std::string& description, const std::vector<std::string>& examples)
{
   _help_messages[topic].emplace_back(HelpCommand{description, examples});
}

std::string Console::Help::getFormattedHelp() const
{
   std::ostringstream oss;

   std::vector<std::string> sorted_topics;
   for (const auto& entry : _help_messages)
   {
      sorted_topics.push_back(entry.first);
   }
   std::sort(sorted_topics.begin(), sorted_topics.end());

   for (const auto& topic : sorted_topics)
   {
      oss << topic << ":\n";
      const auto& commands = _help_messages.at(topic);
      for (const auto& command : commands)
      {
         oss << "   " << command.description << "\n";

         for (const auto& example : command.examples)
         {
            oss << "      example: " << example << "\n";
         }
      }
      oss << "\n";
   }

   return oss.str();
}
