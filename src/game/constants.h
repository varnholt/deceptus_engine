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

static constexpr auto PPM = 48.0f;         // pixels per meter
static constexpr auto MPP = (1.0f / PPM);  // meters per pixel
static constexpr auto TPM = 2.0f;          // tiles per meter
static constexpr auto MPT = 0.5f;          // meters per tile

static constexpr auto PIXELS_PER_TILE = 24;
static constexpr auto PIXELS_PER_HALF_TILE = PIXELS_PER_TILE / 2;
static constexpr auto PIXELS_PER_PHYSICS_TILE = 8;  // each tile is 8x8 px

static constexpr auto DIFF_PLAYER_TILE_TO_PHYSICS = 15;  // 20

static constexpr auto PLAYER_ANIMATION_CYCLES = 8;
static constexpr auto PLAYER_TILES_WIDTH = 24;
static constexpr auto PLAYER_TILES_HEIGHT = 48;
static constexpr auto PLAYER_ACTUAL_WIDTH = 20;   // the actual width can be smaller than the tile width
static constexpr auto PLAYER_ACTUAL_HEIGHT = 32;  // the actual height can be smaller than the tile height

static constexpr auto PLAYER_1_COLLISION_ID = 3;

static constexpr auto FACTOR_DEG_TO_RAD = 0.0174532925199432957f;
static constexpr auto FACTOR_RAD_TO_DEG = 57.295779513082320876f;

// configured timestep is 1/35
// frame update timestep is 1/60
// causes an error
//   pixel pos: 2808.000000, 8739.437500
//   pixel pos: 2808.000000, 8740.535156
// 8739.437500 - 8740.535156 = 1.097656
// 1 / 1.097656 => 0.91103223596463737272879663574016
static constexpr auto TIMESTEP_ERROR = 0.91192227210220912883854305376065f;

enum class DeathReason
{
   Invalid,
   Laser,
   OutOfHealth,
   Smashed,
   TooFast,
   TouchesDeadly,
};

enum class MechanismVersion
{
   Version1,
   Version2
};

// [50]
// [  ]
// [..] foreground layers in front of the player
// [  ]
// [21]
//
// [20] player
//
// [19]
// [..] foreground layers behind the player
// [16]
//
// [15]
// [..] background layers
// [03]
//
// [02]
// [..] parallax layers
// [00]
enum class ZDepth
{
   BackgroundMin = 0,
   BackgroundMax = 15,

   ForegroundMin = 16,
   ForegroundMax = 50,

   Doors = 40,
   Player = 20
};

enum class Alignment
{
   PointsNowhere = 0x00,
   PointsDown = 0x01,
   PointsUp = 0x02,
   PointsRight = 0x04,
   PointsLeft = 0x08,
};

enum class Display
{
   Invalid = 0x00,
   Game = 0x01,
   MainMenu = 0x02,
   Map = 0x04,
   IngameMenu = 0x08,
   Debug = 0x10,
   Modal = 0x20,
   ScreenTransition = 0x40,
};

enum class ItemType
{
   Invalid,
   KeyBlue,
   KeyOrange,
   KeyGreen,
   KeyRed,
   KeyYellow,
};

enum class ExecutionMode
{
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

enum class Look
{
   Inactive = 0x00,
   Active = 0x01,
   Up = 0x02,
   Down = 0x04,
   Left = 0x08,
   Right = 0x10,
};

enum KeyPressed
{
   KeyPressedUp = 0x01,
   KeyPressedDown = 0x02,
   KeyPressedLeft = 0x04,
   KeyPressedRight = 0x08,
   KeyPressedJump = 0x10,
   KeyPressedFire = 0x20,
   KeyPressedRun = 0x40,
   KeyPressedLook = 0x80,
};

enum ObjectType
{
   ObjectTypeInvalid,
   ObjectTypePlayer,
   ObjectTypePlayerFootSensor,
   ObjectTypePlayerHeadSensor,
   ObjectTypePlayerLeftArmSensor,
   ObjectTypePlayerRightArmSensor,
   ObjectTypeProjectile,
   ObjectTypeBouncer,
   ObjectTypeConveyorBelt,
   ObjectTypeCrusher,
   ObjectTypeDeadly,
   ObjectTypeDeathBlock,
   ObjectTypeDoor,
   ObjectTypeEnemy,
   ObjectTypeMoveableBox,
   ObjectTypeMovingPlatform,
   ObjectTypeSolid,
   ObjectTypeSolidOneWay,
   ObjectTypeBubbleCube,
   ObjectTypeCollapsingPlatform,
};

enum EntityCategory
{
   CategoryBoundary = 0x01,
   CategoryFriendly = 0x02,
   CategoryEnemyWalkThrough = 0x04,
   CategoryEnemyCollideWith = 0x08,
};

enum class MessageBoxLocation
{
   Invalid = 0,
   TopLeft,
   TopCenter,
   TopRight,
   MiddleLeft,
   MiddleCenter,
   MiddleRight,
   BottomLeft,
   BottomCenter,
   BottomRight,
};

enum class WeaponType
{
   None = 0,
   Bow = 1,
   Gun = 2,
   Sword = 3,
};

// this enum should be removed
enum AtmosphereTile  // 16 cols per row
{
   AtmosphereTileWaterFull = 48,
   AtmosphereTileWaterTop,
   AtmosphereTileWaterBottom,
   AtmosphereTileWaterLeft,
   AtmosphereTileWaterRight,
   AtmosphereTileWaterCornerTopRight,
   AtmosphereTileWaterCornerBottomRight,
   AtmosphereTileWaterCornerBottomLeft,
   AtmosphereTileWaterCornerTopLeft,

   AtmosphereTileInvalid = 1024,
};

enum class DrawMode
{
   ColorMap,
   NormalMap
};

enum class Edge
{
   None,
   Left,
   Right
};

enum class Dash
{
   None,
   Left,
   Right
};

// it might make more sense to remove game related stuff here and use a simple uint32_t _id
enum class CallbackType
{
   EndGame,
   NextLevel
};
