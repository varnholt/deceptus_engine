#pragma once

#include <string>

#include "json/json.hpp"


class JsonConfiguration
{
   public:

      JsonConfiguration() = default;

   protected:

      void deserializeFromFile(const std::string& filename);
      void serializeToFile(const std::string& filename);

      nlohmann::json toJson(const std::string& data);
      std::string toString(const nlohmann::json config);

      virtual std::string serialize();
      virtual void deserialize(const std::string& data);
};

