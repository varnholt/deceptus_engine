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

   ExtraType _extra_type = ExtraType::Invalid;
};

