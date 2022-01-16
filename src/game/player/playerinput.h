#pragma once

class PlayerInput
{
   public:

      enum class InputType
      {
         Keyboard,
         Controller
      };

      PlayerInput() = default;

      void update();

      bool isKeyboardUsed() const;
      bool isControllerUsed() const;


   private:

      InputType _input_type = InputType::Keyboard;
};

