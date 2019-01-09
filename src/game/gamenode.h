#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H


class GameNode
{
public:
   GameNode(GameNode* parent = nullptr);
   virtual ~GameNode();

   GameNode *getParent() const;

   GameNode* mParent;
};

#endif // GAMEOBJECT_H
