#pragma once

#include "constants.h"
#include "gamenode.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>


class FixtureNode : public GameNode
{
   public:

      using CollisionCallback = std::function<void(void)>;
      using Variant = std::variant<std::string, int32_t, double>;

      FixtureNode(GameNode *parent);
      virtual ~FixtureNode();

      ObjectType getType() const;
      void setType(const ObjectType &type);

      void setFlag(const std::string& flag, bool value);
      bool hasFlag(const std::string& flag);

      void setProperty(const std::string& key, const Variant& value);
      Variant getProperty(const std::string& key) const;

      virtual void collisionWithPlayer();
      void setCollisionCallback(const CollisionCallback& collisionCallback);


   protected:

      ObjectType mType;
      std::map<std::string, bool> mFlags;
      std::map<std::string, Variant> mProperties;
      CollisionCallback mCollisionCallback;
};

