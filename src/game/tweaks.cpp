#include "tweaks.h"


const Tweaks& Tweaks::instance()
{
   static Tweaks _instance;
   return _instance;
}
