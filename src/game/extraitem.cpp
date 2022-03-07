#include "extraitem.h"

#include <algorithm>

ExtraItem::ExtraItem(GameNode *parent)
 : GameNode(parent)
{
    setClassName(typeid(ExtraItem).name());
}

