# Designing a Level

Levels in Deceptus use the Tiled Editor file format _tmx_.
In order to make your first level you will need two things:
Tiled and a solid sprite sheet, also called tileset.

## Some Terms
A _sprite sheet_ is a big texture (an image) that contains all the elements that you want to use in your level. Those are called _sprites_.<br>
You can use as many sprite sheets as you like, however Deceptus only supports one sprite sheet per _layer_.
A layer is something like a transparent sheet where you can draw things on. And you can put as many of these sheets on top of each other, all combined resulting into one final image.<br>
Likewise, your level can consist of many layers put on top of each other.
So you can have a layer for things that should always be in the foreground, another layer for all the stuff that's directly located around our game character and another layer for the background. While writing this, our hero does not have a name yet, so let's just call him Adam for now.

## Your Tileset

Each game defines a particular tile size (the width and height) of a tile in the sprite sheet. Deceptus went for `24x24px`. If you want to go for a different tile size, you'd have to change the code of the game (`constants.h`).<br>
Furthermore, we have decided that 2 tiles should represent 1m in the 'real world'. This is relevant for the physics behavior of the game. So it's good to keep in mind that 48 pixels are equivalent to 1 meter. This constant can also be altered in the game code if needed.

## Your Tiles

Deceptus differentiates between _colliding_ and _non-colliding_ tiles. The latter are tiles that are just drawn onto the screen. They don't have any other influence on the behavior of the game. Colliding tiles on the other hand are those tiles Adam is standing on or bouncing into such as the floor, the walls, etc.<br>
Tiled comes with a 'Tile Collision Editor' where you can define the shape of each tile. This is either a simple rectangle, but it can also be a more complex polygon. That's really up to your level design. In any case, defining the shape of the colliding tiles is probably the first chore that you have to do when starting the design of a level.

_insert  image of Tile Collision Editor_

Keep in mind that the shapes used for each tile will be merged into one big 2D model ('mesh') later on. The merging step only works if the polygons are really accurate. The mesh optimizer used here will create 2 edges instead of 1 if a point of your polygon (vertex) is off by just 1px. To make sure that your vertices really align with each other, it is recommended to open up the tmx file in a text editor and compare if the values match:

```xml
  <tile id="74">
   <objectgroup draworder="index" id="2">
    <object id="1" x="0" y="0" width="24" height="24"/>
   </objectgroup>
  </tile>
  <tile id="75">
   <objectgroup draworder="index" id="2">
    <object id="1" x="0" y="0" width="24" height="24"/>
   </objectgroup>
  </tile>
```

## Level Layers

Custom Properties
z

bg0
bg1
bg2
bg3

```
enum ZDepth
{
   ZDepthBackgroundMin = 0,
   ZDepthBackgroundMax = 15,

   ZDepthForegroundMin = 16,
   ZDepthForegroundMax = 50,

   ZDepthDoors = 40,
   ZDepthPlayer = 16
};


```

### Parallax Layers
```
   float mParallaxFactors[3] = {0.9f, 0.85f, 0.8f};
```




# Mechanisms

## Bouncers

Object layer: bouncers

## Conveyor belts

Object layer: conveyorbelts

## Doors

Tile layer: doors


## Extras

Tile layer: extras


## Fans

Tile layer: fans
Object layer: fans


## Lasers

Tile layer: lasers
Object layer: lasers

Have one rectangle per 'laser unit'.
Snap rectangle to tiles.

Properties

|Name|Type|
|-|-|
|off_time|int (ms)|
|on_time|int (ms)|


## Platforms

Tile layer: platforms
Rail layer: platform_rails
Object layer: platforms

## Portals

Tile layer: portals
Object layer: portals


# Visualization

## Backgrounds

## Parallax

## Foregrounds

## Static images

## Static lights

## Dynamic lights

## Shadows

## Atmosphere layers


# Enemies



# Advanced Topics

## Rooms
