#include "jumpplatform.h"

// https://youtu.be/O7C5I5UuGlQ?t=8s

JumpPlatform::JumpPlatform()
{

}

void JumpPlatform::touch()
{
   jumpsLeft--;

   if (jumpsLeft == 0)
   {
      dissolve();
   }
}


void JumpPlatform::dissolve()
{

}
