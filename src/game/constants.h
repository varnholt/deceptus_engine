#pragma once

// How do I convert pixels to meters?
//
// Suppose you have a sprite for a character that is 100x100 pixels. You
// decide to use a scaling factor that is 0.01. This will make the character
// physics box 1m x 1m. So go make a physics box that is 1x1. Now suppose
// the character starts out at pixel coordinate (345,679). So position the
// physics box at (3.45,6.79). Now simulate the physics world. Suppose the
// character physics box moves to (2.31,4.98), so move your character sprite
// to pixel coordinates (231,498). Now the only tricky part is choosing a
// scaling factor. This really depends on your game. You should try to get
// your moving objects in the range 0.1 - 10 meters, with 1 meter being the
// spot.


#define GAME_NAME "deceptus"

#define PPM 48.0f
#define MPP (1.0f/PPM)

#define PIXELS_PER_TILE 24
#define PIXELS_PER_PHYSICS_TILE 8 // each tile is 8x8 px

#define DIFF_PLAYER_TILE_TO_PHYSICS 15 // 20

#define PLAYER_ANIMATION_CYCLES 8
#define PLAYER_TILES_WIDTH  24
#define PLAYER_TILES_HEIGHT 48
#define PLAYER_ACTUAL_WIDTH  20 // the actual width can be smaller than the tile width
#define PLAYER_ACTUAL_HEIGHT 32 // the actual height can be smaller than the tile height

#define PLAYER_1_COLLISION_ID 3

#ifndef DEGTORAD
#define DEGTORAD 0.0174532925199432957f
#define RADTODEG 57.295779513082320876f
#endif


enum Alignment {
  PointsNowhere = 0x00,
  PointsDown    = 0x01,
  PointsUp      = 0x02,
  PointsRight   = 0x04,
  PointsLeft    = 0x08,
};

enum Display {
  DisplayInvalid   = 0x00,
  DisplayGame      = 0x01,
  DisplayMainMenu  = 0x02,
  DisplayMap       = 0x04,
  DisplayInventory = 0x08,
  DisplayDebug     = 0x10,
};

enum class ItemType {
   Invalid,
   Crowbar,
   Dynamite,
   KeyBlue,
   KeyCrypt,
   KeyGreen,
   KeyRed,
   KeySkull,
   KeyYellow,
   Knife,
   LoveBomb,
   Match,
   Money,
   Saw,
   Shovel,
};

enum class ExecutionMode {
   None,
   Running,
   Paused,
};

enum class MenuAction
{
  Confirm,
  Cancel,
  MoveUp,
  MoveDown,
  Decrease,
  Increase,
};

enum class PlayerAction
{
  None,
  Jump,
  Action,
  Shoot,
  Inventory,
  EnterDoor,
  MoveLeft,
  MoveRight,
  Crouch,
  DropPlatform,
  LookAround,
  DoubleJump,
  Swim,
  DashLeft,
  DashRight,
  WallJump,
};

enum SdlControllerButton
{
  SdlControllerButtonNone             = 0x0,
  SdlControllerButtonA                = 0x1,
  SdlControllerButtonB                = 0x2,
  SdlControllerButtonX                = 0x4,
  SdlControllerButtonY                = 0x8,
  SdlControllerButtonBack             = 0x10,
  SdlControllerButtonGuide            = 0x20,
  SdlControllerButtonStart            = 0x40,
  SdlControllerButtonLeftStick        = 0x80,
  SdlControllerButtonRightStick       = 0x100,
  SdlControllerButtonLeftShoulder     = 0x200,
  SdlControllerButtonRightShoulder    = 0x400,
  SdlControllerButtonDpadUp           = 0x800,
  SdlControllerButtonDpadDown         = 0x1000,
  SdlControllerButtonDpadLeft         = 0x2000,
  SdlControllerButtonDpadRight        = 0x4000,
};

enum class InvetoryAction
{
  ShowMap,
  Pause,
  Confirm,
  Cancel,
  PreviousPage,
  NextPage,
  MoveUp,
  MoveDown,
  MoveRight,
  MoveLeft,
};

enum Look {
  LookInactive = 0x0,
  LookActive   = 0x1,
  LookUp       = 0x2,
  LookDown     = 0x4,
  LookLeft     = 0x8,
  LookRight    = 0x10,
};

enum KeyPressed {
   KeyPressedUp    = 0x01,
   KeyPressedDown  = 0x02,
   KeyPressedLeft  = 0x04,
   KeyPressedRight = 0x08,
   KeyPressedJump  = 0x10,
   KeyPressedFire  = 0x20,
   KeyPressedRun   = 0x40,
   KeyPressedLook  = 0x80,
};

enum AtmosphereTile // 16 cols per row
{
   // PhysicsTileSolidFull = 0,
   // PhysicsTileSolidTop,
   // PhysicsTileSolidBottom,
   // PhysicsTileSolidLeft,
   // PhysicsTileSolidRight,
   // PhysicsTileSolidCornerTopRight,
   // PhysicsTileSolidCornerBottomRight,
   // PhysicsTileSolidCornerBottomLeft,
   // PhysicsTileSolidCornerTopLeft,
   //
   // PhysicsTileOneSidedFull = 16,
   // PhysicsTileOneSidedTop,
   // PhysicsTileOneSidedBottom,
   // PhysicsTileOneSidedLeft,
   // PhysicsTileOneSidedRight,
   // PhysicsTileOneSidedCornerTopRight,
   // PhysicsTileOneSidedCornerBottomRight,
   // PhysicsTileOneSidedCornerBottomLeft,
   // PhysicsTileOneSidedCornerTopLeft,
   //
   // PhysicsTileDeadlyFull = 32,
   // PhysicsTileDeadlyTop,
   // PhysicsTileDeadlyBottom,
   // PhysicsTileDeadlyLeft,
   // PhysicsTileDeadlyRight,
   // PhysicsTileDeadlyCornerTopRight,
   // PhysicsTileDeadlyCornerBottomRight,
   // PhysicsTileDeadlyCornerBottomLeft,
   // PhysicsTileDeadlyCornerTopLeft,

   AtmosphereTileWaterFull = 48,
   AtmosphereTileWaterTop,
   AtmosphereTileWaterBottom,
   AtmosphereTileWaterLeft,
   AtmosphereTileWaterRight,
   AtmosphereTileWaterCornerTopRight,     // not used
   AtmosphereTileWaterCornerBottomRight,  // not used
   AtmosphereTileWaterCornerBottomLeft,   // not used
   AtmosphereTileWaterCornerTopLeft,      // not used

   AtmosphereTileInvalid = 1024,
};

enum ObjectBehavior
{
   ObjectBehaviorSolid = 1,
   ObjectBehaviorDeadly,
};

enum ObjectType
{
   ObjectTypePlayer,
   ObjectTypePlayerFootSensor,
   ObjectTypePlayerHeadSensor,
   ObjectTypeBullet,
   ObjectTypeOneSidedWall,
   ObjectTypeDeadly,
   ObjectTypeMovingPlatform,
   ObjectTypeDoor,
   ObjectTypeEnemy,
   ObjectTypeBouncer,
   ObjectTypeConveyorBelt,
   ObjectTypeJumpPlatform,
   ObjectTypeMoveableBox,
};

enum EntityCategory {
   CategoryBoundary         = 0x01,
   CategoryFriendly         = 0x02,
   CategoryEnemyWalkThrough = 0x04,
   CategoryEnemyCollideWith = 0x08,
};

