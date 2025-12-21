# ------------------------------------------------------------------------------------
# Formatting:
#   uv sync
#   uv run cmake-format -i CMakeLists.txt
# ------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

project(
    deceptus
    VERSION 1.0
    LANGUAGES C CXX)

if(DEFINED ENV{APPVEYOR_BUILD_NUMBER})
    add_definitions(-DBUILD_NUMBER=$ENV{APPVEYOR_BUILD_NUMBER})
else()
    add_definitions(-DBUILD_NUMBER=0)
endif()

include(GNUInstallDirs)

# ----------------------------------------------------------------------------------------------------
# game sources
# ----------------------------------------------------------------------------------------------------

set(GAME_FILES
    src/framework/image/image.cpp
    src/framework/image/image.h
    src/framework/image/layer.cpp
    src/framework/image/layer.h
    src/framework/image/psd.cpp
    src/framework/image/psd.h
    src/framework/image/tga.cpp
    src/framework/image/tga.h
    src/framework/joystick/gamecontroller.cpp
    src/framework/joystick/gamecontroller.h
    src/framework/joystick/gamecontrollerballvector.h
    src/framework/joystick/gamecontrollerinfo.cpp
    src/framework/joystick/gamecontrollerinfo.h
    src/framework/math/box2dtools.cpp
    src/framework/math/box2dtools.h
    src/framework/math/fbm.cpp
    src/framework/math/fbm.h
    src/framework/math/hermitecurve.cpp
    src/framework/math/hermitecurve.h
    src/framework/math/hermitecurvekey.h
    src/framework/math/maptools.cpp
    src/framework/math/maptools.h
    src/framework/math/pathinterpolation.h
    src/framework/math/sfmlmath.cpp
    src/framework/math/sfmlmath.h
    src/framework/tmxparser/tmxanimation.cpp
    src/framework/tmxparser/tmxanimation.h
    src/framework/tmxparser/tmxchunk.cpp
    src/framework/tmxparser/tmxchunk.h
    src/framework/tmxparser/tmxelement.cpp
    src/framework/tmxparser/tmxelement.h
    src/framework/tmxparser/tmxframe.cpp
    src/framework/tmxparser/tmxframe.h
    src/framework/tmxparser/tmximage.cpp
    src/framework/tmxparser/tmximage.h
    src/framework/tmxparser/tmximagelayer.cpp
    src/framework/tmxparser/tmximagelayer.h
    src/framework/tmxparser/tmxlayer.cpp
    src/framework/tmxparser/tmxlayer.h
    src/framework/tmxparser/tmxobject.cpp
    src/framework/tmxparser/tmxobject.h
    src/framework/tmxparser/tmxobjectgroup.cpp
    src/framework/tmxparser/tmxobjectgroup.h
    src/framework/tmxparser/tmxparsedata.h
    src/framework/tmxparser/tmxparser.cpp
    src/framework/tmxparser/tmxparser.h
    src/framework/tmxparser/tmxpolygon.cpp
    src/framework/tmxparser/tmxpolygon.h
    src/framework/tmxparser/tmxpolyline.cpp
    src/framework/tmxparser/tmxpolyline.h
    src/framework/tmxparser/tmxproperties.cpp
    src/framework/tmxparser/tmxproperties.h
    src/framework/tmxparser/tmxproperty.cpp
    src/framework/tmxparser/tmxproperty.h
    src/framework/tmxparser/tmxtemplate.cpp
    src/framework/tmxparser/tmxtemplate.h
    src/framework/tmxparser/tmxtile.cpp
    src/framework/tmxparser/tmxtile.h
    src/framework/tmxparser/tmxtileset.cpp
    src/framework/tmxparser/tmxtileset.h
    src/framework/tmxparser/tmxtools.cpp
    src/framework/tmxparser/tmxtools.h
    src/framework/tools/callbackmap.cpp
    src/framework/tools/checksum.cpp
    src/framework/tools/elapsedtimer.cpp
    src/framework/tools/elapsedtimer.h
    src/framework/tools/globalclock.cpp
    src/framework/tools/jsonconfiguration.cpp
    src/framework/tools/jsonconfiguration.h
    src/framework/tools/log.cpp
    src/framework/tools/log.h
    src/framework/tools/logthread.cpp
    src/framework/tools/logthread.h
    src/framework/tools/resourcepool.h
    src/framework/tools/scopeexit.cpp
    src/framework/tools/scopeexit.h
    src/framework/tools/stopwatch.cpp
    src/framework/tools/stopwatch.h
    src/framework/tools/timer.cpp
    src/game/animation/animation.cpp
    src/game/animation/animation.h
    src/game/animation/animationframedata.cpp
    src/game/animation/animationframedata.h
    src/game/animation/animationplayer.cpp
    src/game/animation/animationplayer.h
    src/game/animation/animationpool.cpp
    src/game/animation/animationpool.h
    src/game/animation/animationsettings.cpp
    src/game/animation/animationsettings.h
    src/game/animation/detonationanimation.cpp
    src/game/animation/detonationanimation.h
    src/game/animation/framemapper.cpp
    src/game/animation/framemapper.h
    src/game/audio/audio.cpp
    src/game/audio/audio.h
    src/game/audio/audiorange.cpp
    src/game/audio/audiorange.h
    src/game/audio/audioupdatedata.cpp
    src/game/audio/audioupdatedata.h
    src/game/audio/musicfilenames.cpp
    src/game/audio/musicfilenames.h
    src/game/audio/musicplayer.cpp
    src/game/audio/musicplayer.h
    src/game/audio/musicplayertypes.h
    src/game/audio/volumeupdater.cpp
    src/game/audio/volumeupdater.h
    src/game/camera/camerapanorama.cpp
    src/game/camera/camerapanorama.h
    src/game/camera/cameraroomlock.cpp
    src/game/camera/cameraroomlock.h
    src/game/camera/camerasystem.cpp
    src/game/camera/camerasystem.h
    src/game/camera/camerasystemconfiguration.cpp
    src/game/camera/camerasystemconfiguration.h
    src/game/camera/camerasystemconfigurationui.cpp
    src/game/camera/camerasystemconfigurationui.h
    src/game/camera/camerazoom.cpp
    src/game/camera/camerazoom.h
    src/game/clock/gameclock.cpp
    src/game/clock/gameclock.h
    src/game/config/gameconfiguration.cpp
    src/game/config/gameconfiguration.h
    src/game/config/tweaks.cpp
    src/game/config/tweaks.h
    src/game/constants.h
    src/game/controller/gamecontrollerdata.cpp
    src/game/controller/gamecontrollerdata.h
    src/game/controller/gamecontrollerdetection.cpp
    src/game/controller/gamecontrollerdetection.h
    src/game/controller/gamecontrollerintegration.cpp
    src/game/controller/gamecontrollerintegration.h
    src/game/debug/console.cpp
    src/game/debug/console.h
    src/game/debug/debugdraw.cpp
    src/game/debug/debugdraw.h
    src/game/debug/drawstates.cpp
    src/game/debug/drawstates.h
    src/game/debug/logui.cpp
    src/game/debug/logui.h
    src/game/effects/boomeffect.cpp
    src/game/effects/boomeffect.h
    src/game/effects/boomeffectenvelope.cpp
    src/game/effects/boomeffectenvelope.h
    src/game/effects/boomeffectenveloperandom.cpp
    src/game/effects/boomeffectenveloperandom.h
    src/game/effects/boomeffectenvelopesine.cpp
    src/game/effects/boomeffectenvelopesine.h
    src/game/effects/boomsettings.cpp
    src/game/effects/boomsettings.h
    src/game/effects/fadetransitioneffect.cpp
    src/game/effects/fadetransitioneffect.h
    src/game/effects/lightsystem.cpp
    src/game/effects/lightsystem.h
    src/game/effects/screentransition.cpp
    src/game/effects/screentransition.h
    src/game/effects/screentransitioneffect.cpp
    src/game/effects/screentransitioneffect.h
    src/game/effects/spawneffect.cpp
    src/game/effects/spawneffect.h
    src/game/effects/waterbubbles.cpp
    src/game/effects/waterbubbles.h
    src/game/event/eventdistributor.cpp
    src/game/event/eventdistributor.h
    src/game/game.cpp
    src/game/game.h
    src/game/splashscreen.cpp
    src/game/splashscreen.h
    src/game/image/layerdata.cpp
    src/game/image/layerdata.h
    src/game/ingamemenu/ingamemenu.cpp
    src/game/ingamemenu/ingamemenu.h
    src/game/ingamemenu/ingamemenuarchives.cpp
    src/game/ingamemenu/ingamemenuarchives.h
    src/game/ingamemenu/ingamemenuaudio.cpp
    src/game/ingamemenu/ingamemenuaudio.h
    src/game/ingamemenu/ingamemenuinventory.cpp
    src/game/ingamemenu/ingamemenuinventory.h
    src/game/ingamemenu/ingamemenumap.cpp
    src/game/ingamemenu/ingamemenumap.h
    src/game/ingamemenu/ingamemenupage.cpp
    src/game/ingamemenu/ingamemenupage.h
    src/game/ingamemenu/menuconfig.cpp
    src/game/ingamemenu/menuconfig.h
    src/game/io/eventserializer.cpp
    src/game/io/eventserializer.h
    src/game/io/gamedeserializedata.cpp
    src/game/io/gamedeserializedata.h
    src/game/io/lazytexture.cpp
    src/game/io/lazytexture.h
    src/game/io/meshtools.cpp
    src/game/io/meshtools.h
    src/game/io/preloader.cpp
    src/game/io/preloader.h
    src/game/io/texturepool.h
    src/game/layers/ambientocclusion.cpp
    src/game/layers/ambientocclusion.h
    src/game/layers/bitmapfont.cpp
    src/game/layers/bitmapfont.h
    src/game/layers/controlleroverlay.cpp
    src/game/layers/controlleroverlay.h
    src/game/layers/infolayer.cpp
    src/game/layers/infolayer.h
    src/game/layers/parallaxlayer.cpp
    src/game/layers/parallaxlayer.h
    src/game/layers/parallaxsettings.cpp
    src/game/layers/parallaxsettings.h
    src/game/layers/rainoverlay.cpp
    src/game/layers/rainoverlay.h
    src/game/layers/thunderstormoverlay.cpp
    src/game/layers/thunderstormoverlay.h
    src/game/layers/weatheroverlay.cpp
    src/game/layers/weatheroverlay.h
    src/game/level/atmosphere.cpp
    src/game/level/atmosphere.h
    src/game/level/blendmodedeserializer.cpp
    src/game/level/blendmodedeserializer.h
    src/game/level/chunk.cpp
    src/game/level/chunk.h
    src/game/level/enemydescription.cpp
    src/game/level/enemydescription.h
    src/game/level/fixturenode.cpp
    src/game/level/fixturenode.h
    src/game/level/gamemechanismregistry.cpp
    src/game/level/gamemechanismregistry.h
    src/game/level/gamenode.cpp
    src/game/level/gamenode.h
    src/game/level/hitbox.cpp
    src/game/level/hitbox.h
    src/game/level/level.cpp
    src/game/level/level.h
    src/game/level/leveldescription.cpp
    src/game/level/leveldescription.h
    src/game/level/levelfiles.cpp
    src/game/level/levelfiles.h
    src/game/level/levels.cpp
    src/game/level/levels.h
    src/game/level/levelscript.cpp
    src/game/level/levelscript.h
    src/game/level/luaconstants.h
    src/game/level/luainterface.cpp
    src/game/level/luainterface.h
    src/game/level/luanode.cpp
    src/game/level/luanode.h
    src/game/level/parsedata.cpp
    src/game/level/parsedata.h
    src/game/level/room.cpp
    src/game/level/room.h
    src/game/level/roomupdater.cpp
    src/game/level/roomupdater.h
    src/game/level/scriptproperty.cpp
    src/game/level/scriptproperty.h
    src/game/level/stenciltilemap.cpp
    src/game/level/stenciltilemap.h
    src/game/level/tilemap.cpp
    src/game/level/tilemap.h
    src/game/level/tilemapfactory.cpp
    src/game/level/tilemapfactory.h
    src/game/level/tmxenemy.cpp
    src/game/level/tmxenemy.h
    src/game/mechanisms/blockingrect.cpp
    src/game/mechanisms/blockingrect.h
    src/game/mechanisms/bouncer.cpp
    src/game/mechanisms/bouncer.h
    src/game/mechanisms/bouncerwrapper.cpp
    src/game/mechanisms/bouncerwrapper.h
    src/game/mechanisms/boxcollider.cpp
    src/game/mechanisms/boxcollider.h
    src/game/mechanisms/bubblecube.cpp
    src/game/mechanisms/bubblecube.h
    src/game/mechanisms/buttonrect.cpp
    src/game/mechanisms/buttonrect.h
    src/game/mechanisms/checkpoint.cpp
    src/game/mechanisms/checkpoint.h
    src/game/mechanisms/collapsingplatform.cpp
    src/game/mechanisms/collapsingplatform.h
    src/game/mechanisms/controllerhelp.cpp
    src/game/mechanisms/controllerhelp.h
    src/game/mechanisms/controllerkeymap.cpp
    src/game/mechanisms/controllerkeymap.h
    src/game/mechanisms/conveyorbelt.cpp
    src/game/mechanisms/conveyorbelt.h
    src/game/mechanisms/crusher.cpp
    src/game/mechanisms/crusher.h
    src/game/mechanisms/damagerect.cpp
    src/game/mechanisms/damagerect.h
    src/game/mechanisms/deathblock.cpp
    src/game/mechanisms/deathblock.h
    src/game/mechanisms/destructibleblockingrect.cpp
    src/game/mechanisms/destructibleblockingrect.h
    src/game/mechanisms/dialogue.cpp
    src/game/mechanisms/dialogue.h
    src/game/mechanisms/door.cpp
    src/game/mechanisms/door.h
    src/game/mechanisms/dust.cpp
    src/game/mechanisms/dust.h
    src/game/mechanisms/enemywall.cpp
    src/game/mechanisms/enemywall.h
    src/game/mechanisms/extra.cpp
    src/game/mechanisms/extra.h
    src/game/mechanisms/extrawrapper.cpp
    src/game/mechanisms/extrawrapper.h
    src/game/mechanisms/fan.cpp
    src/game/mechanisms/fan.h
    src/game/mechanisms/fireflies.cpp
    src/game/mechanisms/fireflies.h
    src/game/mechanisms/flowfieldtexturechangeevent.cpp
    src/game/mechanisms/flowfieldtexturechangeevent.h
    src/game/mechanisms/gamemechanism.cpp
    src/game/mechanisms/gamemechanism.h
    src/game/mechanisms/gamemechanismdeserializer.cpp
    src/game/mechanisms/gamemechanismdeserializer.h
    src/game/mechanisms/gamemechanismdeserializerconstants.h
    src/game/mechanisms/gamemechanismdeserializerregistry.cpp
    src/game/mechanisms/gamemechanismdeserializerregistry.h
    src/game/mechanisms/gamemechanismobserver.cpp
    src/game/mechanisms/gamemechanismobserver.h
    src/game/mechanisms/gateway.cpp
    src/game/mechanisms/gateway.h
    src/game/mechanisms/imagelayer.cpp
    src/game/mechanisms/imagelayer.h
    src/game/mechanisms/infooverlay.cpp
    src/game/mechanisms/infooverlay.h
    src/game/mechanisms/interactionhelp.cpp
    src/game/mechanisms/interactionhelp.h
    src/game/mechanisms/laser.cpp
    src/game/mechanisms/laser.h
    src/game/mechanisms/lever.cpp
    src/game/mechanisms/lever.h
    src/game/mechanisms/levermechanismmerger.cpp
    src/game/mechanisms/levermechanismmerger.h
    src/game/mechanisms/moveablebox.cpp
    src/game/mechanisms/moveablebox.h
    src/game/mechanisms/movingplatform.cpp
    src/game/mechanisms/movingplatform.h
    src/game/mechanisms/onoffblock.cpp
    src/game/mechanisms/onoffblock.h
    src/game/mechanisms/portal.cpp
    src/game/mechanisms/portal.h
    src/game/mechanisms/ringshaderlayer.cpp
    src/game/mechanisms/ringshaderlayer.h
    src/game/mechanisms/rope.cpp
    src/game/mechanisms/rope.h
    src/game/mechanisms/ropewithlight.cpp
    src/game/mechanisms/ropewithlight.h
    src/game/mechanisms/rotatingblade.cpp
    src/game/mechanisms/rotatingblade.h
    src/game/mechanisms/sensorrect.cpp
    src/game/mechanisms/sensorrect.h
    src/game/mechanisms/shaderlayer.cpp
    src/game/mechanisms/shaderlayer.h
    src/game/mechanisms/smokeeffect.cpp
    src/game/mechanisms/soundemitter.cpp
    src/game/mechanisms/soundemitter.h
    src/game/mechanisms/spikeball.cpp
    src/game/mechanisms/spikeball.h
    src/game/mechanisms/spikeblock.cpp
    src/game/mechanisms/spikeblock.h
    src/game/mechanisms/spikes.cpp
    src/game/mechanisms/spikes.h
    src/game/mechanisms/staticlight.cpp
    src/game/mechanisms/staticlight.h
    src/game/mechanisms/textlayer.cpp
    src/game/mechanisms/textlayer.h
    src/game/mechanisms/treasurechest.cpp
    src/game/mechanisms/treasurechest.h
    src/game/mechanisms/waterdamage.cpp
    src/game/mechanisms/waterdamage.h
    src/game/mechanisms/watersurface.cpp
    src/game/mechanisms/watersurface.h
    src/game/mechanisms/weather.cpp
    src/game/mechanisms/weather.h
    src/game/mechanisms/wind.cpp
    src/game/mechanisms/wind.h
    src/game/mechanisms/zoomrect.cpp
    src/game/mechanisms/zoomrect.h
    src/game/physics/chainshapeanalyzer.cpp
    src/game/physics/chainshapeanalyzer.h
    src/game/physics/gamecontactlistener.cpp
    src/game/physics/gamecontactlistener.h
    src/game/physics/onewaywall.cpp
    src/game/physics/onewaywall.h
    src/game/physics/physics.cpp
    src/game/physics/physics.h
    src/game/physics/physicsconfiguration.cpp
    src/game/physics/physicsconfiguration.h
    src/game/physics/physicsconfigurationui.cpp
    src/game/physics/physicsconfigurationui.h
    src/game/physics/squaremarcher.cpp
    src/game/physics/squaremarcher.h
    src/game/physics/worldquery.cpp
    src/game/physics/worldquery.h
    src/game/player/extratable.cpp
    src/game/player/extratable.h
    src/game/player/health.cpp
    src/game/player/health.h
    src/game/player/inventory.cpp
    src/game/player/inventory.h
    src/game/player/inventorybasedcontrols.cpp
    src/game/player/inventorybasedcontrols.h
    src/game/player/inventoryitemdescriptionreader.cpp
    src/game/player/inventoryitemdescriptionreader.h
    src/game/player/player.cpp
    src/game/player/player.h
    src/game/player/playeranimation.cpp
    src/game/player/playeranimation.h
    src/game/player/playerattack.cpp
    src/game/player/playerattack.h
    src/game/player/playerattackdash.cpp
    src/game/player/playerattackdash.h
    src/game/player/playeraudio.cpp
    src/game/player/playeraudio.h
    src/game/player/playerbelt.cpp
    src/game/player/playerbelt.h
    src/game/player/playerbend.h
    src/game/player/playerclimb.cpp
    src/game/player/playerclimb.h
    src/game/player/playercontrols.cpp
    src/game/player/playercontrols.h
    src/game/player/playercontrolstate.cpp
    src/game/player/playercontrolstate.h
    src/game/player/playerdash.cpp
    src/game/player/playerdash.h
    src/game/player/playerinfo.cpp
    src/game/player/playerinfo.h
    src/game/player/playerinput.cpp
    src/game/player/playerinput.h
    src/game/player/playerjump.cpp
    src/game/player/playerjump.h
    src/game/player/playerjumptrace.h
    src/game/player/playerplatform.cpp
    src/game/player/playerplatform.h
    src/game/player/playerspeed.h
    src/game/player/playerstats.cpp
    src/game/player/playerstats.h
    src/game/player/playerstencil.cpp
    src/game/player/playerstencil.h
    src/game/player/playerutils.cpp
    src/game/player/playerutils.h
    src/game/player/skill.cpp
    src/game/player/skill.h
    src/game/player/timerlock.cpp
    src/game/player/timerlock.h
    src/game/player/weaponsystem.cpp
    src/game/player/weaponsystem.h
    src/game/scenes/forestscene.cpp
    src/game/scenes/forestscene.h
    src/game/sfx/gameaudio.cpp
    src/game/sfx/gameaudio.h
    src/game/shaders/atmosphereshader.cpp
    src/game/shaders/atmosphereshader.h
    src/game/shaders/blurshader.cpp
    src/game/shaders/blurshader.h
    src/game/shaders/deathshader.cpp
    src/game/shaders/deathshader.h
    src/game/shaders/gammashader.cpp
    src/game/shaders/gammashader.h
    src/game/state/displaymode.cpp
    src/game/state/displaymode.h
    src/game/state/gamestate.cpp
    src/game/state/gamestate.h
    src/game/state/savestate.cpp
    src/game/state/savestate.h
    src/game/tests/test.cpp
    src/game/tests/test.h
    src/game/ui/messagebox.cpp
    src/game/ui/messagebox.h
    src/game/ui/richtextparser.cpp
    src/game/ui/richtextparser.h
    src/game/weapons/arrow.cpp
    src/game/weapons/arrow.h
    src/game/weapons/bow.cpp
    src/game/weapons/bow.h
    src/game/weapons/gun.cpp
    src/game/weapons/gun.h
    src/game/weapons/playersword.cpp
    src/game/weapons/playersword.h
    src/game/weapons/projectile.cpp
    src/game/weapons/projectile.h
    src/game/weapons/projectilehitanimation.cpp
    src/game/weapons/projectilehitanimation.h
    src/game/weapons/projectilehitaudio.cpp
    src/game/weapons/projectilehitaudio.h
    src/game/weapons/weapon.cpp
    src/game/weapons/weapon.h
    src/game/weapons/weaponfactory.cpp
    src/game/weapons/weaponfactory.h
    src/game/weapons/weaponproperties.cpp
    src/game/weapons/weaponproperties.h
    src/menus/menu.cpp
    src/menus/menu.h
    src/menus/menuaudio.cpp
    src/menus/menuaudio.h
    src/menus/menuscreen.cpp
    src/menus/menuscreen.h
    src/menus/menuscreenachievements.cpp
    src/menus/menuscreenachievements.h
    src/menus/menuscreenaudio.cpp
    src/menus/menuscreenaudio.h
    src/menus/menuscreencontrols.cpp
    src/menus/menuscreencontrols.h
    src/menus/menuscreencredits.cpp
    src/menus/menuscreencredits.h
    src/menus/menuscreenfileselect.cpp
    src/menus/menuscreenfileselect.h
    src/menus/menuscreengame.cpp
    src/menus/menuscreengame.h
    src/menus/menuscreenmain.cpp
    src/menus/menuscreenmain.h
    src/menus/menuscreennameselect.cpp
    src/menus/menuscreennameselect.h
    src/menus/menuscreenoptions.cpp
    src/menus/menuscreenoptions.h
    src/menus/menuscreenpause.cpp
    src/menus/menuscreenpause.h
    src/menus/menuscreenvideo.cpp
    src/menus/menuscreenvideo.h
    src/main.cpp
    deceptus_resource.rc
    src/game/menu3d/menu3drenderer.cpp
    src/game/menu3d/menu3drenderer.h
    src/interfaces/drawable.h
    src/opengl/gl_current.h
    src/opengl/glslprogram.cpp
    src/opengl/glslprogram.h
    src/opengl/glutils.cpp
    src/opengl/glutils.h
    src/opengl/vbos/vbomesh.cpp
    src/opengl/vbos/vbomesh.h
    src/opengl/vbos/vbosphere.cpp
    src/opengl/vbos/vbosphere.h
    src/game/shaders/shaderpool.cpp
    src/game/shaders/shaderpool.h
    src/GL/glew.h
    src/interfaces/drawable.cpp
    src/interfaces/drawable.h
    src/interfaces/transformable.cpp
    src/interfaces/transformable.h
    src/opengl/image/bmpreader.cpp
    src/opengl/image/bmpreader.h
    src/opengl/image/png.cpp
    src/opengl/image/png.h
    src/opengl/image/tgaio.cpp
    src/opengl/image/tgaio.h
    src/game/menu3d/menu3dobject.h
    src/game/menu3d/texturedobject.cpp
    src/game/menu3d/texturedobject.h
    src/game/menu3d/sphereobject.cpp
    src/game/menu3d/sphereobject.h
    src/game/menu3d/texturedsphereobject.cpp
    src/game/menu3d/texturedsphereobject.h)

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
    thirdparty/box2d/src/dynamics/b2_chain_circle_contact.cpp
    thirdparty/box2d/src/dynamics/b2_chain_circle_contact.h
    thirdparty/box2d/src/dynamics/b2_chain_polygon_contact.cpp
    thirdparty/box2d/src/dynamics/b2_chain_polygon_contact.h
    thirdparty/box2d/src/dynamics/b2_circle_contact.cpp
    thirdparty/box2d/src/dynamics/b2_circle_contact.h
    thirdparty/box2d/src/dynamics/b2_contact.cpp
    thirdparty/box2d/src/dynamics/b2_contact_manager.cpp
    thirdparty/box2d/src/dynamics/b2_contact_solver.cpp
    thirdparty/box2d/src/dynamics/b2_contact_solver.h
    thirdparty/box2d/src/dynamics/b2_distance_joint.cpp
    thirdparty/box2d/src/dynamics/b2_edge_circle_contact.cpp
    thirdparty/box2d/src/dynamics/b2_edge_circle_contact.h
    thirdparty/box2d/src/dynamics/b2_edge_polygon_contact.cpp
    thirdparty/box2d/src/dynamics/b2_edge_polygon_contact.h
    thirdparty/box2d/src/dynamics/b2_fixture.cpp
    thirdparty/box2d/src/dynamics/b2_friction_joint.cpp
    thirdparty/box2d/src/dynamics/b2_gear_joint.cpp
    thirdparty/box2d/src/dynamics/b2_island.cpp
    thirdparty/box2d/src/dynamics/b2_island.h
    thirdparty/box2d/src/dynamics/b2_joint.cpp
    thirdparty/box2d/src/dynamics/b2_motor_joint.cpp
    thirdparty/box2d/src/dynamics/b2_mouse_joint.cpp
    thirdparty/box2d/src/dynamics/b2_polygon_circle_contact.cpp
    thirdparty/box2d/src/dynamics/b2_polygon_circle_contact.h
    thirdparty/box2d/src/dynamics/b2_polygon_contact.cpp
    thirdparty/box2d/src/dynamics/b2_polygon_contact.h
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
    thirdparty/imgui/imgui-SFML.cpp
    thirdparty/imgui/imgui-SFML.h
    thirdparty/imgui/imgui-SFML_export.h
    thirdparty/imgui/imgui.cpp
    thirdparty/imgui/imgui.h
    thirdparty/imgui/imgui_draw.cpp
    thirdparty/imgui/imgui_internal.h
    thirdparty/imgui/imgui_tables.cpp
    thirdparty/imgui/imgui_widgets.cpp
    thirdparty/imgui/imstb_rectpack.h
    thirdparty/imgui/imstb_textedit.h
    thirdparty/imgui/imstb_truetype.h
    thirdparty/json/json.hpp
    thirdparty/tinyxml2/tinyxml2.cpp
    thirdparty/tinyxml2/tinyxml2.h)

