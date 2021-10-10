#include "preloader.h"

#include <iostream>

#include "detonationanimation.h"
#include "framework/tools/log.h"


void preloadDetonationAnimations()
{
   Log::Info() << "preloading detonation animations";
   DetonationAnimation::getFrameData(DetonationAnimation::DetonationType::Big);
}


void Preloader::preload()
{
   preloadDetonationAnimations();
}
