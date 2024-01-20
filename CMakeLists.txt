cmake_minimum_required(VERSION 3.16)
project(deceptus VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)

include(GNUInstallDirs)

set(GAME_FILES
    src/framework/image/image.cpp src/framework/image/image.h
    src/framework/image/layer.cpp src/framework/image/layer.h
    src/framework/image/psd.cpp src/framework/image/psd.h
    src/framework/image/tga.cpp src/framework/image/tga.h
    src/framework/joystick/gamecontroller.cpp src/framework/joystick/gamecontroller.h
    src/framework/joystick/gamecontrollerballvector.h
    src/framework/joystick/gamecontrollerinfo.cpp src/framework/joystick/gamecontrollerinfo.h
    src/framework/math/box2dtools.cpp src/framework/math/box2dtools.h
    src/framework/math/fbm.cpp src/framework/math/fbm.h
    src/framework/math/hermitecurve.cpp src/framework/math/hermitecurve.h
    src/framework/math/hermitecurvekey.h
    src/framework/math/maptools.cpp src/framework/math/maptools.h
    src/framework/math/pathinterpolation.h
    src/framework/math/sfmlmath.cpp src/framework/math/sfmlmath.h
    src/framework/tmxparser/tmxanimation.cpp src/framework/tmxparser/tmxanimation.h
    src/framework/tmxparser/tmxchunk.cpp src/framework/tmxparser/tmxchunk.h
    src/framework/tmxparser/tmxelement.cpp src/framework/tmxparser/tmxelement.h
    src/framework/tmxparser/tmxframe.cpp src/framework/tmxparser/tmxframe.h
    src/framework/tmxparser/tmximage.cpp src/framework/tmxparser/tmximage.h
    src/framework/tmxparser/tmximagelayer.cpp src/framework/tmxparser/tmximagelayer.h
    src/framework/tmxparser/tmxlayer.cpp src/framework/tmxparser/tmxlayer.h
    src/framework/tmxparser/tmxobject.cpp src/framework/tmxparser/tmxobject.h
    src/framework/tmxparser/tmxobjectgroup.cpp src/framework/tmxparser/tmxobjectgroup.h
    src/framework/tmxparser/tmxparsedata.h
    src/framework/tmxparser/tmxparser.cpp src/framework/tmxparser/tmxparser.h
    src/framework/tmxparser/tmxpolygon.cpp src/framework/tmxparser/tmxpolygon.h
    src/framework/tmxparser/tmxpolyline.cpp src/framework/tmxparser/tmxpolyline.h
    src/framework/tmxparser/tmxproperties.cpp src/framework/tmxparser/tmxproperties.h
    src/framework/tmxparser/tmxproperty.cpp src/framework/tmxparser/tmxproperty.h
    src/framework/tmxparser/tmxtemplate.cpp src/framework/tmxparser/tmxtemplate.h
    src/framework/tmxparser/tmxtile.cpp src/framework/tmxparser/tmxtile.h
    src/framework/tmxparser/tmxtileset.cpp src/framework/tmxparser/tmxtileset.h
    src/framework/tmxparser/tmxtools.cpp src/framework/tmxparser/tmxtools.h
    src/framework/tools/callbackmap.cpp
    src/framework/tools/checksum.cpp
    src/framework/tools/elapsedtimer.cpp src/framework/tools/elapsedtimer.h
    src/framework/tools/globalclock.cpp
    src/framework/tools/jsonconfiguration.cpp src/framework/tools/jsonconfiguration.h
    src/framework/tools/log.cpp src/framework/tools/log.h
    src/framework/tools/logthread.cpp src/framework/tools/logthread.h
    src/framework/tools/scopeexit.cpp src/framework/tools/scopeexit.h
    src/framework/tools/stopwatch.cpp src/framework/tools/stopwatch.h
    src/framework/tools/timer.cpp
    src/game/ambientocclusion.cpp src/game/ambientocclusion.h
    src/game/animation.cpp src/game/animation.h
    src/game/animationframedata.cpp src/game/animationframedata.h
    src/game/animationplayer.cpp src/game/animationplayer.h
    src/game/animationpool.cpp src/game/animationpool.h
    src/game/animationsettings.cpp src/game/animationsettings.h
    src/game/arrow.cpp src/game/arrow.h
    src/game/atmosphere.cpp src/game/atmosphere.h
    src/game/audio.cpp src/game/audio.h
    src/game/audiorange.cpp src/game/audiorange.h
    src/game/audioupdatedata.cpp src/game/audioupdatedata.h
    src/game/bitmapfont.cpp src/game/bitmapfont.h
    src/game/boomeffect.cpp src/game/boomeffect.h
    src/game/boomeffectenvelope.cpp src/game/boomeffectenvelope.h
    src/game/boomeffectenveloperandom.cpp src/game/boomeffectenveloperandom.h
    src/game/boomeffectenvelopesine.cpp src/game/boomeffectenvelopesine.h
    src/game/boomsettings.cpp src/game/boomsettings.h
    src/game/bow.cpp src/game/bow.h
    src/game/camerapanorama.cpp src/game/camerapanorama.h
    src/game/cameraroomlock.cpp src/game/cameraroomlock.h
    src/game/camerasystem.cpp src/game/camerasystem.h
    src/game/camerasystemconfiguration.cpp src/game/camerasystemconfiguration.h
    src/game/camerasystemconfigurationui.cpp src/game/camerasystemconfigurationui.h
    src/game/chainshapeanalyzer.cpp src/game/chainshapeanalyzer.h
    src/game/chunk.cpp src/game/chunk.h
    src/game/console.cpp src/game/console.h
    src/game/constants.h
    src/game/cutscene.cpp src/game/cutscene.h
    src/game/debugdraw.cpp src/game/debugdraw.h
    src/game/detonationanimation.cpp src/game/detonationanimation.h
    src/game/displaymode.cpp src/game/displaymode.h
    src/game/drawstates.cpp src/game/drawstates.h
    src/game/effects/lightsystem.cpp
    src/game/enemydescription.cpp src/game/enemydescription.h
    src/game/eventdistributor.cpp src/game/eventdistributor.h
    src/game/eventserializer.cpp src/game/eventserializer.h
    src/game/extratable.cpp src/game/extratable.h
    src/game/fadetransitioneffect.cpp src/game/fadetransitioneffect.h
    src/game/fixturenode.cpp src/game/fixturenode.h
    src/game/forestscene.cpp src/game/forestscene.h
    src/game/framemapper.cpp src/game/framemapper.h
    src/game/game.cpp src/game/game.h
    src/game/gameaudio.cpp src/game/gameaudio.h
    src/game/gameclock.cpp src/game/gameclock.h
    src/game/gameconfiguration.cpp src/game/gameconfiguration.h
    src/game/gamecontactlistener.cpp src/game/gamecontactlistener.h
    src/game/gamecontrollerdata.cpp src/game/gamecontrollerdata.h
    src/game/gamecontrollerdetection.cpp src/game/gamecontrollerdetection.h
    src/game/gamecontrollerintegration.cpp src/game/gamecontrollerintegration.h
    src/game/gamedeserializedata.cpp src/game/gamedeserializedata.h
    src/game/gamemechanism.cpp src/game/gamemechanism.h
    src/game/gamemechanismdeserializer.cpp src/game/gamemechanismdeserializer.h
    src/game/gamemechanismdeserializerconstants.h
    src/game/gamenode.cpp src/game/gamenode.h
    src/game/gamestate.cpp src/game/gamestate.h
    src/game/gun.cpp src/game/gun.h
    src/game/health.cpp src/game/health.h
    src/game/hitbox.cpp src/game/hitbox.h
    src/game/imagelayer.cpp src/game/imagelayer.h
    src/game/infolayer.cpp src/game/infolayer.h
    src/game/ingamemenu.cpp src/game/ingamemenu.h
    src/game/ingamemenuarchives.cpp src/game/ingamemenuarchives.h
    src/game/ingamemenuaudio.cpp src/game/ingamemenuaudio.h
    src/game/ingamemenuinventory.cpp src/game/ingamemenuinventory.h
    src/game/ingamemenumap.cpp src/game/ingamemenumap.h
    src/game/ingamemenupage.cpp src/game/ingamemenupage.h
    src/game/inventory.cpp src/game/inventory.h
    src/game/inventorybasedcontrols.cpp src/game/inventorybasedcontrols.h
    src/game/inventoryitemdescriptionreader.cpp src/game/inventoryitemdescriptionreader.h
    src/game/layerdata.cpp src/game/layerdata.h
    src/game/level.cpp src/game/level.h
    src/game/leveldescription.cpp src/game/leveldescription.h
    src/game/levelfiles.cpp src/game/levelfiles.h
    src/game/levels.cpp src/game/levels.h
    src/game/levelscript.cpp src/game/levelscript.h
    src/game/luaconstants.h
    src/game/luainterface.cpp src/game/luainterface.h
    src/game/luanode.cpp src/game/luanode.h
    src/game/mechanisms/blockingrect.cpp src/game/mechanisms/blockingrect.h
    src/game/mechanisms/bouncer.cpp src/game/mechanisms/bouncer.h
    src/game/mechanisms/bouncerwrapper.cpp src/game/mechanisms/bouncerwrapper.h
    src/game/mechanisms/bubblecube.cpp src/game/mechanisms/bubblecube.h
    src/game/mechanisms/checkpoint.cpp src/game/mechanisms/checkpoint.h
    src/game/mechanisms/collapsingplatform.cpp src/game/mechanisms/collapsingplatform.h
    src/game/mechanisms/controllerhelp.cpp src/game/mechanisms/controllerhelp.h
    src/game/mechanisms/conveyorbelt.cpp src/game/mechanisms/conveyorbelt.h
    src/game/mechanisms/crusher.cpp src/game/mechanisms/crusher.h
    src/game/mechanisms/damagerect.cpp src/game/mechanisms/damagerect.h
    src/game/mechanisms/deathblock.cpp src/game/mechanisms/deathblock.h
    src/game/mechanisms/dialogue.cpp src/game/mechanisms/dialogue.h
    src/game/mechanisms/door.cpp src/game/mechanisms/door.h
    src/game/mechanisms/dust.cpp src/game/mechanisms/dust.h
    src/game/mechanisms/extra.cpp src/game/mechanisms/extra.h
    src/game/mechanisms/fan.cpp src/game/mechanisms/fan.h
    src/game/mechanisms/fireflies.cpp src/game/mechanisms/fireflies.h
    src/game/mechanisms/laser.cpp
    src/game/mechanisms/laser.h
    src/game/mechanisms/lever.cpp src/game/mechanisms/lever.h
    src/game/mechanisms/levermechanismmerger.cpp src/game/mechanisms/levermechanismmerger.h
    src/game/mechanisms/moveablebox.cpp src/game/mechanisms/moveablebox.h
    src/game/mechanisms/movingplatform.cpp src/game/mechanisms/movingplatform.h
    src/game/mechanisms/onoffblock.cpp src/game/mechanisms/onoffblock.h
    src/game/mechanisms/portal.cpp src/game/mechanisms/portal.h
    src/game/mechanisms/portalwrapper.cpp src/game/mechanisms/portalwrapper.h
    src/game/mechanisms/rope.cpp src/game/mechanisms/rope.h
    src/game/mechanisms/ropewithlight.cpp src/game/mechanisms/ropewithlight.h
    src/game/mechanisms/rotatingblade.cpp src/game/mechanisms/rotatingblade.h
    src/game/mechanisms/sensorrect.cpp src/game/mechanisms/sensorrect.h
    src/game/mechanisms/shaderlayer.cpp src/game/mechanisms/shaderlayer.h
    src/game/mechanisms/smokeeffect.cpp
    src/game/mechanisms/soundemitter.cpp src/game/mechanisms/soundemitter.h
    src/game/mechanisms/spikeball.cpp src/game/mechanisms/spikeball.h
    src/game/mechanisms/spikeblock.cpp src/game/mechanisms/spikeblock.h
    src/game/mechanisms/spikes.cpp src/game/mechanisms/spikes.h
    src/game/mechanisms/staticlight.cpp src/game/mechanisms/staticlight.h
    src/game/mechanisms/waterdamage.cpp src/game/mechanisms/waterdamage.h
    src/game/mechanisms/watersurface.cpp src/game/mechanisms/watersurface.h
    src/game/mechanisms/weather.cpp src/game/mechanisms/weather.h
    src/game/menuconfig.cpp src/game/menuconfig.h
    src/game/meshtools.cpp src/game/meshtools.h
    src/game/messagebox.cpp src/game/messagebox.h
    src/game/onewaywall.cpp src/game/onewaywall.h
    src/game/overlays/controlleroverlay.cpp src/game/overlays/controlleroverlay.h
    src/game/overlays/rainoverlay.cpp src/game/overlays/rainoverlay.h
    src/game/overlays/thunderstormoverlay.cpp src/game/overlays/thunderstormoverlay.h
    src/game/overlays/weatheroverlay.cpp src/game/overlays/weatheroverlay.h
    src/game/parallaxlayer.cpp src/game/parallaxlayer.h
    src/game/parallaxsettings.cpp src/game/parallaxsettings.h
    src/game/parsedata.cpp src/game/parsedata.h
    src/game/physics/physics.cpp src/game/physics/physics.h
    src/game/physics/physicsconfiguration.cpp src/game/physics/physicsconfiguration.h
    src/game/physics/physicsconfigurationui.cpp src/game/physics/physicsconfigurationui.h
    src/game/player/player.cpp src/game/player/player.h
    src/game/player/playeranimation.cpp src/game/player/playeranimation.h
    src/game/player/playerattack.h
    src/game/player/playeraudio.cpp src/game/player/playeraudio.h
    src/game/player/playerbelt.cpp src/game/player/playerbelt.h
    src/game/player/playerbend.h
    src/game/player/playerclimb.cpp src/game/player/playerclimb.h
    src/game/player/playercontrols.cpp src/game/player/playercontrols.h
    src/game/player/playerdash.cpp src/game/player/playerdash.h
    src/game/player/playerinfo.cpp src/game/player/playerinfo.h
    src/game/player/playerinput.cpp src/game/player/playerinput.h
    src/game/player/playerjump.cpp src/game/player/playerjump.h
    src/game/player/playerjumptrace.h
    src/game/player/playerspeed.h
    src/game/player/timerlock.cpp src/game/player/timerlock.h
    src/game/playerstencil.cpp src/game/playerstencil.h
    src/game/playerutils.cpp src/game/playerutils.h
    src/game/preloader.cpp src/game/preloader.h
    src/game/projectile.cpp src/game/projectile.h
    src/game/projectilehitanimation.cpp src/game/projectilehitanimation.h
    src/game/projectilehitaudio.cpp src/game/projectilehitaudio.h
    src/game/room.cpp src/game/room.h
    src/game/roomupdater.cpp src/game/roomupdater.h
    src/game/savestate.cpp src/game/savestate.h
    src/game/screentransition.cpp src/game/screentransition.h
    src/game/screentransitioneffect.cpp src/game/screentransitioneffect.h
    src/game/scriptproperty.cpp src/game/scriptproperty.h
    src/game/shaders/atmosphereshader.cpp src/game/shaders/atmosphereshader.h
    src/game/shaders/blurshader.cpp src/game/shaders/blurshader.h
    src/game/shaders/deathshader.cpp src/game/shaders/deathshader.h
    src/game/shaders/gammashader.cpp src/game/shaders/gammashader.h
    src/game/skill.cpp src/game/skill.h
    src/game/squaremarcher.cpp src/game/squaremarcher.h
    src/game/stenciltilemap.cpp src/game/stenciltilemap.h
    src/game/sword.cpp src/game/sword.h
    src/game/test.cpp src/game/test.h
    src/game/texturepool.cpp src/game/texturepool.h
    src/game/tilemap.cpp src/game/tilemap.h
    src/game/tilemapfactory.cpp src/game/tilemapfactory.h
    src/game/tmxenemy.cpp src/game/tmxenemy.h
    src/game/tweaks.cpp src/game/tweaks.h
    src/game/volumeupdater.cpp src/game/volumeupdater.h
    src/game/waterbubbles.cpp src/game/waterbubbles.h
    src/game/weapon.cpp src/game/weapon.h
    src/game/weaponfactory.cpp src/game/weaponfactory.h
    src/game/weaponproperties.cpp src/game/weaponproperties.h
    src/game/weaponsystem.cpp src/game/weaponsystem.h
    src/game/worldquery.cpp src/game/worldquery.h
    src/main.cpp
    src/menus/menu.cpp src/menus/menu.h
    src/menus/menuaudio.cpp src/menus/menuaudio.h
    src/menus/menuscreen.cpp src/menus/menuscreen.h
    src/menus/menuscreenachievements.cpp src/menus/menuscreenachievements.h
    src/menus/menuscreenaudio.cpp src/menus/menuscreenaudio.h
    src/menus/menuscreencontrols.cpp src/menus/menuscreencontrols.h
    src/menus/menuscreencredits.cpp src/menus/menuscreencredits.h
    src/menus/menuscreenfileselect.cpp src/menus/menuscreenfileselect.h
    src/menus/menuscreengame.cpp src/menus/menuscreengame.h
    src/menus/menuscreenmain.cpp src/menus/menuscreenmain.h
    src/menus/menuscreennameselect.cpp src/menus/menuscreennameselect.h
    src/menus/menuscreenoptions.cpp src/menus/menuscreenoptions.h
    src/menus/menuscreenpause.cpp src/menus/menuscreenpause.h
    src/menus/menuscreenvideo.cpp src/menus/menuscreenvideo.h
)

