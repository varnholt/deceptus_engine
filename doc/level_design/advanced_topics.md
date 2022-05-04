# Advanced Topics

## Checkpoints

Whenever Adam reaches a checkpoint, the current state of the game is saved. I.e. the player's current skills (such as double jump, wall slide, wall jump etc.) and the player's location within the level are serialized to disk. When the player dies later on, he would re-spawn at the last checkpoint.

Checkpoints are implemented as simple rectangle objects inside your level. In order to add checkpoints to your level, define an object group '`checkpoints`' and add rectangles inside this group that have reasonable names. The last checkpoint, i.e. the end of your level, must have the name '`end`'.

Checkpoints have the custom properties below:

|Custom Property|Type|Description|
|-|-|-|
|Object Name|string|If your Tiled 'Object Name' is called '`end`', the next level will be loaded after reaching the checkpoint.|
|index|int|The index of your checkpoint. The player will always respawn at the last reached checkpoint with the largest index.|
|sprite_pos_x_px|int|x position of the checkpoint sprite (given in px)|
|sprite_pos_y_px|int|y position of the checkpoint sprite (given in px)|
|z|int|The z index of the sprite layer|


![](images/checkpoints.png)


## Rooms

Usually the game's camera system keeps on following Adam so he always stays in focus. However, in Metroid-like games it's quite common to limit the camera range to one room, open a door, go to the next room and then move the camera's focus over to the other room.

If you are using rooms, it is important to make them at least as large as your screen (640x360px). Having rooms smaller than one screen would defeat the point of having rooms, hey?

If you define 2 rooms, the camera would limit the viewer's perspective to that room until the player has left that room. Then the camera would either move over to the next room or just focussing Adam if he entered a region where no room is defined.

Rooms are rectangles, or combinations of rectangles. In order to define a room, create an object group '`rooms`' first, then draw rectangles around those area that are supposed to be your rooms. Rooms should be given descriptive labels such as '`kitchen`' or '`bathroom`'. 

### Room Groups

If you want to merge multiple rects together to one larger room, you can also do that. Just insert a property named '`group`' with a group label such as '`bedrooms`' to each of your rectangles (even though no sane person would ever voluntarily merge their bedrooms).

![](images/rooms.png)

There can be screen transitions between rooms. For example, you may define a 'fade transition' for a room. That will fade the camera out, when the player just entered the room and the camera still focusses the 'old' room and then fade it when the new room is in focus. There are quite a few settings to give the user all liberty to set this up right. Once you've come up with a good set of settings, it makes sense to keep them consistent across all rooms.

### Properties per room group

The values below should be identical for all rectangles of one room group.

|Custom Property|Type|Description|
|-|-|-|
|group|string|The group identifier which can be used to group multiple rectangles to a larger room group. The default is an empty string.|
|transition|string|The type of the transition. At the moment only the type `fade_out_fade_in` is supported (optional).|
|fade_out_speed|float|An factor used to adjust the speed when fading out (optional). `1.0` is slow fading, `2.0` is rather fast. The default is `2.0`.|
|fade_in_speed|float|A factor used to adjust the speed when fading in (optional). `1.0` is slow fading, `2.0` is rather fast. The default is `2.0`.|
|delay_between_effects_ms|int|The delay in ms to be elapsed between both parts (e.g. 'fade out', 'fade in') of the transition (optional). The default value is `250`.|
|camera_sync_after_fade_out|bool|Move the camera focus to the new room immediately after fading out; the new room is then focussed when fading back in (optional). The default value is `true`.|
|camera_lock_delay_ms|int|This value will 'hold' the camera focus in the old room, even though a new room has been entered already. Once the defined time in milliseconds is elapsed, the new room is focussed. The default value is `0`. <br> If `camera_sync_after_fade_out` is set to `true`, it'll override this setting and synchronize the camera position as promised. Actually these two settings work very well together. If you are uncertain what value to put here, you can just choose a rather long duration (1000ms) and enable `camera_sync_after_fade_out`. <br>This will lock the camera in the old room once the player entered a new room. Then, when the screen is black, the camera focus will be moved to the new room (the camera lock is released) and the new room is shown when fading in. It all sounds more complicated than it is. Just try a configuration like this and play around with it:<br>![](images/rooms_settings.png)|

