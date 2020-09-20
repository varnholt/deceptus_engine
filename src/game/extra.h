#pragma once

class Extra
{
public:

   enum class ExtraType
   {
      Invalid,
      Health
   };

   Extra() = default;

   ExtraType mExtraType = ExtraType::Invalid;
};

