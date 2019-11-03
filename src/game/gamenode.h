#pragma once


class GameNode
{
public:
   GameNode(GameNode* parent = nullptr);
   virtual ~GameNode() = default;

   GameNode* getParent() const;


private:
   GameNode* mParent = nullptr;
};