set(THIRD_PARTY_FILES
    thirdparty/box2d/include/box2d/b2_api.h
    thirdparty/box2d/include/box2d/b2_block_allocator.h
    thirdparty/box2d/include/box2d/b2_body.h
    thirdparty/box2d/include/box2d/b2_broad_phase.h
    thirdparty/box2d/include/box2d/b2_chain_shape.h
    thirdparty/box2d/include/box2d/b2_circle_shape.h
    thirdparty/box2d/include/box2d/b2_collision.h
    thirdparty/box2d/include/box2d/b2_common.h
    thirdparty/box2d/include/box2d/b2_contact.h
    thirdparty/box2d/include/box2d/b2_contact_manager.h
    thirdparty/box2d/include/box2d/b2_distance.h
    thirdparty/box2d/include/box2d/b2_distance_joint.h
    thirdparty/box2d/include/box2d/b2_draw.h
    thirdparty/box2d/include/box2d/b2_dynamic_tree.h
    thirdparty/box2d/include/box2d/b2_edge_shape.h
    thirdparty/box2d/include/box2d/b2_fixture.h
    thirdparty/box2d/include/box2d/b2_friction_joint.h
    thirdparty/box2d/include/box2d/b2_gear_joint.h
    thirdparty/box2d/include/box2d/b2_growable_stack.h
    thirdparty/box2d/include/box2d/b2_joint.h
    thirdparty/box2d/include/box2d/b2_math.h
    thirdparty/box2d/include/box2d/b2_motor_joint.h
    thirdparty/box2d/include/box2d/b2_mouse_joint.h
    thirdparty/box2d/include/box2d/b2_polygon_shape.h
    thirdparty/box2d/include/box2d/b2_prismatic_joint.h
    thirdparty/box2d/include/box2d/b2_pulley_joint.h
    thirdparty/box2d/include/box2d/b2_revolute_joint.h
    thirdparty/box2d/include/box2d/b2_rope.h
    thirdparty/box2d/include/box2d/b2_settings.h
    thirdparty/box2d/include/box2d/b2_shape.h
    thirdparty/box2d/include/box2d/b2_stack_allocator.h
    thirdparty/box2d/include/box2d/b2_time_of_impact.h
    thirdparty/box2d/include/box2d/b2_time_step.h
    thirdparty/box2d/include/box2d/b2_timer.h
    thirdparty/box2d/include/box2d/b2_types.h
    thirdparty/box2d/include/box2d/b2_weld_joint.h
    thirdparty/box2d/include/box2d/b2_wheel_joint.h
    thirdparty/box2d/include/box2d/b2_world.h
    thirdparty/box2d/include/box2d/b2_world_callbacks.h
    thirdparty/box2d/include/box2d/box2d.h
    thirdparty/box2d/src/collision/b2_broad_phase.cpp
    thirdparty/box2d/src/collision/b2_chain_shape.cpp
    thirdparty/box2d/src/collision/b2_circle_shape.cpp
    thirdparty/box2d/src/collision/b2_collide_circle.cpp
    thirdparty/box2d/src/collision/b2_collide_edge.cpp
    thirdparty/box2d/src/collision/b2_collide_polygon.cpp
    thirdparty/box2d/src/collision/b2_collision.cpp
    thirdparty/box2d/src/collision/b2_distance.cpp
    thirdparty/box2d/src/collision/b2_dynamic_tree.cpp
    thirdparty/box2d/src/collision/b2_edge_shape.cpp
    thirdparty/box2d/src/collision/b2_polygon_shape.cpp
    thirdparty/box2d/src/collision/b2_time_of_impact.cpp
    thirdparty/box2d/src/common/b2_block_allocator.cpp
    thirdparty/box2d/src/common/b2_draw.cpp
    thirdparty/box2d/src/common/b2_math.cpp
    thirdparty/box2d/src/common/b2_settings.cpp
    thirdparty/box2d/src/common/b2_stack_allocator.cpp
    thirdparty/box2d/src/common/b2_timer.cpp
    thirdparty/box2d/src/dynamics/b2_body.cpp
    thirdparty/box2d/src/dynamics/b2_chain_circle_contact.cpp thirdparty/box2d/src/dynamics/b2_chain_circle_contact.h
    thirdparty/box2d/src/dynamics/b2_chain_polygon_contact.cpp thirdparty/box2d/src/dynamics/b2_chain_polygon_contact.h
    thirdparty/box2d/src/dynamics/b2_circle_contact.cpp thirdparty/box2d/src/dynamics/b2_circle_contact.h
    thirdparty/box2d/src/dynamics/b2_contact.cpp
    thirdparty/box2d/src/dynamics/b2_contact_manager.cpp
    thirdparty/box2d/src/dynamics/b2_contact_solver.cpp thirdparty/box2d/src/dynamics/b2_contact_solver.h
    thirdparty/box2d/src/dynamics/b2_distance_joint.cpp
    thirdparty/box2d/src/dynamics/b2_edge_circle_contact.cpp thirdparty/box2d/src/dynamics/b2_edge_circle_contact.h
    thirdparty/box2d/src/dynamics/b2_edge_polygon_contact.cpp thirdparty/box2d/src/dynamics/b2_edge_polygon_contact.h
    thirdparty/box2d/src/dynamics/b2_fixture.cpp
    thirdparty/box2d/src/dynamics/b2_friction_joint.cpp
    thirdparty/box2d/src/dynamics/b2_gear_joint.cpp
    thirdparty/box2d/src/dynamics/b2_island.cpp thirdparty/box2d/src/dynamics/b2_island.h
    thirdparty/box2d/src/dynamics/b2_joint.cpp
    thirdparty/box2d/src/dynamics/b2_motor_joint.cpp
    thirdparty/box2d/src/dynamics/b2_mouse_joint.cpp
    thirdparty/box2d/src/dynamics/b2_polygon_circle_contact.cpp thirdparty/box2d/src/dynamics/b2_polygon_circle_contact.h
    thirdparty/box2d/src/dynamics/b2_polygon_contact.cpp thirdparty/box2d/src/dynamics/b2_polygon_contact.h
    thirdparty/box2d/src/dynamics/b2_prismatic_joint.cpp
    thirdparty/box2d/src/dynamics/b2_pulley_joint.cpp
    thirdparty/box2d/src/dynamics/b2_revolute_joint.cpp
    thirdparty/box2d/src/dynamics/b2_weld_joint.cpp
    thirdparty/box2d/src/dynamics/b2_wheel_joint.cpp
    thirdparty/box2d/src/dynamics/b2_world.cpp
    thirdparty/box2d/src/dynamics/b2_world_callbacks.cpp
    thirdparty/box2d/src/rope/b2_rope.cpp
    thirdparty/imgui/imconfig-SFML.h
    thirdparty/imgui/imconfig.h
    thirdparty/imgui/imgui-SFML.cpp thirdparty/imgui/imgui-SFML.h
    thirdparty/imgui/imgui-SFML_export.h
    thirdparty/imgui/imgui.cpp thirdparty/imgui/imgui.h
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_internal.h
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/imstb_rectpack.h
    thirdparty/imgui/imstb_textedit.h
    thirdparty/imgui/imstb_truetype.h
    thirdparty/json/json.hpp
    thirdparty/tinyxml2/tinyxml2.cpp thirdparty/tinyxml2/tinyxml2.h
)

