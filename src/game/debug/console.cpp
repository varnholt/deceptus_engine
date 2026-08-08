#include "console.h"

#include "framework/tools/log.h"
#include "game/config/tweaks.h"
#include "game/debug/debugdrawstates.h"
#include "framework/tools/callbackmap.h"
#include "game/constants.h"
#include "game/level/gamemechanismregistry.h"
#include "game/level/levelregistry.h"
#include "game/level/levels.h"
#include "game/level/room.h"
#include "game/mechanisms/checkpoint.h"
#include "game/player/player.h"
#include "game/player/playerinfo.h"
#include "game/player/playerregistry.h"
#include "game/player/weaponsystem.h"
#include "game/shaders/postprocessing.h"
#include "game/state/gamestate.h"
#include "game/state/savestate.h"
#include "game/weapons/bow.h"
#include "game/weapons/weaponfactory.h"

#include <cctype>
#include <iostream>
#include <map>
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

std::string toLowerCase(const std::string& text)
{
   std::string lower_case;
   lower_case.reserve(text.size());
   for (const auto character : text)
   {
      lower_case.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
   }
   return lower_case;
}

//! \brief console input prefix that switches the help panel into its detailed mode
constexpr std::string_view help_prefix{"help"};

std::string joinNames(const std::vector<std::string>& names)
{
   std::string joined;
   for (const auto& name : names)
   {
      if (!joined.empty())
      {
         joined += '|';
      }
      joined += name;
   }
   return joined;
}

std::string joinEffectNames()
{
   return joinNames(PostProcessing::getEffectNames());
}

