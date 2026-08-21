#include "platformuser.h"

#ifdef __SWITCH__
#include <switch.h>
#endif

#include <cctype>
#include <cstdlib>
#include <string_view>

namespace
{

#ifndef __SWITCH__
std::string extractFirstName(std::string_view username)
{
   // heuristic 1: split CamelCase
   for (std::size_t i = 1; i < username.size(); ++i)
   {
      if (std::isupper(static_cast<unsigned char>(username[i])))
      {
         return std::string(username.substr(0, i));
      }
   }

   // heuristic 2: try underscores or dots
   if (auto pos = username.find_first_of("._"); pos != std::string_view::npos)
   {
      return std::string(username.substr(0, pos));
   }

   // fallback: return full username
   return std::string(username);
}
#endif

}  // namespace

namespace PlatformUser
{

std::string getUserName()
{
#ifdef __SWITCH__
   if (R_FAILED(accountInitialize(AccountServiceType_Application)))
   {
      return {};
   }

   AccountUid account_uid{};

   // a title started from the home menu carries a preselected user; a homebrew title takeover
   // does not, so fall back to whoever unlocked the console last
   if (R_FAILED(accountGetPreselectedUser(&account_uid)) && R_FAILED(accountGetLastOpenedUser(&account_uid)))
   {
      accountExit();
      return {};
   }

   if (account_uid.uid[0] == 0 && account_uid.uid[1] == 0)
   {
      accountExit();
      return {};
   }

   AccountProfile account_profile{};
   if (R_FAILED(accountGetProfile(&account_profile, account_uid)))
   {
      accountExit();
      return {};
   }

   AccountProfileBase account_profile_base{};
   const auto profile_result = accountProfileGet(&account_profile, nullptr, &account_profile_base);
   accountProfileClose(&account_profile);
   accountExit();

   if (R_FAILED(profile_result))
   {
      return {};
   }

   // the nickname field is a fixed size buffer and is not guaranteed to be null terminated
   const std::string_view nickname(account_profile_base.nickname, sizeof(account_profile_base.nickname));
   const auto terminator_position = nickname.find('\0');

   // the nickname is a display name the player picked, so it is returned as-is instead of being
   // run through the first-name heuristics that desktop account names need
   return std::string(terminator_position == std::string_view::npos ? nickname : nickname.substr(0, terminator_position));
#else
   // probably requires a regular expression to filter out the unicode crap
   auto* u1 = std::getenv("USERNAME");
   auto* u2 = std::getenv("USER");
   const std::string raw_name = u1 ? u1 : (u2 ? u2 : "");
   return extractFirstName(raw_name);
#endif
}

}  // namespace PlatformUser