# add_executable(deceptus ${THIRD_PARTY_FILES} ${GAME_FILES})

add_executable(deceptus ${GAME_FILES})
target_sources(deceptus PRIVATE ${THIRD_PARTY_FILES})

# disable warnings for third party
set(THIRD_PARTY_FILES_OPTIONS -w)
target_compile_options(deceptus PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${THIRD_PARTY_FILES_OPTIONS}>)

target_include_directories(deceptus PRIVATE
    .
    src
    src/game
    thirdparty
    thirdparty/box2d/include
    thirdparty/lua/include
    thirdparty/sfml/include
)

target_compile_definitions(deceptus PRIVATE
    DEVELOPMENT_MODE
    USE_GL
    _USE_MATH_DEFINES
)

if(debug)
    set_target_properties(deceptus PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    target_compile_definitions(deceptus PRIVATE
        DEBUG
    )
endif()

if(win32-msvc)
    target_compile_definitions(deceptus PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )

    target_compile_options(deceptus
        /wd5045
    )
endif()


if(UNIX OR win32-clang++)
    target_compile_options(deceptus
        -Wno-backslash-newline-escape
        -Wno-deprecated-declarations
        /std:c++20
    )
endif()

if(UNIX OR win32-g++)
    target_compile_options(deceptus
        -lc++fs
        -lfmt
    )
endif()


if(WIN32)
    set(SDL_PATH ${CMAKE_SOURCE_DIR}/thirdparty/sdl/lib/x64)
    set(LUA_PATH ${CMAKE_SOURCE_DIR}/thirdparty/lua/lib64)
    set(SFML_PATH ${CMAKE_SOURCE_DIR}/thirdparty/sfml/lib)

    link_directories(${LUA_PATH} ${SDL_PATH} ${SFML_PATH})
    
    target_link_libraries(deceptus PRIVATE
        ${SDL_PATH}/SDL2.lib
        ${LUA_PATH}/lua53.lib
        glu32
        opengl32
    )

    if(CMAKE_BUILD_TYPE STREQUAL Release)
        target_link_libraries(deceptus PRIVATE
            ${SFML_PATH}/sfml-audio.lib
            ${SFML_PATH}/sfml-graphics.lib
            ${SFML_PATH}/sfml-network.lib
            ${SFML_PATH}/sfml-system.lib
            ${SFML_PATH}/sfml-window.lib
        )
    else()
        target_link_libraries(deceptus PRIVATE
            ${SFML_PATH}/sfml-audio-d.lib
            ${SFML_PATH}/sfml-graphics-d.lib
            ${SFML_PATH}/sfml-network-d.lib
            ${SFML_PATH}/sfml-system-d.lib
            ${SFML_PATH}/sfml-window-d.lib
        )
    endif()
endif()


if(UNIX)

    find_package(PkgConfig)
    
    PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
    PKG_SEARCH_MODULE(LUA REQUIRED lua)
    PKG_SEARCH_MODULE(SFML-AUDIO REQUIRED sfml-audio)
    PKG_SEARCH_MODULE(SFML-GRAPHICS REQUIRED sfml-graphics)
    PKG_SEARCH_MODULE(SFML-NETWORK REQUIRED sfml-network)
    PKG_SEARCH_MODULE(SFML-SYSTEM REQUIRED sfml-system)
    PKG_SEARCH_MODULE(SFML-WINDOW REQUIRED sfml-window)
    
    target_link_libraries(deceptus PRIVATE
    	${SDL2_LIBRARIES}	 
    	${LUA_LIBRARIES}	 
    	${SFML-AUDIO_LIBRARIES}	 
    	${SFML-GRAPHICS_LIBRARIES} 
    	${SFML-NETWORK_LIBRARIES} 
    	${SFML-SYSTEM_LIBRARIES} 
    	${SFML-WINDOW_LIBRARIES} 
        GL
        X11
        stdc++fs
    )
    
endif()

install(TARGETS deceptus
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

