#pragma once

#include <cstdint>
#include <string>
#include <vector>

class GameNode
{
public:

   GameNode(GameNode* parent = nullptr);
   virtual ~GameNode() = default;

   GameNode* getParent() const;
   void dump(int32_t depth = 0);

   const std::string& getObjectName() const;
   void setObjectName(const std::string& newObject_name);

   protected:
   void setClassName(const std::string& name);

private:
   std::string _class_name;
   std::string _object_name;
   std::vector<GameNode*> _children;
   GameNode* _parent = nullptr;
};

