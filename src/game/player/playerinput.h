#pragma once

/*! \brief PlayerInput tells where the last player input came from
 *
 * That can be either keyboard or controller.
 */
class PlayerInput
{
public:
   enum class InputType
   {
      Keyboard,
      Controller
   };

   PlayerInput() = default;

   void update(InputType input_type);

   bool isKeyboardUsed() const;
   bool isControllerUsed() const;

private:
   InputType _input_type = InputType::Keyboard;
};
