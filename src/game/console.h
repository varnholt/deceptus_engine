#include <string>

class Console
{
   public:
      Console() = default;

      bool isActive() const;
      void setActive(bool active);

      void append(char c);
      void execute();


   private:

      bool mActive = false;
      std::string mCommand;
};

