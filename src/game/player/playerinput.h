#pragma once

/// \brief stores the most recent active input device for the player.
class PlayerInput
{
public:
   enum class InputType
   {
      Keyboard,
      Controller
   };

   /// \brief constructs input-source tracking with keyboard as the default source.
   PlayerInput() = default;

   /// \brief records which input device was used last.
   /// \param input_type input source to store.
   void update(InputType input_type);

   /// \brief reports whether keyboard input was used most recently.
   /// \return true when the last recorded input source is keyboard.
   bool isKeyboardUsed() const;
   /// \brief reports whether controller input was used most recently.
   /// \return true when the last recorded input source is controller.
   bool isControllerUsed() const;

private:
   InputType _input_type = InputType::Keyboard;
};
