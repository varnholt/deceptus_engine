#include <deque>
#include <string>

class Console
{
   public:

      bool isActive() const;
      void setActive(bool active);

      void append(char c);
      void chop();
      void execute();

      const std::string& getCommand() const;
      const std::deque<std::string>& getLog() const;

      static Console& getInstance();


private:

      Console() = default;

      bool mActive = false;
      std::string mCommand;

      std::deque<std::string> mLog;

      static Console mConsole;
};

