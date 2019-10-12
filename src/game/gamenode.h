#pragma once

class GameNode
{
public:
   GameNode(GameNode* parent = nullptr);
   virtual ~GameNode();

   GameNode* getParent() const;

private:
   GameNode* mParent = nullptr;
};

