#include <string>

class Console
{
   public:

      bool isActive() const;
      void setActive(bool active);

      void append(char c);
      void chop();
      void execute();

      std::string getCommand() const;

      static Console& getInstance();


private:

      Console() = default;
      bool mActive = false;
      std::string mCommand;

      static Console mConsole;
};

