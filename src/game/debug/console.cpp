#include "console.h"

#include "framework/tools/log.h"
#include "game/config/tweaks.h"
#include "game/debug/debugdrawstates.h"
#include "game/level/levelregistry.h"
#include "game/level/room.h"
#include "game/mechanisms/checkpoint.h"
#include "game/player/player.h"
#include "game/player/playerinfo.h"
#include "game/player/playerregistry.h"
#include "game/player/weaponsystem.h"
#include "game/state/gamestate.h"
#include "game/state/savestate.h"
#include "game/weapons/bow.h"
#include "game/weapons/weaponfactory.h"

#include <iostream>
#include <ostream>
#include <ranges>
#include <sstream>

namespace
{
void giveWeaponToPlayer(const std::shared_ptr<Weapon>& weapon)
{
   auto& weapons = SaveState::getPlayerInfo()._weapons;
   weapons._weapons.push_back(weapon);
   weapons._selected = weapon;
}
}  // namespace

Console::Console()
{
   // weapon
   _help.registerCommand("inventory", "weapon <add/clear> <sword/bow/gun>: add/clear weapons", {"weapon add sword", "weapon clear"});

   addCommand(
      "weapon add gun",
      [this](const auto&)
      {
         giveWeaponGun();
         _log.emplace_back("given gun to player");
      }
   );

   addCommand(
      "weapon add bow",
      [this](const auto&)
      {
         giveWeaponBow();
         _log.emplace_back("given bow to player");
      }
   );

   addCommand(
      "weapon add sword",
      [this](const auto&)
      {
         giveWeaponSword();
         _log.emplace_back("given sword to player");
      }
   );

   addCommand(
      "weapon clear",
      [this](const auto&)
      {
         SaveState::getPlayerInfo()._weapons._weapons.clear();
         SaveState::getPlayerInfo()._weapons._selected.reset();
         _log.emplace_back("cleared all weapons");
      }
   );

   // extra
   _help.registerCommand(
      "inventory",
      "extra <add/clear> <climb/dash/wallslide/walljump/doublejump/invulnerable/crouch/all>: toggle extras",
      {"extra add doublejump", "extra clear"}
   );

   addCommand(
      "extra add climb",
      [this](const auto&)
      {
         SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(Skill::SkillType::WallClimb);
         _log.emplace_back("given climb extra to player");
      }
   );

   addCommand(
      "extra add crouch",
      [this](const auto&)
      {
         SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(Skill::SkillType::Crouch);
         _log.emplace_back("given crouch extra to player");
      }
   );

   addCommand(
      "extra add dash",
      [this](const auto&)
      {
         SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(Skill::SkillType::Dash);
         _log.emplace_back("given dash extra to player");
      }
   );

   addCommand(
      "extra add wallslide",
      [this](const auto&)
      {
         SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(Skill::SkillType::WallSlide);
         _log.emplace_back("given wallslide extra to player");
      }
   );

   addCommand(
      "extra add walljump",
      [this](const auto&)
      {
         SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(Skill::SkillType::WallJump);
         _log.emplace_back("given walljump extra to player");
      }
   );

   addCommand(
      "extra add doublejump",
      [this](const auto&)
      {
         SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(Skill::SkillType::DoubleJump);
         _log.emplace_back("given doublejump extra to player");
      }
   );

   addCommand(
      "extra add invulnerable",
      [this](const auto&)
      {
         SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(Skill::SkillType::Invulnerable);
         _log.emplace_back("given invulnerable extra to player");
      }
   );

   addCommand(
      "extra add all",
      [this](const auto&)
      {
         SaveState::getPlayerInfo()._extra_table._skills._skills = 0xffffffff;
         _log.emplace_back("given all extras to player");
      }
   );

   addCommand(
      "extra clear",
      [this](const auto&)
      {
         SaveState::getPlayerInfo()._extra_table._skills._skills = 0;
         _log.emplace_back("cleared all player extras");
      }
   );

   // item
   _help.registerCommand(
      "inventory",
      "item <add/clear/list/listall/remove> <item name>: add/clear/list/remove items",
      {"item add key_skull", "item remove key_skull", "item list", "item clear", "item listall"}
   );

   addCommand(
      "item add",
      [this](const auto& args)
      {
         if (args.size() == 3)
         {
            const auto& item_name = args.at(2);
            const auto known_names = SaveState::getPlayerInfo()._inventory.readItemNames();
            if (std::ranges::find(known_names, item_name) == known_names.end())
            {
               _log.emplace_back("unknown item: " + item_name);
               return;
            }
            SaveState::getPlayerInfo()._inventory.add(item_name);
            _log.emplace_back("added item to player");
         }
      }
   );

   addCommand(
      "item remove",
      [this](const auto& args)
      {
         if (args.size() == 3)
         {
            SaveState::getPlayerInfo()._inventory.remove(args.at(2));
            _log.emplace_back("removed item from player");
         }
      }
   );

   addCommand(
      "item list",
      [this](const auto&)
      {
         for (const auto& item : SaveState::getPlayerInfo()._inventory._items)
         {
            _log.emplace_back(item);
         }
      }
   );

   addCommand(
      "item listall",
      [this](const auto&)
      {
         for (const auto& name : SaveState::getPlayerInfo()._inventory.readItemNames())
         {
            _log.emplace_back(name);
         }
      }
   );

   addCommand(
      "item clear",
      [this](const auto&)
      {
         SaveState::getPlayerInfo()._inventory.clear();
         _log.emplace_back("removed all items");
      }
   );

   // teleportation
   registerCallback("tps", [this](const auto&) { teleportToStartPosition(); }, "teleportation", "tps: teleport to start position");

   registerCallback(
      "tpp",
      [this](const auto& args)
      {
         if (args.size() == 3)
         {
            teleportToTile(std::atoi(args.at(1).c_str()), std::atoi(args.at(2).c_str()));
         }
      },
      "teleportation",
      "tpp <x>,<y>: teleport to tile position",
      {"tpp 100, 330"}
   );

   registerCallback(
      "tpc",
      [this](const auto& args)
      {
         if (args.size() == 2)
         {
            teleportToCheckpoint(std::atoi(args.at(1).c_str()));
         }
      },
      "teleportation",
      "tpc <n>: teleport to checkpoint",
      {"tpc 0"}
   );

   registerCallback(
      "tpr",
      [this](const auto& args)
      {
         if (args.size() == 2)
         {
            teleportToRoom(args.at(1));
         }
      },
      "teleportation",
      "tpr <name>: teleport to room by name",
      {"tpr my_room"}
   );

   // playback
   _help.registerCommand("leveldesign", "playback <enable/disable/load/save/replay/reset>: use game playback", {"playback enable"});
   addCommand(
      "playback enable",
      [this](const auto&)
      {
         EventSerializer::getInstance("player")->setEnabled(true);
         _log.emplace_back("playback enabled");
      }
   );

   addCommand(
      "playback disable",
      [this](const auto&)
      {
         EventSerializer::getInstance("player")->setEnabled(false);
         _log.emplace_back("playback disabled");
      }
   );

   addCommand(
      "playback save",
      [this](const auto&)
      {
         EventSerializer::getInstance("player")->serialize();
         _log.emplace_back("playback saved");
      }
   );

   addCommand(
      "playback load",
      [this](const auto&)
      {
         EventSerializer::getInstance("player")->deserialize();
         _log.emplace_back("playback loaded");
      }
   );

   addCommand(
      "playback replay",
      [this](const auto&)
      {
         EventSerializer::getInstance("player")->play();
         _log.emplace_back("playback started");
      }
   );

   addCommand(
      "playback reset",
      [this](const auto&)
      {
         EventSerializer::getInstance("player")->clear();
         _log.emplace_back("playback reset");
      }
   );

   // global playback
   _help.registerCommand(
      "leveldesign", "globalplayback <enable/disable/load/save/replay/reset>: use global game playback", {"globalplayback enable"}
   );

   addCommand(
      "globalplayback enable",
      [this](const auto&)
      {
         EventSerializer::getInstance("global")->setEnabled(true);
         _log.emplace_back("global playback enabled");
      }
   );

   addCommand(
      "globalplayback disable",
      [this](const auto&)
      {
         EventSerializer::getInstance("global")->setEnabled(false);
         _log.emplace_back("global playback disabled");
      }
   );

   addCommand(
      "globalplayback save",
      [this](const auto&)
      {
         EventSerializer::getInstance("global")->serialize();
         _log.emplace_back("global playback saved");
      }
   );

   addCommand(
      "globalplayback load",
      [this](const auto&)
      {
         EventSerializer::getInstance("global")->deserialize();
         _log.emplace_back("global playback loaded");
      }
   );

   addCommand(
      "globalplayback replay",
      [this](const auto&)
      {
         EventSerializer::getInstance("global")->play();
         _log.emplace_back("global playback started");
      }
   );

   addCommand(
      "globalplayback reset",
      [this](const auto&)
      {
         EventSerializer::getInstance("global")->clear();
         _log.emplace_back("global playback reset");
      }
   );

   // playerlight
   _help.registerCommand(
      "leveldesign",
      "playerlight <enable/disable/alpha>: toggle or set player light intensity",
      {"playerlight enable", "playerlight disable", "playerlight alpha 100"}
   );

   addCommand(
      "playerlight enable",
      [this](const auto&)
      {
         auto level = LevelRegistry::getCurrent();
         if (level && level->getPlayerLight())
         {
            level->getPlayerLight()->_enabled = true;
         }
         _log.emplace_back("player light enabled");
      }
   );

   addCommand(
      "playerlight disable",
      [this](const auto&)
      {
         auto level = LevelRegistry::getCurrent();
         if (level && level->getPlayerLight())
         {
            level->getPlayerLight()->_enabled = false;
         }
         _log.emplace_back("player light disabled");
      }
   );

   addCommand(
      "playerlight alpha",
      [this](const auto& args)
      {
         if (args.size() == 3)
         {
            const auto alpha = static_cast<uint8_t>(std::clamp(std::atoi(args.at(2).c_str()), 0, 255));
            auto level = LevelRegistry::getCurrent();
            if (level && level->getPlayerLight())
            {
               level->getPlayerLight()->_color.a = alpha;
#ifdef __EMSCRIPTEN__
               level->getPlayerLight()->_sprite->color = level->getPlayerLight()->_color;
#else
               level->getPlayerLight()->_sprite->setColor(level->getPlayerLight()->_color);
#endif
            }
            std::ostringstream oss;
            oss << "player light alpha set to " << static_cast<int>(alpha);
            _log.push_back(oss.str());
         }
      }
   );

   // cheats
   registerCallback(
      "iddqd",
      [this](const auto&)
      {
         SaveState::getPlayerInfo()._extra_table._skills._skills |= static_cast<int32_t>(Skill::SkillType::Invulnerable);
         _log.emplace_back("invulnerable");
      },
      "cheats",
      "iddqd: make player invulnerable"
   );

   registerCallback(
      "damage",
      [this](const auto& args)
      {
         if (args.size() == 2)
         {
            const auto damage = std::atoi(args.at(1).c_str());
            PlayerRegistry::getFirst()->damage(damage);
            std::ostringstream os;
            os << "damage player " << damage << std::endl;
            _log.push_back(os.str());
         }
      },
      "cheats",
      "damage <n>: cause damage to player",
      {"damage 100"}
   );

   registerCallback(
      "pgravity",
      [this](const auto& args)
      {
         if (args.size() == 2)
         {
            const auto scale = std::atof(args.at(1).c_str());
            PlayerRegistry::getFirst()->getBody()->SetGravityScale(scale);
            std::ostringstream os;
            os << "player gravity " << scale << std::endl;
            _log.push_back(os.str());
         }
      },
      "cheats",
      "pgravity <gravity>: set player gravity scale",
      {"pgravity 0.1"}
   );

   // leveldesign
   registerCallback(
      "cpanlimitoff",
      [this](const auto&)
      {
         Tweaks::instance()._cpan_unlimited = true;
         _log.emplace_back("disabled cpan limit");
      },
      "leveldesign",
      "cpanlimitoff: disable cpan maximum radius"
   );

   registerCallback(
      "ra",
      [](const auto&) { std::static_pointer_cast<Player>(PlayerRegistry::getFirst())->reloadAnimationPool(); },
      "leveldesign",
      "ra: reload animations"
   );
}

