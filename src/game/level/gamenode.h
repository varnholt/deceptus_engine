#pragma once

#include <cstdint>
#include <string>
#include <vector>

/// \brief base node for a lightweight parent-child hierarchy used by level objects.
class GameNode
{
public:
   /// \brief creates a node and links it to the optional parent.
   /// \param parent parent node that receives this node as a child.
   GameNode(GameNode* parent = nullptr);  // should become a weakptr
   /// \brief unlinks this node from its parent and detaches all children.
   virtual ~GameNode();

   /// \brief returns the parent node.
   /// \return parent pointer, or nullptr when this is a root node.
   GameNode* getParent() const;

   /// \brief prints this subtree to stdout for debugging.
   /// \param depth indentation depth for recursive tree printing.
   void dump(int32_t depth = 0);

   /// \brief returns the logical object id used for lookups and serialization.
   /// \return object id string.
   const std::string& getObjectId() const;

   /// \brief sets the logical object id.
   /// \param object_id object id string.
   void setObjectId(const std::string& object_id);

   /// \brief returns the stored runtime class name label.
   /// \return class name label.
   std::string getClassName() const;

   /// \brief sets the runtime class name label.
   /// \param name class name label.
   void setClassName(const std::string& name);

protected:
   std::string _class_name;
   std::string _object_id;
   std::vector<GameNode*> _children;
   GameNode* _parent = nullptr;  // should become a weakptr
};
