#include "preloader.h"

#include <iostream>

#include "detonationanimation.h"


void preloadDetonationAnimations()
{
   std::cout << "[x] preloading detonation animations" << std::endl;
   DetonationAnimation::getFrameData(DetonationAnimation::DetonationType::Big);
}


void Preloader::preload()
{
   preloadDetonationAnimations();
}