void Console::setActive(bool active)
{
   _active = active;
}

void Console::append(char32_t unicode)
{
   if (unicode > 0x1F && unicode < 0x80)
   {
      _command.push_back(unicode);
   }
}

void Console::chop()
{
   if (_command.empty())
   {
      return;
   }

   _command.pop_back();
}

void Console::giveWeaponBow()
{
   auto bow = WeaponFactory::create(WeaponType::Bow);
   std::dynamic_pointer_cast<Bow>(bow)->setLauncherBody(PlayerRegistry::getFirst()->getBody());
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
   auto level = LevelRegistry::getCurrent();
   level->loadStartPosition();
   const auto pos_px = level->getStartPosition();
   PlayerRegistry::getFirst()->setBodyViaPixelPosition(static_cast<float>(pos_px.x), static_cast<float>(pos_px.y));
}

void Console::teleportToCheckpoint(int32_t checkpoint_index)
{
   std::ostringstream os;

   auto checkpoint = Checkpoint::getCheckpoint(checkpoint_index, LevelRegistry::getCurrent()->getMechanismRegistry().getCheckpoints());
   if (checkpoint)
   {
      const auto pos = checkpoint->spawnPoint();
      os << "jumped to checkpoint " << checkpoint_index << std::endl;

      PlayerRegistry::getFirst()->setBodyViaPixelPosition(static_cast<float>(pos.x), static_cast<float>(pos.y));
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

   PlayerRegistry::getFirst()->setBodyViaPixelPosition(
      static_cast<float>(x_tl * PIXELS_PER_TILE), static_cast<float>(y_tl * PIXELS_PER_TILE)
   );
}

void Console::teleportToRoom(const std::string& room_name)
{
   std::ostringstream os;
   auto level = LevelRegistry::getCurrent();
   const auto& rooms = level->getRooms();

   std::shared_ptr<Room> found_room;
   for (const auto& room : rooms)
   {
      if (room->getObjectId() == room_name)
      {
         found_room = room;
         break;
      }
   }

   if (!found_room)
   {
      _log.push_back("room '" + room_name + "' not found");
      _log.push_back("available rooms:");

      // list available rooms (alphabetically sorted), 5 per line
      constexpr size_t rooms_per_line = 5;

      std::vector<std::string> room_names;
      room_names.reserve(rooms.size());
      for (const auto& room : rooms)
      {
         room_names.push_back(room->getObjectId());
      }
      std::ranges::sort(room_names);

      std::string line = "  ";
      for (size_t index = 0; index < room_names.size(); ++index)
      {
         if (index > 0 && index % rooms_per_line == 0)
         {
            _log.push_back(line);
            line = "  ";
         }

         if (index % rooms_per_line > 0)
         {
            line += ", ";
         }

         line += room_names[index];
      }

      if (!line.empty() && line != "  ")
      {
         _log.push_back(line);
      }

      return;
   }

   sf::Vector2f target_position;
   if (!found_room->_sub_rooms.empty())
   {
      const auto& sub_room = found_room->_sub_rooms.front();
      target_position.x = sub_room._rect.position.x + sub_room._rect.size.x / 2.0f;
      target_position.y = sub_room._rect.position.y + sub_room._rect.size.y / 2.0f;
      os << "teleported to room '" << room_name << "' (first subroom)" << std::endl;
   }

   _log.push_back(os.str());
   PlayerRegistry::getFirst()->setBodyViaPixelPosition(target_position.x, target_position.y);
}

const Console::Help& Console::help() const
{
   return _help;
}

void Console::execute()
{
   Log::Info() << "process command: " << _command;

   std::istringstream iss(_command);
   std::vector<std::string> results((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

   if (results.empty())
   {
      return;
   }

   _log.push_back(_command);

   const auto joined = [&](size_t token_count)
   {
      std::string key = results[0];
      for (size_t token_index = 1; token_index < token_count; ++token_index)
      {
         key += ' ';
         key += results[token_index];
      }
      return key;
   };

   auto command_it = _registered_commands.end();
   for (auto token_count = std::min(results.size(), size_t{3}); token_count > 0 && command_it == _registered_commands.end(); --token_count)
   {
      command_it = _registered_commands.find(joined(token_count));
   }

   if (command_it != _registered_commands.end())
   {
      command_it->second(results);
   }
   else
   {
      std::ostringstream os;
      os << "unknown command: " << _command << std::endl;
      _log.push_back(os.str());
   }

   while (_log.size() > 50)
   {
      _log.pop_front();
   }

   _history.push_back(_command);
   _history_index = static_cast<int32_t>(_history.size());
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

void Console::complete()
{
   if (_command.empty())
   {
      return;
   }

   std::vector<std::string> matches;
   for (const auto& [command_name, callback] : _registered_commands)
   {
      if (command_name.size() >= _command.size() && command_name.compare(0, _command.size(), _command) == 0)
      {
         matches.push_back(command_name);
      }
   }

   if (matches.empty())
   {
      return;
   }

   std::ranges::sort(matches);

   if (matches.size() == 1)
   {
      _command = matches.front() + ' ';
      return;
   }

   // reduce all matches to their longest common prefix so the input can be extended as far as it is unambiguous
   std::string common_prefix = matches.front();
   for (const auto& match : matches)
   {
      const auto comparable_length = std::min(common_prefix.size(), match.size());
      size_t prefix_length = 0;
      while (prefix_length < comparable_length && common_prefix[prefix_length] == match[prefix_length])
      {
         ++prefix_length;
      }
      common_prefix.resize(prefix_length);
   }

   _command = common_prefix;

   // print the remaining candidates so the user can decide how to continue typing
   std::string candidate_line = "  ";
   for (size_t index = 0; index < matches.size(); ++index)
   {
      if (index > 0)
      {
         candidate_line += "  ";
      }
      candidate_line += matches[index];
   }
   _log.push_back(candidate_line);
}

void Console::addCommand(const std::string& command, CommandFunction callback)
{
   _registered_commands[command] = callback;
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

void Console::toggleActive()
{
   DebugDrawStates::_draw_console = !DebugDrawStates::_draw_console;
   Console::getInstance().setActive(DebugDrawStates::_draw_console);
   GameState::getInstance().enqueueTogglePauseResume();
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

void Console::processEvent(sf::Keyboard::Key key)
{
   if (key == sf::Keyboard::Key::Enter)
   {
      execute();
   }
   else if (key == sf::Keyboard::Key::Backspace)
   {
      chop();
   }
   else if (key == sf::Keyboard::Key::Up)
   {
      previousCommand();
   }
   else if (key == sf::Keyboard::Key::Down)
   {
      nextCommand();
   }
   else if (key == sf::Keyboard::Key::Tab)
   {
      complete();
   }
   else if (key == sf::Keyboard::Key::F12)
   {
      toggleActive();
   }
}