add_executable(
    deceptus
    ${GAME_FILES}
    data/scripts/enemies/archer.lua
    data/scripts/enemies/arrowtrap.lua
    data/scripts/enemies/arrowtrap.lua
    data/scripts/enemies/arrowtrap_2.lua
    data/scripts/enemies/arrowtrap_2.lua
    data/scripts/enemies/bat_2.lua
    data/scripts/enemies/bat_oneway.lua
    data/scripts/enemies/blob_2.lua
    data/scripts/enemies/bonefish.lua
    data/scripts/enemies/cannon_2.lua
    data/scripts/enemies/constants.lua
    data/scripts/enemies/critter.lua
    data/scripts/enemies/deprecated_bat.lua
    data/scripts/enemies/deprecated_blob.lua
    data/scripts/enemies/deprecated_cannon.lua
    data/scripts/enemies/deprecated_dumb.lua
    data/scripts/enemies/deprecated_endboss_1.lua
    data/scripts/enemies/deprecated_klonk.lua
    data/scripts/enemies/deprecated_skeleton.lua
    data/scripts/enemies/ghost.lua
    data/scripts/enemies/helpers.lua
    data/scripts/enemies/interpolation.lua
    data/scripts/enemies/klonk_2.lua
    data/scripts/enemies/landmine.lua
    data/scripts/enemies/minik_bomber.lua
    data/scripts/enemies/nukumaru.lua
    data/scripts/enemies/rat.lua
    data/scripts/enemies/shadow_monk.lua
    data/scripts/enemies/spiky.lua
    data/scripts/enemies/vector2.lua
    data/scripts/enemies/vectorial2.lua
    data/scripts/enemies/watermine.lua
    data/shaders/blur.frag
    data/shaders/brightness.frag
    data/shaders/death.frag
    data/shaders/death.vert
    data/shaders/flash.frag
    data/shaders/light.frag
    data/shaders/pixelate.frag
    data/shaders/raycast.frag
    data/shaders/raycast.vert
    data/shaders/water.frag
    data/shaders/waterfall.frag
    data/shaders/waterfall.vert
    data/shaders/wave.vert
    doc/level_design/advanced_topics.md
    doc/level_design/designing_a_level.md
    doc/level_design/enemies.md
    doc/level_design/mechanisms.md
    doc/level_design/visual_effects.md
    doc/lua_interface/readme.md
    data/scripts/enemies/state_machine.lua
    data/scripts/enemies/archer.lua
    data/scripts/enemies/arrowtrap.lua
    data/scripts/enemies/arrowtrap_2.lua
    data/scripts/enemies/bat_2.lua
    data/scripts/enemies/bat_oneway.lua
    data/scripts/enemies/blob_2.lua
    data/scripts/enemies/bonefish.lua
    data/scripts/enemies/cannon_2.lua
    data/scripts/enemies/constants.lua
    data/scripts/enemies/critter.lua
    data/scripts/enemies/deprecated_bat.lua
    data/scripts/enemies/deprecated_blob.lua
    data/scripts/enemies/deprecated_cannon.lua
    data/scripts/enemies/deprecated_dumb.lua
    data/scripts/enemies/deprecated_endboss_1.lua
    data/scripts/enemies/deprecated_klonk.lua
    data/scripts/enemies/deprecated_skeleton.lua
    data/scripts/enemies/ghost.lua
    data/scripts/enemies/helpers.lua
    data/scripts/enemies/interpolation.lua
    data/scripts/enemies/klonk_2.lua
    data/scripts/enemies/landmine.lua
    data/scripts/enemies/minik_bomber.lua
    data/scripts/enemies/nukumaru.lua
    data/scripts/enemies/rat.lua
    data/scripts/enemies/shadow_monk.lua
    data/scripts/enemies/spiky.lua
    data/scripts/enemies/state_machine.lua
    data/scripts/enemies/vector2.lua
    data/scripts/enemies/vectorial2.lua
    data/scripts/enemies/watermine.lua)

