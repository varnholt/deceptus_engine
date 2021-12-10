#include <deque>
#include <string>
#include <vector>

class Console
{
   public:

      bool isActive() const;
      void setActive(bool active);

      void append(char c);
      void chop();
      void execute();
      void previousCommand();
      void nextCommand();

      const std::string& getCommand() const;
      const std::deque<std::string>& getLog() const;

      static Console& getInstance();


private:

      Console() = default;
      void showHelp();

      void giveWeaponBow();
      void giveWeaponGun();

      bool _active = false;
      std::string _command;

      std::vector<std::string> _history;
      int32_t _history_index = 0;

      std::deque<std::string> _log;
};

