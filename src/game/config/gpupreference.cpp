#include "gpupreference.h"

#ifdef _WIN32
#include <windows.h>

#include <string>
#endif

namespace
{

#ifdef _WIN32

//! where windows keeps the per application GPU preferences the graphics settings page writes
constexpr auto registry_path = L"Software\\Microsoft\\DirectX\\UserGpuPreferences";

//! the value windows stores for the discrete GPU
constexpr auto value_high_performance = L"GpuPreference=2;";

//! the value windows stores for the integrated GPU
constexpr auto value_power_saving = L"GpuPreference=1;";

///
/// \brief Determines the full path of the running executable.
/// \return the path windows uses as the value name, empty when it cannot be determined.
///
std::wstring getExecutablePath()
{
   // the path is the value name, so it has to match what windows itself would write, which is the
   // full path of the running image
   std::wstring path(MAX_PATH, L'\0');
   while (true)
   {
      const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
      if (length == 0)
      {
         return {};
      }

      if (length < path.size())
      {
         path.resize(length);
         return path;
      }

      path.resize(path.size() * 2);
   }
}

#endif

}  // namespace

bool GpuPreference::isSupported()
{
#ifdef _WIN32
   return true;
#else
   return false;
#endif
}

GpuPreference::Preference GpuPreference::read()
{
#ifdef _WIN32
   const auto executable_path = getExecutablePath();
   if (executable_path.empty())
   {
      return Preference::Automatic;
   }

   DWORD type = 0;
   DWORD size_bytes = 0;
   if (RegGetValueW(HKEY_CURRENT_USER, registry_path, executable_path.c_str(), RRF_RT_REG_SZ, &type, nullptr, &size_bytes) != ERROR_SUCCESS)
   {
      return Preference::Automatic;
   }

   std::wstring value(size_bytes / sizeof(wchar_t), L'\0');
   if (RegGetValueW(HKEY_CURRENT_USER, registry_path, executable_path.c_str(), RRF_RT_REG_SZ, &type, value.data(), &size_bytes) !=
       ERROR_SUCCESS)
   {
      return Preference::Automatic;
   }

   // windows may store more than the GPU preference in this value, so it is searched rather than compared
   if (value.find(value_high_performance) != std::wstring::npos)
   {
      return Preference::HighPerformance;
   }

   if (value.find(value_power_saving) != std::wstring::npos)
   {
      return Preference::PowerSaving;
   }

   return Preference::Automatic;
#else
   return Preference::Automatic;
#endif
}

void GpuPreference::write(Preference preference)
{
#ifdef _WIN32
   const auto executable_path = getExecutablePath();
   if (executable_path.empty())
   {
      return;
   }

   HKEY key = nullptr;
   if (RegCreateKeyExW(HKEY_CURRENT_USER, registry_path, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS)
   {
      return;
   }

   if (preference == Preference::Automatic)
   {
      // no stored value is what 'let windows decide' looks like, so the entry is removed rather than blanked
      RegDeleteValueW(key, executable_path.c_str());
   }
   else
   {
      const std::wstring value = (preference == Preference::HighPerformance) ? value_high_performance : value_power_saving;
      RegSetValueExW(
         key,
         executable_path.c_str(),
         0,
         REG_SZ,
         reinterpret_cast<const BYTE*>(value.c_str()),
         static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t))
      );
   }

   RegCloseKey(key);
#else
   (void)preference;
#endif
}