std::string joinScopeNames()
{
   return joinNames(PostProcessing::getScopeNames());
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

   // level loading
   registerCallback(
      "level list",
      [this](const auto&) { listLevels(); },
      "leveldesign",
      "level list: list the levels from levels.json"
   );

   registerCallback(
      "level load",
      [this](const auto& args)
      {
         if (args.size() == 3)
         {
            loadLevel(args.at(2));
         }
         else
         {
            _log.emplace_back("usage: level load <index|name>");
         }
      },
      "leveldesign",
      "level load <index|name>: load a level listed in levels.json",
      {"level load 2", "level load graveyard"}
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

   // mechanisms
   _help.registerCommand(
      "leveldesign",
      "mechanism <list/enable/disable> <type>: enable or disable every mechanism of a type",
      {"mechanism list", "mechanism disable smoke", "mechanism enable smoke"}
   );

   addCommand("mechanism list", [this](const auto&) { listMechanismTypes(); });

   addCommand(
      "mechanism enable",
      [this](const auto& args)
      {
         if (args.size() == 3)
         {
            setMechanismTypeEnabled(args.at(2), true);
         }
         else
         {
            _log.emplace_back("usage: mechanism enable <type>");
         }
      }
   );

   addCommand(
      "mechanism disable",
      [this](const auto& args)
      {
         if (args.size() == 3)
         {
            setMechanismTypeEnabled(args.at(2), false);
         }
         else
         {
            _log.emplace_back("usage: mechanism disable <type>");
         }
      }
   );

   // lighting
   _help.registerCommand(
      "leveldesign",
      "lighting <enable/disable>: bypass the deferred lighting pass and show the level unlit",
      {"lighting enable", "lighting disable"}
   );

   addCommand(
      "lighting enable",
      [this](const auto&)
      {
         DebugDrawStates::_draw_lighting = true;
         _log.emplace_back("lighting enabled");
      }
   );

   addCommand(
      "lighting disable",
      [this](const auto&)
      {
         DebugDrawStates::_draw_lighting = false;
         _log.emplace_back("lighting disabled, level is drawn unlit");
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

   // ingame map
   _help.registerCommand(
      "cheats", "map <reveal/clear>: reveal the whole ingame map, or reset it to completely unexplored", {"map reveal", "map clear"}
   );

   addCommand(
      "map reveal",
      [this](const auto&)
      {
         const auto& level = LevelRegistry::getCurrent();
         if (!level)
         {
            _log.emplace_back("no level loaded");
            return;
         }

         level->setMapRevealed(true);
         _log.emplace_back("revealed the whole map");
      }
   );

   addCommand(
      "map clear",
      [this](const auto&)
      {
         const auto& level = LevelRegistry::getCurrent();
         if (!level)
         {
            _log.emplace_back("no level loaded");
            return;
         }

         level->setMapRevealed(false);
         for (const auto& room : level->getRooms())
         {
            room->clearVisited();
         }

         _log.emplace_back("cleared the map");
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

   // help
   registerCallback(
      "help",
      [this](const auto& args)
      {
         // the panel already answers this live while typing, so executing it only needs to leave
         // something in the log confirming what was looked up
         if (args.size() < 2)
         {
            _log.emplace_back("help: type a command name to see its examples in the panel");
            return;
         }

         _log.emplace_back("help: " + args.at(1));
      },
      "general",
      "help <command>: show examples for a command in the help panel",
      {"help postfx", "help tpp"}
   );

   // rendering
   registerCallback(
      "postfx",
      [this](const auto& args)
      {
         if (args.size() != 2)
         {
            _log.emplace_back("usage: postfx <" + joinEffectNames() + ">");
            return;
         }

         const auto effect = PostProcessing::effectFromName(args.at(1));
         if (!effect.has_value())
         {
            _log.emplace_back("unknown post processing effect: " + args.at(1));
            _log.emplace_back("available effects: " + joinEffectNames());
            return;
         }

         PostProcessing::getInstance().setEffect(effect.value());
         _log.emplace_back("post processing effect: " + args.at(1));
      },
      "rendering",
      "postfx <" + joinEffectNames() + ">: set the full screen post processing effect",
      {"postfx gameboy", "postfx none"}
   );

   registerCallback(
      "postfx scope",
      [this](const auto& args)
      {
         if (args.size() != 3)
         {
            _log.emplace_back("usage: postfx scope <" + joinScopeNames() + ">");
            return;
         }

         const auto scope = PostProcessing::scopeFromName(args.at(2));
         if (!scope.has_value())
         {
            _log.emplace_back("unknown post processing scope: " + args.at(2));
            _log.emplace_back("available scopes: " + joinScopeNames());
            return;
         }

         PostProcessing::getInstance().setScope(scope.value());
         _log.emplace_back("post processing scope: " + args.at(2));
      },
      "rendering",
      "postfx scope <" + joinScopeNames() + ">: apply the effect to the whole frame or to the level only",
      {"postfx scope level", "postfx scope all"}
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

void Console::listMechanismTypes()
{
   const auto level = LevelRegistry::getCurrent();
   if (!level)
   {
      _log.emplace_back("no level loaded");
      return;
   }

   // sorted so the output is stable, the registry map is unordered
   std::map<std::string, std::pair<int32_t, int32_t>> counts_by_type;
   for (const auto& [group, mechanisms] : level->getMechanismRegistry().getMap())
   {
      if (mechanisms == nullptr || mechanisms->empty())
      {
         continue;
      }

      auto& counts = counts_by_type[group];
      counts.first = static_cast<int32_t>(mechanisms->size());
      counts.second =
         static_cast<int32_t>(std::ranges::count_if(*mechanisms, [](const auto& mechanism) { return mechanism->isEnabled(); }));
   }

   if (counts_by_type.empty())
   {
      _log.emplace_back("this level has no mechanisms");
      return;
   }

   // packed a few per line, a level can easily have more types than the console can show
   constexpr auto types_per_line = 3;
   std::ostringstream line;
   auto column = 0;
   for (const auto& [group, counts] : counts_by_type)
   {
      line << group << " " << counts.second << "/" << counts.first << "   ";
      if (++column == types_per_line)
      {
         _log.push_back(line.str());
         line.str({});
         column = 0;
      }
   }

   if (column > 0)
   {
      _log.push_back(line.str());
   }
}

void Console::setMechanismTypeEnabled(const std::string& type_filter, bool enabled)
{
   const auto level = LevelRegistry::getCurrent();
   if (!level)
   {
      _log.emplace_back("no level loaded");
      return;
   }

   const auto needle = toLowerCase(type_filter);
   const auto mechanisms = level->getMechanismRegistry().searchMechanismsIf(
      [&needle](const auto& mechanism, std::string_view group)
      { return toLowerCase(std::string{group}).contains(needle) || toLowerCase(std::string{mechanism->objectName()}).contains(needle); }
   );

   if (mechanisms.empty())
   {
      _log.emplace_back("no mechanism type matching '" + type_filter + "', try 'mechanism list'");
      return;
   }

   for (const auto& mechanism : mechanisms)
   {
      mechanism->setEnabled(enabled);
   }

   std::ostringstream message;
   message << (enabled ? "enabled " : "disabled ") << mechanisms.size() << " mechanism(s) matching '" << type_filter << "'";
   _log.push_back(message.str());
}

void Console::listLevels()
{
   const auto level_items = Levels::readLevelItems();
   if (level_items.empty())
   {
      _log.emplace_back("no levels listed in data/config/levels.json");
      return;
   }

   for (auto index = 0; index < static_cast<int32_t>(level_items.size()); index++)
   {
      _log.push_back("  " + std::to_string(index) + ": " + level_items[index]._level_name);
   }
}

void Console::loadLevel(const std::string& level_identifier)
{
   const auto level_items = Levels::readLevelItems();
   if (level_items.empty())
   {
      _log.emplace_back("no levels listed in data/config/levels.json");
      return;
   }

   const auto lowercase = [](std::string text)
   {
      std::ranges::transform(text, text.begin(), [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
      return text;
   };

   // an all-digit identifier is an index into levels.json, anything else is matched against the
   // level description filenames so that "level load graveyard" works without the full path
   std::optional<int32_t> matched_index;
   const auto is_index =
      !level_identifier.empty() && std::ranges::all_of(level_identifier, [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; });

   if (is_index)
   {
      const auto index = std::atoi(level_identifier.c_str());
      if (index >= 0 && index < static_cast<int32_t>(level_items.size()))
      {
         matched_index = index;
      }
   }
   else
   {
      const auto needle = lowercase(level_identifier);
      for (auto index = 0; index < static_cast<int32_t>(level_items.size()); index++)
      {
         if (lowercase(level_items[index]._level_name).find(needle) != std::string::npos)
         {
            matched_index = index;
            break;
         }
      }
   }

   if (!matched_index.has_value())
   {
      _log.push_back("no level matching '" + level_identifier + "', available levels:");
      listLevels();
      return;
   }

   const auto& level_name = level_items[matched_index.value()]._level_name;
   _log.push_back("loading level " + std::to_string(matched_index.value()) + ": " + level_name);

   // this is the route the lua scripts and checkpoints take for a level change: point the save
   // state at the level and let the loader pick it up. LevelTransitionHandler is deliberately not
   // used - that belongs to the in-level LevelTransition mechanism, which also runs a screen fade
   // and can carry a spawn position, neither of which applies to loading a level from the console.
   SaveState::getCurrent()._level_index = matched_index.value();
   CallbackMap::getInstance().call(static_cast<int32_t>(CallbackType::LoadLevel));
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

std::vector<Console::Help::HelpLine> Console::Help::getVisibleLines(const std::string& filter, size_t max_lines) const
{
   using Kind = HelpLine::Kind;

   std::vector<std::string> sorted_topics;
   sorted_topics.reserve(_help_messages.size());
   for (const auto& entry : _help_messages)
   {
      sorted_topics.push_back(entry.first);
   }
   std::ranges::sort(sorted_topics);

   // "help <something>" asks for detail about a command, which is the only case where the examples
   // are worth the lines they cost
   const auto trimmed = toLowerCase(filter).substr(0, filter.find_last_not_of(' ') + 1);
   const auto detailed = trimmed.starts_with(help_prefix);
   const auto search_term = detailed ? trimmed.substr(help_prefix.size()) : trimmed;
   const auto needle = search_term.substr(std::min(search_term.find_first_not_of(' '), search_term.size()));

   std::vector<HelpLine> lines;

   // nothing to filter by: list the topics only, so the panel keeps its size as commands are added
   if (needle.empty() && !detailed)
   {
      for (const auto& topic : sorted_topics)
      {
         lines.push_back({._kind = Kind::Topic, ._text = topic});
      }
      lines.push_back({._kind = Kind::Hint, ._text = "type to filter, 'help <command>' for examples"});
   }
   else
   {
      for (const auto& topic : sorted_topics)
      {
         std::vector<HelpLine> topic_lines;

         // the panel offers the topic names as the way in, so typing one has to select that whole
         // topic rather than being matched against the command descriptions and finding nothing
         const auto topic_matches = toLowerCase(topic).contains(needle);

         for (const auto& command : _help_messages.at(topic))
         {
            if (!needle.empty() && !topic_matches && !toLowerCase(command.description).contains(needle))
            {
               continue;
            }

            topic_lines.push_back({._kind = Kind::Command, ._text = command.description});

            if (detailed)
            {
               for (const auto& example : command.examples)
               {
                  topic_lines.push_back({._kind = Kind::Example, ._text = example});
               }
            }
         }

         if (topic_lines.empty())
         {
            continue;
         }

         lines.push_back({._kind = Kind::Topic, ._text = topic});
         lines.insert(lines.end(), topic_lines.begin(), topic_lines.end());
      }

      if (lines.empty())
      {
         lines.push_back({._kind = Kind::Hint, ._text = "no matching command"});
      }
   }

   // hard clamp, so the panel cannot outgrow the screen again however many commands are added
   if (max_lines > 0 && lines.size() > max_lines)
   {
      if (max_lines == 1)
      {
         lines.resize(1);
      }
      else
      {
         const auto hidden = lines.size() - (max_lines - 1);
         lines.resize(max_lines - 1);
         lines.push_back({._kind = Kind::Hint, ._text = "+" + std::to_string(hidden) + " more, keep typing"});
      }
   }

   return lines;
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
