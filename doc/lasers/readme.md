# Laser concept

![Image of Yaktocat](example.png)

## Blue zone, frame 1 and 2
This is the warn notification.  
We basically loop between these 2 frames for 1.5 - 2.0 seconds so the player is notified that something will happen. The beam doesnt appear from nowhere, we must be fair.

## Purple zone (frame 3-10)
Charging state is active.   
I designed these frames in order to have a smooth appearance of the laser.

## Red zone (frame 11-17)
Laser hitbox is enabled.  
This is where this hazard does its damage, the frames are loopable

## Green zone (rest of frames)
Laser shuts down, hitbox is disabled.  
Again, I designed this for a smooth transition between the states.