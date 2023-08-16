#include "camerasystemconfiguration.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

#include "json/json.hpp"

using json = nlohmann::json;

bool CameraSystemConfiguration::sInitialized = false;
CameraSystemConfiguration CameraSystemConfiguration::sInstance;

CameraSystemConfiguration& CameraSystemConfiguration::getInstance()
{
   if (!sInitialized)
   {
      sInitialized = true;
      // sInstance.serializeToFile();
      sInstance.deserializeFromFile();
   }

   return sInstance;
}

std::string CameraSystemConfiguration::serialize()
{
   // create a JSON value with different types
   json config = {
      {"CameraSystemConfiguration",
       {{"velocity_factor_x", _camera_velocity_factor_x},
        {"focus_zone_divider", _focus_zone_divider},
        {"target_shift_factor", _target_shift_factor},
        {"back_in_bounds_tolerance_x", _back_in_bounds_tolerance_x},
        {"follow_player_orientation", _follow_player_orientation},
        {"velocity_factor_y", _camera_velocity_factor_y},
        {"panic_line_divider", _panic_line_divider},
        {"view_ratio_y", _view_ratio_y},
        {"back_in_bounds_tolerance_y", _back_in_bounds_tolerance_y},
        {"player_offset_y", _player_offset_y},
        {"panic_acceleration_factor_y", _panic_acceleration_factor_y}}}};

   std::stringstream sstream;
   sstream << std::setw(4) << config << "\n\n";
   return sstream.str();
}

void CameraSystemConfiguration::deserialize(const std::string& data)
{
   json config = json::parse(data);

   const auto camera_config = config["CameraSystemConfiguration"];

   _camera_velocity_factor_x = camera_config["velocity_factor_x"].get<float>();
   _focus_zone_divider = camera_config["focus_zone_divider"].get<float>();
   _target_shift_factor = camera_config["target_shift_factor"].get<float>();
   _back_in_bounds_tolerance_x = camera_config["back_in_bounds_tolerance_x"].get<int32_t>();
   _follow_player_orientation = camera_config["follow_player_orientation"].get<bool>();
   _camera_velocity_factor_y = camera_config["velocity_factor_y"].get<float>();
   _panic_line_divider = camera_config["panic_line_divider"].get<float>();
   _view_ratio_y = camera_config["view_ratio_y"].get<float>();
   _back_in_bounds_tolerance_y = camera_config["back_in_bounds_tolerance_y"].get<int32_t>();
   _player_offset_y = camera_config["player_offset_y"].get<int32_t>();
   _panic_acceleration_factor_y = camera_config["panic_acceleration_factor_y"].get<float>();
}

void CameraSystemConfiguration::deserializeFromFile(const std::string& filename)
{
   std::ifstream ifs(filename, std::ifstream::in);

   char c = static_cast<char>(ifs.get());
   std::string data;

   while (ifs.good())
   {
      data.push_back(c);
      c = static_cast<char>(ifs.get());
   }

   ifs.close();

   deserialize(data);
}

void CameraSystemConfiguration::serializeToFile(const std::string& filename)
{
   std::string data = serialize();
   std::ofstream file(filename);
   file << data;
}

bool CameraSystemConfiguration::isFollowingPlayerOrientation() const
{
   return _follow_player_orientation;
}

int32_t CameraSystemConfiguration::getPlayerOffsetY() const
{
   return _player_offset_y;
}

int32_t CameraSystemConfiguration::getBackInBoundsToleranceY() const
{
   return _back_in_bounds_tolerance_y;
}

float CameraSystemConfiguration::getViewRatioY() const
{
   return _view_ratio_y;
}

float CameraSystemConfiguration::getPanicLineDivider() const
{
   return _panic_line_divider;
}

float CameraSystemConfiguration::getCameraVelocityFactorY() const
{
   return _camera_velocity_factor_y;
}

int32_t CameraSystemConfiguration::getBackInBoundsToleranceX() const
{
   return _back_in_bounds_tolerance_x;
}

float CameraSystemConfiguration::getTargetShiftFactor() const
{
   return _target_shift_factor;
}

float CameraSystemConfiguration::getFocusZoneDivider() const
{
   return _focus_zone_divider;
}

float CameraSystemConfiguration::getCameraVelocityFactorX() const
{
   return _camera_velocity_factor_x;
}