target_sources(deceptus PRIVATE ${THIRD_PARTY_FILES})

# set up third party libraries
include(FetchContent)

# SFML configuration
set(SFML_BUILD_AUDIO
    ON
    CACHE BOOL "" FORCE)
set(SFML_BUILD_GRAPHICS
    ON
    CACHE BOOL "" FORCE)
set(SFML_BUILD_WINDOW
    ON
    CACHE BOOL "" FORCE)
set(SFML_BUILD_SYSTEM
    ON
    CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK
    ON
    CACHE BOOL "" FORCE)
set(SFML_USE_STATIC_STD_LIBS
    OFF
    CACHE BOOL "" FORCE)
set(SFML_BUILD_SHARED_LIBS
    ON
    CACHE BOOL "" FORCE)

# fetch dependencies
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.x)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.4)
FetchContent_Declare(
    LUA
    GIT_REPOSITORY https://github.com/lua/lua.git
    GIT_TAG v5.4.7)

FetchContent_MakeAvailable(SFML SDL3 LUA)

# ----------------------------------------------------------------------------------------------------
# lua configuration (lua is not a cmake project)
# ----------------------------------------------------------------------------------------------------

add_subdirectory(thirdparty/lua)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/lua)
target_link_libraries(deceptus PRIVATE lua)

