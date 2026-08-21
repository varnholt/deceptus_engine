#pragma once

#include <string>

///
/// \brief Resolves the name of the user the game is running for.
///
namespace PlatformUser
{
///
/// \brief Returns the platform's idea of the current user's name.
///
/// On the switch this is the nickname of the console account the title was launched with. On
/// desktop it is the account name from the environment, reduced to a first name. The result is
/// raw platform data: it may be empty and may contain characters the caller cannot display.
///
/// \return User name, or an empty string when the platform has none to offer.
///
std::string getUserName();

}  // namespace PlatformUser
