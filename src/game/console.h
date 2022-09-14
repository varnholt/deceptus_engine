#include <deque>
#include <functional>
#include <map>
#include <string>
#include <vector>

class Console
{
public:
   using CommandFunction = std::function<void(void)>;

   bool isActive() const;
   void setActive(bool active);

   void append(char c);
   void chop();
   void execute();
   void previousCommand();
   void nextCommand();
   void registerCallback(const std::string& command, const std::string& description, CommandFunction callback);

   const std::string& getCommand() const;
   const std::deque<std::string>& getLog() const;

   static Console& getInstance();

private:
   Console() = default;
   void showHelp();

   void giveWeaponBow();
   void giveWeaponGun();
   void giveWeaponSword();

   bool _active = false;
   std::string _command;

   std::vector<std::string> _history;
   int32_t _history_index = 0;

   std::deque<std::string> _log;

   // support for generic commands registered from the outside
   std::map<std::string, CommandFunction> _registered_commands;
   std::vector<std::pair<std::string, std::string>> _registered_command_help;
};
