#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "SFML/Graphics.hpp"

#include "game/level/luanode.h"

/// \brief singleton that owns and updates all active LuaNode instances.
class LuaInterface
{
public:
   /// \brief gets the global Lua interface instance.
   /// \return singleton instance.
   static LuaInterface& instance();

   using ChunkFilter = std::function<bool(const std::shared_ptr<GameMechanism>&)>;

   /// \brief initializes Lua interface state.
   /// \details currently a no-op placeholder for future setup.
   void initialize();
   /// \brief updates all scripted nodes that pass the chunk filter.
   /// \param dt frame time passed to each LuaNode update.
   /// \param filter predicate that decides whether a node is processed this frame.
   void update(const sf::Time& dt, const ChunkFilter& filter);
   /// \brief removes all registered LuaNode instances.
   void reset();

   /// \brief creates a LuaNode from a script file and stores it in the interface list.
   /// \param parent parent node in the scene graph.
   /// \param filename script file to load for the new LuaNode.
   /// \return shared pointer to the created LuaNode.
   std::shared_ptr<LuaNode> addObject(GameNode* parent, const std::string& filename);
   /// \brief finds the LuaNode that owns a given lua state pointer.
   /// \param state lua state associated with a LuaNode instance.
   /// \return matching LuaNode, or nullptr when no node owns the state.
   std::shared_ptr<LuaNode> getObject(lua_State*);
   /// \brief gets the internal list of all active LuaNode objects.
   /// \return constant reference to the internal LuaNode list.
   const std::vector<std::shared_ptr<LuaNode>>& getObjectList();

private:
   LuaInterface() = default;
};
