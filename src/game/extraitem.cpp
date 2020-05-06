#include "extraitem.h"

#include <algorithm>

ExtraItem::ExtraItem(GameNode *parent)
 : GameNode(parent)
{
    setName(typeid(ExtraItem).name());
}

