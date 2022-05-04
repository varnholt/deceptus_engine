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

![](images/tile_collision_editor.png)

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

### A First Test Run

The first thing you want to do next is to define the colliding layer of your level because once you've done that, you are actually able to try it out inside the game engine.

This layer _must_ be called 'level' and it should have a custom property of type `int` with the value `15`. More on that later.

When you save the tmx file the game will evaluate all the collision information that you defined in the Collision Editor, merge that to one large mesh and pass that to the physics engine.

Now you've got to define a JSON file that describes the most basic information about your level, such as
- the filename of your level
- Adam's start position

To do so, create a subdirectory inside the game's `data` folder such as `level-awesome` and copy your tmx file and all related files such as your sprite sheets etc. to this folder. Now create a file called `level.json` that looks like the one below:

```json
{
  "filename": "data/level-awesome/my_awesome_level.tmx",
  "startposition": [45, 27]
}
```

The `filename` property just points to your tmx file.<br>
The `startposition` property is a x, y array that defines Adam's start position. This position is in tile coordinates. You can see the tile coordinates at Tiled's status bar at the bottom. This is why it's been decided that tile coordinates are more convenient than pixel coordinates.

The last step needed to be finally able to play your first level is to define the order of your levels. For that purpose, open up `data/config/levels.json` and create a section for your level. If you put it first, it will load right after you start a new game:

```json
[
   {
      "levelname": "data/level-awesome/level.json"
   },
   ...
]
```

Now you're all set! Go and try out your first level!

Check out the addendum for more info about the overall folder structure of the Deceptus Engine.


### Adding More Layers

Of course, having a single layer level will look a bit dull so you're probably interested in how to add layers to the foreground and background.

Deceptus currently supports 51 layers while the layer furthest away from your eye (in the background) has the z coordinate `0` and the frontmost layer has the z coordinate `50`. Adam moves around on `z = 16`. If you take a look at the previous paragraph, you'll notice that this is right on top of the level layer.

Deceptus could just use the layer order that you define inside your tmx file by moving layers up and down. However, the game engine wants to know the exact z coordinate of each layer since that's much more predictable than auto-computing the z coordinate from your layer stack inside Tiled. This is why each layer should have z custom property `z` of type `int`.

|Custom Property|Type|Description|
|-|-|-|
|z|int|The z depth of your layer from 0 (far far away) to 50 (frontmost)|

Wait, there's more! The Deceptus Engine supports a bunch of 'custom' layers such as _Parallax_ or _Image Layers_ You won't need any of that during your early design steps but once your gameplay is solid, feel free to move on to the chapter 'Visualization'.

