#include "debugdrawstates.h"

bool DebugDrawStates::_draw_test_scene = false;
bool DebugDrawStates::_draw_console = false;
bool DebugDrawStates::_draw_debug_info = false;
bool DebugDrawStates::_draw_controller_overlay = false;
bool DebugDrawStates::_draw_camera_system = false;
bool DebugDrawStates::_draw_physics_config = false;
bool DebugDrawStates::_draw_log = false;
#ifdef DECEPTUS_MECHANISM_INSPECTOR
bool DebugDrawStates::_draw_mechanism_inspector = false;
#endif
bool DebugDrawStates::_draw_lighting = true;
