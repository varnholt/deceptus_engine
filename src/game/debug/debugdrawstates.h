#ifndef DEBUGDRAWSTATES_H
#define DEBUGDRAWSTATES_H

/// \brief holds global toggles that enable or disable optional debug overlays.
struct DebugDrawStates
{
   static bool _draw_test_scene;
   static bool _draw_console;
   static bool _draw_debug_info;
   static bool _draw_controller_overlay;
   static bool _draw_camera_system;
   static bool _draw_physics_config;
   static bool _draw_log;

   //! \brief when false the deferred lighting pass is bypassed and the level is drawn unlit
   static bool _draw_lighting;
};  // namespace DrawStates

#endif  // DEBUGDRAWSTATES_H