# ----------------------------------------------------------------------------------------------------
# compiler & linker options
# ----------------------------------------------------------------------------------------------------

set(THIRD_PARTY_FILES_OPTIONS -w)
target_compile_options(deceptus PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${THIRD_PARTY_FILES_OPTIONS}>)

target_include_directories(
    deceptus
    PRIVATE .
            src
            src/game
            thirdparty
            thirdparty/box2d/include
            thirdparty/lua
            ${SFML_INCLUDE_DIR}
            ${SDL3_INCLUDE_DIRS})

target_compile_definitions(deceptus PRIVATE DEVELOPMENT_MODE USE_GL _USE_MATH_DEFINES)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(deceptus PRIVATE DEBUG)
endif()

# ----------------------------------------------------------------------------------------------------
# platform-specific configurations
# ----------------------------------------------------------------------------------------------------

target_link_libraries(deceptus PRIVATE sfml-system sfml-audio sfml-graphics sfml-window SDL3::SDL3
                                       lua opengl32 glu32)

if(WIN32)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(deceptus PROPERTIES WIN32_EXECUTABLE FALSE)
    else()
        set_target_properties(deceptus PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()

    if(win32-msvc)
        target_compile_definitions(deceptus PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(deceptus PRIVATE /wd5045)
    endif()

    if(MSVC)
        add_compile_options(/MP)
    endif()

    # Link GLEW library for Windows
    target_link_libraries(deceptus PRIVATE
        ${CMAKE_SOURCE_DIR}/src/GL/lib/Release/x64/glew32s.lib
    )

endif()

if(UNIX AND NOT APPLE)

    target_link_libraries(deceptus PRIVATE GL X11 stdc++fs)

endif()

if(APPLE)

    find_library(OPENGL_LIBRARY OpenGL)
    target_link_libraries(deceptus PRIVATE ${OPENGL_LIBRARY} "-framework Cocoa")

    # force use of brew clang (intel)
    set(HOMEBREW_LLVM "/usr/local/opt/llvm")
    set(CMAKE_CXX_COMPILER "${HOMEBREW_LLVM}/bin/clang++")
    set(CMAKE_C_COMPILER "${HOMEBREW_LLVM}/bin/clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I${HOMEBREW_LLVM}/include/c++/v1")
    set(CMAKE_EXE_LINKER_FLAGS
        "${CMAKE_EXE_LINKER_FLAGS} -L${HOMEBREW_LLVM}/lib -Wl,-rpath,${HOMEBREW_LLVM}/lib")
    set(SDKROOT $ENV{SDKROOT})
endif()

# ----------------------------------------------------------------------------------------------------
# installation
# ----------------------------------------------------------------------------------------------------

install(
    TARGETS deceptus
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(SCRIPT "${CMAKE_SOURCE_DIR}/thirdparty/sdl2/copy_sdl2_dll.cmake")
