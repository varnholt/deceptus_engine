#pragma once

#include <cstdint>
#include <string>
#include <vector>

class GameNode
{
public:

   GameNode(GameNode* parent = nullptr);
   virtual ~GameNode();

   GameNode* getParent() const;
   void dump(int32_t depth = 0);

   const std::string& getObjectId() const;
   void setObjectId(const std::string& newObject_name);

protected:

   void setClassName(const std::string& name);

   std::string _class_name;
   std::string _object_id;
   std::vector<GameNode*> _children;
   GameNode* _parent = nullptr;
};

