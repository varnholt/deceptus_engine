#include "camerasystemconfiguration.h"

#include <fstream>
#include <iostream>
#include <iomanip>
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
   json config =
   {
      {
         "CameraSystemConfiguration",
         {
            {"damping_factor_x",           mDampingFactorX},
            {"focus_zone_divider",         mFocusZoneDivider},
            {"target_shift_factor",        mTargetShiftFactor},
            {"back_in_bounds_tolerance_x", mBackInBoundsToleranceX},
            {"damping_factor_y",           mDampingFactorY},
            {"panic_line_divider",         mPanicLineDivider},
            {"view_ratio_y",               mViewRatioY},
            {"back_in_bounds_tolerance_y", mBackInBoundsToleranceY},
            {"player_offset_y",            mPlayerOffsetY},
         }
      }
   };

   std::stringstream sstream;
   sstream << std::setw(4) << config << "\n\n";
   return sstream.str();
}


void CameraSystemConfiguration::deserialize(const std::string& data)
{
   json config = json::parse(data);

   mDampingFactorX         = config["CameraSystemConfiguration"]["damping_factor_x"].get<float>();
   mFocusZoneDivider       = config["CameraSystemConfiguration"]["focus_zone_divider"].get<float>();
   mTargetShiftFactor      = config["CameraSystemConfiguration"]["target_shift_factor"].get<float>();
   mBackInBoundsToleranceX = config["CameraSystemConfiguration"]["back_in_bounds_tolerance_x"].get<int32_t>();
   mDampingFactorY         = config["CameraSystemConfiguration"]["damping_factor_y"].get<float>();
   mPanicLineDivider       = config["CameraSystemConfiguration"]["panic_line_divider"].get<float>();
   mViewRatioY             = config["CameraSystemConfiguration"]["view_ratio_y"].get<float>();
   mBackInBoundsToleranceY = config["CameraSystemConfiguration"]["back_in_bounds_tolerance_y"].get<int32_t>();
   mPlayerOffsetY          = config["CameraSystemConfiguration"]["player_offset_y"].get<int32_t>();
}


void CameraSystemConfiguration::deserializeFromFile(const std::string &filename)
{
  std::ifstream ifs (filename, std::ifstream::in);

  char c = ifs.get();
  std::string data;

  while (ifs.good())
  {
    // std::cout << c;
    data.push_back(c);
    c = ifs.get();
  }

  ifs.close();

  deserialize(data);
}


void CameraSystemConfiguration::serializeToFile(const std::string &filename)
{
  std::string data = serialize();
  std::ofstream file(filename);
  file << data;
}

int32_t CameraSystemConfiguration::getPlayerOffsetY() const
{
   return mPlayerOffsetY;
}


int32_t CameraSystemConfiguration::getBackInBoundsToleranceY() const
{
   return mBackInBoundsToleranceY;
}


float CameraSystemConfiguration::getViewRatioY() const
{
   return mViewRatioY;
}


float CameraSystemConfiguration::getPanicLineDivider() const
{
   return mPanicLineDivider;
}


float CameraSystemConfiguration::getDampingFactorY() const
{
   return mDampingFactorY;
}


int32_t CameraSystemConfiguration::getBackInBoundsToleranceX() const
{
   return mBackInBoundsToleranceX;
}


float CameraSystemConfiguration::getTargetShiftFactor() const
{
   return mTargetShiftFactor;
}


float CameraSystemConfiguration::getFocusZoneDivider() const
{
   return mFocusZoneDivider;
}


float CameraSystemConfiguration::getDampingFactorX() const
{
   return mDampingFactorX;
}

