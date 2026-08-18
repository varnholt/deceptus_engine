#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "SFML/Graphics.hpp"

/// \brief manages the in-game developer console, including command parsing, history, and output logs.
class Console
{
public:
   using CommandFunction = std::function<void(const std::vector<std::string>&)>;

   /// \brief stores one help entry with a command description and optional usage examples.
   struct HelpCommand
   {
      std::string description;
      std::vector<std::string> examples;
   };

   /// \brief groups help entries by topic and formats them for display in the console.
   struct Help
   {
      /// \brief one rendered line of the help panel, classified so the renderer can indent and color it.
      struct HelpLine
      {
         enum class Kind
         {
            Topic,
            Command,
            Example,
            Hint
         };

         Kind _kind{Kind::Command};  //!< how the line should be indented and colored
         std::string _text;          //!< text to render
      };

      /// \brief adds a command description to a help topic.
      /// \param topic topic name used to group related commands.
      /// \param description one-line command description shown in help output.
      /// \param examples optional usage examples shown below the description.
      void registerCommand(const std::string& topic, const std::string& description, const std::vector<std::string>& examples = {});

      /// \brief builds the help lines to show for the current console input.
      ///
      /// With nothing typed only the topic names are listed, so the panel stays a fixed handful of
      /// lines no matter how many commands exist. Typing narrows the list to the commands matching
      /// the input and reveals their examples, which are only interesting for the command actually
      /// being reached for. The result never exceeds max_lines, so the panel cannot outgrow the
      /// screen again however many commands are added later.
      /// \param filter current console input, matched case insensitively against the descriptions.
      /// \param max_lines maximum number of lines the caller is able to display.
      /// \return lines to render, in display order.
      std::vector<HelpLine> getVisibleLines(const std::string& filter, size_t max_lines) const;

      /// \brief builds a sorted, multi-line help text containing all registered topics and commands.
      /// \return formatted help text ready to print into the console log.
      std::string getFormattedHelp() const;
      std::map<std::string, std::vector<HelpCommand>> _help_messages;
   };

   /// \brief changes whether the console accepts input and is considered active.
   /// \param active true to enable the console, false to disable it.
   void setActive(bool active);

   /// \brief appends one printable ascii character to the current command line.
   /// \param unicode character code received from text input.
   void append(char32_t unicode);

   /// \brief removes the last character from the current command line, if any.
   void chop();

   /// \brief executes the current command, applies game-side effects, logs output, and stores command history.
   void execute();

   /// \brief recalls the previous command from history into the input line.
   void previousCommand();

   /// \brief recalls the next command from history into the input line.
   void nextCommand();

   /// \brief completes the current command line against the registered commands.
   /// on a unique prefix match the command is filled in and a trailing space is appended;
   /// on multiple matches the input is extended to the longest common prefix and the
   /// remaining candidates are printed to the console log.
   void complete();

   /// \brief registers an external command callback and exposes it in the help system.
   /// \param command command keyword used to trigger the callback.
   /// \param callback function invoked when the command is entered.
   /// \param topic help topic under which the command description is listed.
   /// \param description human-readable help text for the command.
   /// \param examples optional usage examples displayed in the help output.
   void registerCallback(
      const std::string& command,
      CommandFunction callback,
      const std::string& topic,
      const std::string& description,
      const std::vector<std::string>& examples = {}
   );

   /// \brief retrieves the command currently being edited in the input line.
   /// \return reference to the current command string.
   const std::string& getCommand() const;

   /// \brief retrieves the rolling console log containing command echoes and execution results.
   /// \return reference to the log deque.
   const std::deque<std::string>& getLog() const;

   /// \brief retrieves the global console singleton.
   /// \return reference to the shared console instance.
   static Console& getInstance();

   /// \brief retrieves read-only access to registered help topics and commands.
   /// \return reference to the console help registry.
   const Help& help() const;

   /// \brief toggles console visibility, syncs debug draw state, and enqueues pause or resume.
   void toggleActive();

   /// \brief handles console hotkeys such as execute, history navigation, and toggle.
   /// \param key keyboard key received from SFML.
   void processEvent(sf::Keyboard::Key key);

private:
   /// \brief constructs the singleton console and pre-registers built-in command help entries.
   Console();

   /// \brief inserts a command into the dispatch table without adding a help entry.
   /// \param command the key used to look up this command (may be multi-token, e.g. "weapon add gun").
   /// \param callback function invoked when the command is matched.
   void addCommand(const std::string& command, CommandFunction callback);

   /// \brief gives the player a bow weapon and binds it to the current player body.
   void giveWeaponBow();

   /// \brief gives the player a gun weapon and selects it.
   void giveWeaponGun();

   /// \brief gives the player a sword weapon and selects it.
   void giveWeaponSword();

   /// \brief gives the player a harpoon weapon and selects it.
   void giveWeaponHarpoon();

   /// \brief teleports the player to the level start spawn position.
   void teleportToStartPosition();

   /// \brief teleports the player to a checkpoint spawn point and logs the result.
   /// \param checkpoint_index index of the checkpoint in the level checkpoint registry.
   void teleportToCheckpoint(int32_t checkpoint_index);

   /// \brief teleports the player to a tile coordinate converted to pixel space.
   /// \param x_tl tile x coordinate.
   /// \param y_tl tile y coordinate.
   void teleportToTile(int32_t x_tl, int32_t y_tl);

   /// \brief teleports the player to the center of the first sub-room of a named room.
   /// \param room_name room object id to search in the current level.
   void teleportToRoom(const std::string& room_name);

   /// \brief prints the levels listed in levels.json together with their indices.
   void listLevels();

   /// \brief prints the mechanism types present in the current level with their enabled counts.
   void listMechanismTypes();

   /// \brief enables or disables every mechanism whose type matches a filter.
   /// \details the filter is matched case insensitively against both the tmx group name a mechanism
   ///          was loaded from and its object name, as a substring, so "smoke" reaches the
   ///          "smoke_effect" group without having to spell it out.
   /// \param type_filter partial mechanism type name.
   /// \param enabled true to enable the matching mechanisms, false to disable them.
   void setMechanismTypeEnabled(const std::string& type_filter, bool enabled);

   /// \brief switches to another level listed in levels.json.
   /// \details the level is identified either by its index in the list or by a case insensitive
   ///          substring of its description filename, so "level load graveyard" is enough. this
   ///          takes the same route as a level change requested from a lua script: the save state
   ///          is pointed at the level and the loader picks it up, leaving the player at the target
   ///          level's own start position or checkpoint.
   /// \param level_identifier index into levels.json, or part of a level description filename.
   void loadLevel(const std::string& level_identifier);

   bool _active = false;
   std::string _command;

   std::vector<std::string> _history;
   int32_t _history_index = 0;
   std::deque<std::string> _log;

   // support for generic commands registered from the outside
   std::map<std::string, CommandFunction> _registered_commands;

   Help _help;
};
