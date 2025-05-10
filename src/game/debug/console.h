#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>

class Console
{
public:
   using CommandFunction = std::function<void(void)>;

   struct HelpCommand
   {
      std::string description;
      std::vector<std::string> examples;
   };

   struct Help
   {
      void registerCommand(const std::string& topic, const std::string& description, const std::vector<std::string>& examples = {});
      std::string getFormattedHelp() const;
      std::map<std::string, std::vector<HelpCommand>> _help_messages;
   };

   bool isActive() const;
   void setActive(bool active);

   void append(char c);
   void chop();
   void execute();
   void previousCommand();
   void nextCommand();

   void registerCallback(
      const std::string& command,
      CommandFunction callback,
      const std::string& topic,
      const std::string& description,
      const std::vector<std::string>& examples = {}
   );

   const std::string& getCommand() const;
   const std::deque<std::string>& getLog() const;

   static Console& getInstance();

   const Help& help() const;

private:
   Console();

   void giveWeaponBow();
   void giveWeaponGun();
   void giveWeaponSword();

   void teleportToStartPosition();
   void teleportToCheckpoint(int32_t checkpoint_index);
   void teleportToTile(int32_t x_tl, int32_t y_tl);

   bool _active = false;
   std::string _command;

   std::vector<std::string> _history;
   int32_t _history_index = 0;
   std::deque<std::string> _log;

   // support for generic commands registered from the outside
   std::map<std::string, CommandFunction> _registered_commands;

   Help _help;
};
