#include "preloader.h"

#include <iostream>

#include "framework/tools/log.h"
#include "game/animation/detonationanimation.h"

void preloadDetonationAnimations()
{
   Log::Info() << "preloading detonation animations";
   DetonationAnimation::getFrameData(DetonationAnimation::DetonationType::Big);
}


void Preloader::preload()
{
   preloadDetonationAnimations();
}
