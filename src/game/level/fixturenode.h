#pragma once

#include "constants.h"
#include "gamenode.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>

/// \brief stores metadata attached to a box2d fixture through user data.
class FixtureNode : public GameNode
{
public:
   using CollisionCallback = std::function<void(void)>;
   using Variant = std::variant<std::string, int32_t, double, bool>;

   /// \brief creates fixture metadata and attaches it to the game node hierarchy.
   /// \param parent owning node, typically the level.
   FixtureNode(GameNode* parent);

   /// \brief returns the object type used for collision handling.
   /// \return fixture object type.
   ObjectType getType() const;

   /// \brief sets the object type used for collision handling.
   /// \param type fixture object type.
   void setType(const ObjectType& type);

   /// \brief sets or overwrites a named boolean flag.
   /// \param flag flag name.
   /// \param value flag value to store.
   void setFlag(const std::string& flag, bool value);

   /// \brief returns the stored value for a named flag.
   /// \param flag flag name.
   /// \return stored flag value; missing flags evaluate to false and are inserted.
   bool hasFlag(const std::string& flag);

   /// \brief sets or overwrites a typed property value.
   /// \param key property name.
   /// \param value property value as variant type.
   void setProperty(const std::string& key, const Variant& value);

   /// \brief returns a typed property value.
   /// \param key property name.
   /// \return stored property value for the key.
   Variant getProperty(const std::string& key) const;

   /// \brief checks whether a property key exists.
   /// \param key property name.
   /// \return true when the key exists in the property map.
   bool hasProperty(const std::string& key) const;

   /// \brief invokes the registered player collision callback.
   virtual void collisionWithPlayer();

   /// \brief stores the callback executed by collisionWithPlayer().
   /// \param collisionCallback callback invoked on player collision.
   void setCollisionCallback(const CollisionCallback& collisionCallback);

protected:
   ObjectType _type = ObjectTypeInvalid;
   std::map<std::string, bool> _flags;
   std::map<std::string, Variant> _properties;
   CollisionCallback _collision_callback;
};
