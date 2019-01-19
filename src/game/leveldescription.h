#pragma once

#include "json/json.hpp"


struct ScriptProperty
{
   ScriptProperty() = default;

   std::string mName;
   std::string mValue;
};


struct EnemyDescription
{
   EnemyDescription() = default;

   std::string mScript;
   std::vector<int> mStartPosition;
   std::vector<int> mPatrolPath;
   std::vector<ScriptProperty> mProperties;
};


struct LevelDescription
{
   LevelDescription() = default;

   std::string mFilename;
   std::vector<int> mStartPosition;
   std::vector<EnemyDescription> mEnemies;

   static std::shared_ptr<LevelDescription> load(const std::string& path);
};

void to_json(nlohmann::json& j, const ScriptProperty& p);
void from_json(const nlohmann::json& j, ScriptProperty& p);

void to_json(nlohmann::json& j, const EnemyDescription& d);
void from_json(const nlohmann::json& j, EnemyDescription& d);

void to_json(nlohmann::json& j, const LevelDescription& d);
void from_json(const nlohmann::json& j, LevelDescription& d);