### Properties for each room rectangle

The properties below may differ for each rectangle within one room group.

|Custom Property|Type|Description|
|-|-|-|
|start_position_left_x_px|int|When player entering from the left of the room, position him to the absolute x position, given in pixels. Also need to provide `start_position_left_y_px`|
|start_position_left_y_px|int|When player entering from the left of the room, position him to the absolute y position, given in pixels. Also need to provide `start_position_left_x_px`|
|start_position_right_x_px|int|When player entering from the right of the room, position him to the absolute x position, given in pixels. Also need to provide `start_position_right_y_px`|
|start_position_right_y_px|int|When player entering from the right of the room, position him to the absolute y position, given in pixels. Also need to provide `start_position_right_x_px`|
|start_position_top_x_px|int|When player entering from the top of the room, position him to the absolute x position, given in pixels. Also need to provide `start_position_top_y_px`|
|start_position_top_y_px|int|When player entering from the top of the room, position him to the absolute y position, given in pixels. Also need to provide `start_position_top_x_px`|
|start_position_bottom_x_px|int|When player entering from the bottom of the room, position him to the absolute x position, given in pixels. Also need to provide `start_position_bottom_y_px`|
|start_position_bottom_y_px|int|When player entering from the bottom of the room, position him to the absolute y position, given in pixels. Also need to provide `start_position_bottom_x_px`|
|start_offset_left_x_px|int|When player entering from the left of the room, position him to the relative x offset, given in pixels. The y counterpart `start_offset_left_y_px` is optional.|
|start_offset_left_y_px|int|When player entering from the left of the room, position him to the relative x offset, given in pixels. The x counterpart `start_offset_right_x_px` must be provided.|
|start_offset_right_x_px|int|When player entering from the right of the room, position him to the relative x offset, given in pixels. The y counterpart `start_offset_right_y_px` is optional.|
|start_offset_right_y_px|int|When player entering from the right of the room, position him to the relative x offset, given in pixels. The x counterpart `start_offset_right_x_px` must be provided.|

# Addendum

## Folder Structure

Here's an overview of the game's folder structure and some guidance how to set up the file and folder structure of your level.

- `📁 root`
  - `📁 data`
    - `📁 config`: This folder contains all the game's configuration files
    - `📁 effects`: You can store texture custom graphics effects here
    - `📁 fonts`: Truetype and bitmap fonts go here
    - `📁 game`: In-game overlays and menus
    - `📁 joystick`: The SDL game controller database
    - `📁 level-your_level_name`
      - `📄 your_level_name.json`
      - `📄 your_level_name.tmx`
      - 📄 all your `.tsx` files
      - 📄 your ambient occlusion files
      - `📁 images`:  Images for your image layers
      - `📁 tilesets`: Tiles used inside your tmx/tsx files
    - `📁 light`: Your light textures for static and dynamic lights
    - `📁 menus`: All the menu images are stored here.
    - `📁 music`: Well...
    - `📁 scenes`: Image for in-game cut scenes etc.
    - `📁 scripts`
      - `📁 enemies`: All the lua scripts for the game's enemies
    - `📁 shaders`: All shaders used by the Deceptus Engine
    - `📁 sounds`: All sounds used by the engine as well as referened by the lua scripts
    - `📁 sprites`: All enemy sprites, i.e. all sprites that do not depend on a particular level design
    - `📁 weapons`: Weapon-related sprites
  - `📁 doc`
    - `📁 game_physics`: Information about the game physics (such as jump behavior, etc.)
    - `📁 level_design`: This is what you are staring at right now
    - `📁 lua_interface`: A description of the lua interface used by the game's enemies
  - `📁 tools`
    - `📁 generate_ao`: A tool that transforms a colored image into an AO texture
    - `📁 pack_texture`: A program that strips empty areas from textures and creates a new texture plus UVs
    - `📁 path_merge`: A tool that eliminates redundant vertices and generates an optimized coherent mesh
    - `📁 tmx_rasterizer`: A program that extracts layers from a TMX file and generates a single image out of it
