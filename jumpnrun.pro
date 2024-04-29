TEMPLATE = app
TARGET = deceptus
CONFIG_APP_NAME = deceptus

DEFINES += _USE_MATH_DEFINES
DEFINES += USE_GL
DEFINES += DEVELOPMENT_MODE

CONFIG += c++latest
CONFIG -= debug_and_release

OBJECTS_DIR = .obj

# get rid of all qt
CONFIG -= QT
CONFIG -= app_bundle
QT -= gui
QT -= core
LIBS -= -lQtGui
LIBS -= -lQtCore

RC_ICONS = data/game/deceptus.ico

DEFINES_RELEASE += RELEASE_BUILD

DEFINES += SFML_DEFINE_DISCRETE_GPU_PREFERENCE

# mac shit
CONFIG += c++20
QMAKE_MACOSX_DEPLOYMENT_TARGET = 14.4.1


debug {
   CONFIG += console
   DEFINES += DEBUG
}

win32-msvc {
   # link debug symbols
   message("configured for msvc")
   DEFINES += _CRT_SECURE_NO_WARNINGS
   QMAKE_CXXFLAGS_RELEASE += /Zi
   QMAKE_LFLAGS_RELEASE += /DEBUG /OPT:REF /OPT:ICF
   QMAKE_CFLAGS_WARN_ON = -W4
   QMAKE_CXXFLAGS_WARN_ON = -W4
   QMAKE_CXXFLAGS += /wd5045§
}

macx|linux|win32-clang++ {
   message("configured for clang")
   CLANG_VERSION = $$system("clang --version")
   message($$CLANG_VERSION)

   #QMAKE_CXXFLAGS += -std:c++20
   QMAKE_CXXFLAGS += -std=c++20
   QMAKE_CXXFLAGS += -Wno-backslash-newline-escape
   QMAKE_CXXFLAGS += -Wno-deprecated-declarations
   QMAKE_CXXFLAGS += -mmacosx-version-min=13.3
}

#macx|linux|win32-g++ {
#   message("configured for g++")
#   QMAKE_CXXFLAGS += -std=c++20
#   QMAKE_CXXFLAGS += -lc++fs
#   QMAKE_CXXFLAGS += -lfmt
#}

win32 {
   LIBS += -Lthirdparty\lua\lib64
   LIBS += -Lthirdparty\sdl\lib\x64
   LIBS += -lSDL2
   LIBS += -lglu32
   LIBS += -lopengl32
   LIBS += -llua53

   # sfml
   LIBS += -Lthirdparty\sfml\lib
   CONFIG(release, debug|release) {
      LIBS += -lsfml-audio
      LIBS += -lsfml-graphics
      LIBS += -lsfml-network
      LIBS += -lsfml-window
      LIBS += -lsfml-system
   } else {
      LIBS += -lsfml-audio-d
      LIBS += -lsfml-graphics-d
      LIBS += -lsfml-network-d
      LIBS += -lsfml-window-d
      LIBS += -lsfml-system-d
   }
}

linux {
   LIBS += -lstdc++fs
   LIBS += $$system(pkg-config lua --libs)
   LIBS += $$system(pkg-config sdl2 --libs)
   LIBS += -lGL
   LIBS += $$system(pkg-config sfml-all --libs)
   LIBS += -lX11
}

macx {
    LIBS += -L/usr/local/opt/sdl2/lib
    LIBS += -lsdl2

    LIBS += -L/usr/local/opt/lua/lib
    LIBS += -llua


    LIBS += -L/usr/local/opt/sfml/lib
    LIBS += -lsfml-audio
    LIBS += -lsfml-graphics
    LIBS += -lsfml-network
    LIBS += -lsfml-window
    LIBS += -lsfml-system
}

#sfml
INCLUDEPATH += thirdparty/sfml/include
DEPENDPATH += thirdparty/sfml/include

INCLUDEPATH += .
INCLUDEPATH += src
INCLUDEPATH += src/game
INCLUDEPATH += thirdparty
INCLUDEPATH += thirdparty/box2d/include
INCLUDEPATH += thirdparty/lua/include

SOURCES += \
   src/framework/image/image.cpp \
   src/framework/image/layer.cpp \
   src/framework/image/psd.cpp \
   src/framework/image/tga.cpp \
   src/framework/joystick/gamecontroller.cpp \
   src/framework/joystick/gamecontrollerinfo.cpp \
   src/framework/math/box2dtools.cpp \
   src/framework/math/fbm.cpp \
   src/framework/math/hermitecurve.cpp \
   src/framework/math/maptools.cpp \
   src/framework/math/sfmlmath.cpp \
   src/framework/tmxparser/tmxanimation.cpp \
   src/framework/tmxparser/tmxchunk.cpp \
   src/framework/tmxparser/tmxelement.cpp \
   src/framework/tmxparser/tmxframe.cpp \
   src/framework/tmxparser/tmximage.cpp \
   src/framework/tmxparser/tmximagelayer.cpp \
   src/framework/tmxparser/tmxlayer.cpp \
   src/framework/tmxparser/tmxobject.cpp \
   src/framework/tmxparser/tmxobjectgroup.cpp \
   src/framework/tmxparser/tmxparser.cpp \
   src/framework/tmxparser/tmxpolygon.cpp \
   src/framework/tmxparser/tmxpolyline.cpp \
   src/framework/tmxparser/tmxproperties.cpp \
   src/framework/tmxparser/tmxproperty.cpp \
   src/framework/tmxparser/tmxtemplate.cpp \
   src/framework/tmxparser/tmxtile.cpp \
   src/framework/tmxparser/tmxtileset.cpp \
   src/framework/tmxparser/tmxtools.cpp \
   src/framework/tools/callbackmap.cpp \
   src/framework/tools/checksum.cpp \
   src/framework/tools/elapsedtimer.cpp \
   src/framework/tools/globalclock.cpp \
   src/framework/tools/jsonconfiguration.cpp \
   src/framework/tools/log.cpp \
   src/framework/tools/logthread.cpp \
   src/framework/tools/scopeexit.cpp \
   src/framework/tools/stopwatch.cpp \
   src/framework/tools/timer.cpp \
   src/game/ambientocclusion.cpp \
   src/game/animation.cpp \
   src/game/animationframedata.cpp \
   src/game/animationplayer.cpp \
   src/game/animationpool.cpp \
   src/game/animationsettings.cpp \
   src/game/arrow.cpp \
   src/game/atmosphere.cpp \
   src/game/audio.cpp \
   src/game/audiorange.cpp \
   src/game/audioupdatedata.cpp \
   src/game/bitmapfont.cpp \
   src/game/blendmodedeserializer.cpp \
   src/game/boomeffect.cpp \
   src/game/boomeffectenvelope.cpp \
   src/game/boomeffectenveloperandom.cpp \
   src/game/boomeffectenvelopesine.cpp \
   src/game/boomsettings.cpp \
   src/game/bow.cpp \
   src/game/camerapanorama.cpp \
   src/game/cameraroomlock.cpp \
   src/game/camerasystem.cpp \
   src/game/camerasystemconfiguration.cpp \
   src/game/camerasystemconfigurationui.cpp \
   src/game/chainshapeanalyzer.cpp \
   src/game/chunk.cpp \
   src/game/console.cpp \
   src/game/cutscene.cpp \
   src/game/debugdraw.cpp \
   src/game/detonationanimation.cpp \
   src/game/displaymode.cpp \
   src/game/drawstates.cpp \
   src/game/effects/lightsystem.cpp \
   src/game/effects/spawneffect.cpp \
   src/game/enemydescription.cpp \
   src/game/eventdistributor.cpp \
   src/game/eventserializer.cpp \
   src/game/extratable.cpp \
   src/game/fadetransitioneffect.cpp \
   src/game/fixturenode.cpp \
   src/game/forestscene.cpp \
   src/game/framemapper.cpp \
   src/game/game.cpp \
   src/game/gameaudio.cpp \
   src/game/gameclock.cpp \
   src/game/gameconfiguration.cpp \
   src/game/gamecontactlistener.cpp \
   src/game/gamecontrollerdata.cpp \
   src/game/gamecontrollerdetection.cpp \
   src/game/gamecontrollerintegration.cpp \
   src/game/gamedeserializedata.cpp \
   src/game/gamemechanism.cpp \
   src/game/gamemechanismdeserializer.cpp \
   src/game/gamenode.cpp \
   src/game/gamestate.cpp \
   src/game/gun.cpp \
   src/game/health.cpp \
   src/game/hitbox.cpp \
   src/game/infolayer.cpp \
   src/game/ingamemenu.cpp \
   src/game/ingamemenuarchives.cpp \
   src/game/ingamemenuaudio.cpp \
   src/game/ingamemenuinventory.cpp \
   src/game/ingamemenumap.cpp \
   src/game/ingamemenupage.cpp \
   src/game/inventory.cpp \
   src/game/inventorybasedcontrols.cpp \
   src/game/inventoryitemdescriptionreader.cpp \
   src/game/layerdata.cpp \
   src/game/level.cpp \
   src/game/leveldescription.cpp \
   src/game/levelfiles.cpp \
   src/game/levels.cpp \
   src/game/levelscript.cpp \
   src/game/luainterface.cpp \
   src/game/luanode.cpp \
   src/game/mechanisms/blockingrect.cpp \
   src/game/mechanisms/bouncer.cpp \
   src/game/mechanisms/bouncerwrapper.cpp \
   src/game/mechanisms/bubblecube.cpp \
   src/game/mechanisms/checkpoint.cpp \
   src/game/mechanisms/collapsingplatform.cpp \
   src/game/mechanisms/controllerhelp.cpp \
   src/game/mechanisms/conveyorbelt.cpp \
   src/game/mechanisms/crusher.cpp \
   src/game/mechanisms/damagerect.cpp \
   src/game/mechanisms/deathblock.cpp \
   src/game/mechanisms/dialogue.cpp \
   src/game/mechanisms/door.cpp \
   src/game/mechanisms/dust.cpp \
   src/game/mechanisms/extra.cpp \
   src/game/mechanisms/extrawrapper.cpp \
   src/game/mechanisms/fan.cpp \
   src/game/mechanisms/fireflies.cpp \
   src/game/mechanisms/imagelayer.cpp \
   src/game/mechanisms/infooverlay.cpp \
   src/game/mechanisms/interactionhelp.cpp \
   src/game/mechanisms/laser.cpp \
   src/game/mechanisms/lever.cpp \
   src/game/mechanisms/levermechanismmerger.cpp \
   src/game/mechanisms/moveablebox.cpp \
   src/game/mechanisms/movingplatform.cpp \
   src/game/mechanisms/onoffblock.cpp \
   src/game/mechanisms/portal.cpp \
   src/game/mechanisms/portalwrapper.cpp \
   src/game/mechanisms/rope.cpp \
   src/game/mechanisms/ropewithlight.cpp \
   src/game/mechanisms/rotatingblade.cpp \
   src/game/mechanisms/sensorrect.cpp \
   src/game/mechanisms/shaderlayer.cpp \
   src/game/mechanisms/smokeeffect.cpp \
   src/game/mechanisms/soundemitter.cpp \
   src/game/mechanisms/spikeball.cpp \
   src/game/mechanisms/spikeblock.cpp \
   src/game/mechanisms/spikes.cpp \
   src/game/mechanisms/staticlight.cpp \
   src/game/mechanisms/textlayer.cpp \
   src/game/mechanisms/treasurechest.cpp \
   src/game/mechanisms/waterdamage.cpp \
   src/game/mechanisms/watersurface.cpp \
   src/game/mechanisms/weather.cpp \
   src/game/menuconfig.cpp \
   src/game/meshtools.cpp \
   src/game/messagebox.cpp \
   src/game/onewaywall.cpp \
   src/game/overlays/controlleroverlay.cpp \
   src/game/overlays/rainoverlay.cpp \
   src/game/overlays/thunderstormoverlay.cpp \
   src/game/overlays/weatheroverlay.cpp \
   src/game/parallaxlayer.cpp \
   src/game/parallaxsettings.cpp \
   src/game/parsedata.cpp \
   src/game/physics/physics.cpp \
   src/game/physics/physicsconfiguration.cpp \
   src/game/physics/physicsconfigurationui.cpp \
   src/game/player/player.cpp \
   src/game/player/playeranimation.cpp \
   src/game/player/playeraudio.cpp \
   src/game/player/playerbelt.cpp \
   src/game/player/playerclimb.cpp \
   src/game/player/playercontrols.cpp \
   src/game/player/playerdash.cpp \
   src/game/player/playerinfo.cpp \
   src/game/player/playerinput.cpp \
   src/game/player/playerjump.cpp \
   src/game/player/timerlock.cpp \
   src/game/playerstencil.cpp \
   src/game/playerutils.cpp \
   src/game/preloader.cpp \
   src/game/projectile.cpp \
   src/game/projectilehitanimation.cpp \
   src/game/projectilehitaudio.cpp \
   src/game/room.cpp \
   src/game/roomupdater.cpp \
   src/game/savestate.cpp \
   src/game/screentransition.cpp \
   src/game/screentransitioneffect.cpp \
   src/game/scriptproperty.cpp \
   src/game/shaders/atmosphereshader.cpp \
   src/game/shaders/blurshader.cpp \
   src/game/shaders/deathshader.cpp \
   src/game/shaders/gammashader.cpp \
   src/game/skill.cpp \
   src/game/squaremarcher.cpp \
   src/game/stenciltilemap.cpp \
   src/game/sword.cpp \
   src/game/test.cpp \
   src/game/texturepool.cpp \
   src/game/tilemap.cpp \
   src/game/tilemapfactory.cpp \
   src/game/tmxenemy.cpp \
   src/game/tweaks.cpp \
   src/game/volumeupdater.cpp \
   src/game/waterbubbles.cpp \
   src/game/weapon.cpp \
   src/game/weaponfactory.cpp \
   src/game/weaponproperties.cpp \
   src/game/weaponsystem.cpp \
   src/game/worldquery.cpp \
   src/main.cpp \
   src/menus/menu.cpp \
   src/menus/menuaudio.cpp \
   src/menus/menuscreen.cpp \
   src/menus/menuscreenachievements.cpp \
   src/menus/menuscreenaudio.cpp \
   src/menus/menuscreencontrols.cpp \
   src/menus/menuscreencredits.cpp \
   src/menus/menuscreenfileselect.cpp \
   src/menus/menuscreengame.cpp \
   src/menus/menuscreenmain.cpp \
   src/menus/menuscreennameselect.cpp \
   src/menus/menuscreenoptions.cpp \
   src/menus/menuscreenpause.cpp \
   src/menus/menuscreenvideo.cpp \



HEADERS += \
   src/framework/image/image.h \
   src/framework/image/layer.h \
   src/framework/image/psd.h \
   src/framework/image/tga.h \
   src/framework/joystick/gamecontroller.h \
   src/framework/joystick/gamecontrollerballvector.h \
   src/framework/joystick/gamecontrollerinfo.h \
   src/framework/math/box2dtools.h \
   src/framework/math/fbm.h \
   src/framework/math/hermitecurve.h \
   src/framework/math/hermitecurvekey.h \
   src/framework/math/maptools.h \
   src/framework/math/pathinterpolation.h \
   src/framework/math/sfmlmath.h \
   src/framework/tmxparser/tmxanimation.h \
   src/framework/tmxparser/tmxchunk.h \
   src/framework/tmxparser/tmxelement.h \
   src/framework/tmxparser/tmxframe.h \
   src/framework/tmxparser/tmximage.h \
   src/framework/tmxparser/tmximagelayer.h \
   src/framework/tmxparser/tmxlayer.h \
   src/framework/tmxparser/tmxobject.h \
   src/framework/tmxparser/tmxobjectgroup.h \
   src/framework/tmxparser/tmxparsedata.h \
   src/framework/tmxparser/tmxparser.h \
   src/framework/tmxparser/tmxpolygon.h \
   src/framework/tmxparser/tmxpolyline.h \
   src/framework/tmxparser/tmxproperties.h \
   src/framework/tmxparser/tmxproperty.h \
   src/framework/tmxparser/tmxtemplate.h \
   src/framework/tmxparser/tmxtile.h \
   src/framework/tmxparser/tmxtileset.h \
   src/framework/tmxparser/tmxtools.h \
   src/framework/tools/elapsedtimer.h \
   src/framework/tools/jsonconfiguration.h \
   src/framework/tools/log.h \
   src/framework/tools/logthread.h \
   src/framework/tools/scopeexit.h \
   src/framework/tools/stopwatch.h \
   src/game/ambientocclusion.h \
   src/game/animation.h \
   src/game/animationframedata.h \
   src/game/animationplayer.h \
   src/game/animationpool.h \
   src/game/animationsettings.h \
   src/game/arrow.h \
   src/game/atmosphere.h \
   src/game/audio.h \
   src/game/audiorange.h \
   src/game/audioupdatedata.h \
   src/game/bitmapfont.h \
   src/game/blendmodedeserializer.h \
   src/game/boomeffect.h \
   src/game/boomeffectenvelope.h \
   src/game/boomeffectenveloperandom.h \
   src/game/boomeffectenvelopesine.h \
   src/game/boomsettings.h \
   src/game/bow.h \
   src/game/bulletplayer.h \
   src/game/camerapanorama.h \
   src/game/cameraroomlock.h \
   src/game/camerasystem.h \
   src/game/camerasystemconfiguration.h \
   src/game/camerasystemconfigurationui.h \
   src/game/chainshapeanalyzer.h \
   src/game/chunk.h \
   src/game/console.h \
   src/game/constants.h \
   src/game/cutscene.h \
   src/game/debugdraw.h \
   src/game/detonationanimation.h \
   src/game/displaymode.h \
   src/game/drawstates.h \
   src/game/effects/spawneffect.h \
   src/game/enemydescription.h \
   src/game/eventdistributor.h \
   src/game/eventserializer.h \
   src/game/extratable.h \
   src/game/fadetransitioneffect.h \
   src/game/fixturenode.h \
   src/game/forestscene.h \
   src/game/framemapper.h \
   src/game/game.h \
   src/game/gameaudio.h \
   src/game/gameclock.h \
   src/game/gameconfiguration.h \
   src/game/gamecontactlistener.h \
   src/game/gamecontrollerdata.h \
   src/game/gamecontrollerdetection.h \
   src/game/gamecontrollerintegration.h \
   src/game/gamedeserializedata.h \
   src/game/gamemechanism.h \
   src/game/gamemechanismdeserializer.h \
   src/game/gamemechanismdeserializerconstants.h \
   src/game/gamenode.h \
   src/game/gamestate.h \
   src/game/gun.h \
   src/game/health.h \
   src/game/hitbox.h \
   src/game/infolayer.h \
   src/game/ingamemenu.h \
   src/game/ingamemenuarchives.h \
   src/game/ingamemenuaudio.h \
   src/game/ingamemenuinventory.h \
   src/game/ingamemenumap.h \
   src/game/ingamemenupage.h \
   src/game/inventory.h \
   src/game/inventorybasedcontrols.h \
   src/game/inventoryitemdescriptionreader.h \
   src/game/laser.h \
   src/game/layerdata.h \
   src/game/level.h \
   src/game/leveldescription.h \
   src/game/levelfiles.h \
   src/game/levels.h \
   src/game/levelscript.h \
   src/game/luaconstants.h \
   src/game/luainterface.h \
   src/game/luanode.h \
   src/game/mechanisms/blockingrect.h \
   src/game/mechanisms/bouncer.h \
   src/game/mechanisms/bouncerwrapper.h \
   src/game/mechanisms/bubblecube.h \
   src/game/mechanisms/checkpoint.h \
   src/game/mechanisms/collapsingplatform.h \
   src/game/mechanisms/controllerhelp.h \
   src/game/mechanisms/conveyorbelt.h \
   src/game/mechanisms/crusher.h \
   src/game/mechanisms/damagerect.h \
   src/game/mechanisms/deathblock.h \
   src/game/mechanisms/dialogue.h \
   src/game/mechanisms/door.h \
   src/game/mechanisms/dust.h \
   src/game/mechanisms/extra.h \
   src/game/mechanisms/extrawrapper.h \
   src/game/mechanisms/fan.h \
   src/game/mechanisms/fireflies.h \
   src/game/mechanisms/imagelayer.h \
   src/game/mechanisms/infooverlay.h \
   src/game/mechanisms/interactionhelp.h \
   src/game/mechanisms/lever.h \
   src/game/mechanisms/levermechanismmerger.h \
   src/game/mechanisms/moveablebox.h \
   src/game/mechanisms/movingplatform.h \
   src/game/mechanisms/onoffblock.h \
   src/game/mechanisms/portal.h \
   src/game/mechanisms/portalwrapper.h \
   src/game/mechanisms/rope.h \
   src/game/mechanisms/ropewithlight.h \
   src/game/mechanisms/rotatingblade.h \
   src/game/mechanisms/sensorrect.h \
   src/game/mechanisms/shaderlayer.h \
   src/game/mechanisms/soundemitter.h \
   src/game/mechanisms/spikeball.h \
   src/game/mechanisms/spikeblock.h \
   src/game/mechanisms/spikes.h \
   src/game/mechanisms/staticlight.h \
   src/game/mechanisms/textlayer.h \
   src/game/mechanisms/treasurechest.h \
   src/game/mechanisms/waterdamage.h \
   src/game/mechanisms/watersurface.h \
   src/game/mechanisms/weather.h \
   src/game/menuconfig.h \
   src/game/meshtools.h \
   src/game/messagebox.h \
   src/game/onewaywall.h \
   src/game/overlays/controlleroverlay.h \
   src/game/overlays/rainoverlay.h \
   src/game/overlays/thunderstormoverlay.h \
   src/game/overlays/weatheroverlay.h \
   src/game/parallaxlayer.h \
   src/game/parallaxsettings.h \
   src/game/parsedata.h \
   src/game/physics/physics.h \
   src/game/physics/physicsconfiguration.h \
   src/game/physics/physicsconfigurationui.h \
   src/game/player/player.h \
   src/game/player/playeranimation.h \
   src/game/player/playerattack.h \
   src/game/player/playeraudio.h \
   src/game/player/playerbelt.h \
   src/game/player/playerbend.h \
   src/game/player/playerclimb.h \
   src/game/player/playercontrols.h \
   src/game/player/playerdash.h \
   src/game/player/playerinfo.h \
   src/game/player/playerinput.h \
   src/game/player/playerjump.h \
   src/game/player/playerjumptrace.h \
   src/game/player/playerspeed.h \
   src/game/player/timerlock.h \
   src/game/playerstencil.h \
   src/game/playerutils.h \
   src/game/preloader.h \
   src/game/projectile.h \
   src/game/projectilehitanimation.h \
   src/game/projectilehitaudio.h \
   src/game/room.h \
   src/game/roomupdater.h \
   src/game/savestate.h \
   src/game/screentransition.h \
   src/game/screentransitioneffect.h \
   src/game/scriptproperty.h \
   src/game/shaders/atmosphereshader.h \
   src/game/shaders/blurshader.h \
   src/game/shaders/deathshader.h \
   src/game/shaders/gammashader.h \
   src/game/skill.h \
   src/game/squaremarcher.h \
   src/game/stenciltilemap.h \
   src/game/sword.h \
   src/game/test.h \
   src/game/texturepool.h \
   src/game/tilemap.h \
   src/game/tilemapfactory.h \
   src/game/tmxenemy.h \
   src/game/tweaks.h \
   src/game/valuereader.h \
   src/game/volumeupdater.h \
   src/game/waterbubbles.h \
   src/game/weapon.h \
   src/game/weaponfactory.h \
   src/game/weaponproperties.h \
   src/game/weaponsystem.h \
   src/game/worldquery.h \
   src/menus/menu.h \
   src/menus/menuaudio.h \
   src/menus/menuscreen.h \
   src/menus/menuscreenachievements.h \
   src/menus/menuscreenaudio.h \
   src/menus/menuscreencontrols.h \
   src/menus/menuscreencredits.h \
   src/menus/menuscreenfileselect.h \
   src/menus/menuscreengame.h \
   src/menus/menuscreenmain.h \
   src/menus/menuscreennameselect.h \
   src/menus/menuscreenoptions.h \
   src/menus/menuscreenpause.h \
   src/menus/menuscreenvideo.h \


# thirdparty

SOURCES += \
   thirdparty/tinyxml2/tinyxml2.cpp \


HEADERS += \
   thirdparty/tinyxml2/tinyxml2.h \
   thirdparty/json/json.hpp \


# add box2d

HEADERS += \
   thirdparty/box2d/include/box2d/b2_api.h \
   thirdparty/box2d/include/box2d/b2_block_allocator.h \
   thirdparty/box2d/include/box2d/b2_body.h \
   thirdparty/box2d/include/box2d/b2_broad_phase.h \
   thirdparty/box2d/include/box2d/b2_chain_shape.h \
   thirdparty/box2d/include/box2d/b2_circle_shape.h \
   thirdparty/box2d/include/box2d/b2_collision.h \
   thirdparty/box2d/include/box2d/b2_common.h \
   thirdparty/box2d/include/box2d/b2_contact.h \
   thirdparty/box2d/include/box2d/b2_contact_manager.h \
   thirdparty/box2d/include/box2d/b2_distance.h \
   thirdparty/box2d/include/box2d/b2_distance_joint.h \
   thirdparty/box2d/include/box2d/b2_draw.h \
   thirdparty/box2d/include/box2d/b2_dynamic_tree.h \
   thirdparty/box2d/include/box2d/b2_edge_shape.h \
   thirdparty/box2d/include/box2d/b2_fixture.h \
   thirdparty/box2d/include/box2d/b2_friction_joint.h \
   thirdparty/box2d/include/box2d/b2_gear_joint.h \
   thirdparty/box2d/include/box2d/b2_growable_stack.h \
   thirdparty/box2d/include/box2d/b2_joint.h \
   thirdparty/box2d/include/box2d/b2_math.h \
   thirdparty/box2d/include/box2d/b2_motor_joint.h \
   thirdparty/box2d/include/box2d/b2_mouse_joint.h \
   thirdparty/box2d/include/box2d/b2_polygon_shape.h \
   thirdparty/box2d/include/box2d/b2_prismatic_joint.h \
   thirdparty/box2d/include/box2d/b2_pulley_joint.h \
   thirdparty/box2d/include/box2d/b2_revolute_joint.h \
   thirdparty/box2d/include/box2d/b2_rope.h \
   thirdparty/box2d/include/box2d/b2_settings.h \
   thirdparty/box2d/include/box2d/b2_shape.h \
   thirdparty/box2d/include/box2d/b2_stack_allocator.h \
   thirdparty/box2d/include/box2d/b2_timer.h \
   thirdparty/box2d/include/box2d/b2_time_of_impact.h \
   thirdparty/box2d/include/box2d/b2_time_step.h \
   thirdparty/box2d/include/box2d/b2_types.h \
   thirdparty/box2d/include/box2d/b2_weld_joint.h \
   thirdparty/box2d/include/box2d/b2_wheel_joint.h \
   thirdparty/box2d/include/box2d/b2_world.h \
   thirdparty/box2d/include/box2d/b2_world_callbacks.h \
   thirdparty/box2d/include/box2d/box2d.h

SOURCES += \
   thirdparty/box2d/src/collision/b2_broad_phase.cpp \
   thirdparty/box2d/src/collision/b2_chain_shape.cpp \
   thirdparty/box2d/src/collision/b2_circle_shape.cpp \
   thirdparty/box2d/src/collision/b2_collide_circle.cpp \
   thirdparty/box2d/src/collision/b2_collide_edge.cpp \
   thirdparty/box2d/src/collision/b2_collide_polygon.cpp \
   thirdparty/box2d/src/collision/b2_collision.cpp \
   thirdparty/box2d/src/collision/b2_distance.cpp \
   thirdparty/box2d/src/collision/b2_dynamic_tree.cpp \
   thirdparty/box2d/src/collision/b2_edge_shape.cpp \
   thirdparty/box2d/src/collision/b2_polygon_shape.cpp \
   thirdparty/box2d/src/collision/b2_time_of_impact.cpp \
   thirdparty/box2d/src/common/b2_block_allocator.cpp \
   thirdparty/box2d/src/common/b2_draw.cpp \
   thirdparty/box2d/src/common/b2_math.cpp \
   thirdparty/box2d/src/common/b2_settings.cpp \
   thirdparty/box2d/src/common/b2_stack_allocator.cpp \
   thirdparty/box2d/src/common/b2_timer.cpp \
   thirdparty/box2d/src/dynamics/b2_body.cpp \
   thirdparty/box2d/src/dynamics/b2_chain_circle_contact.cpp \
   thirdparty/box2d/src/dynamics/b2_chain_circle_contact.h \
   thirdparty/box2d/src/dynamics/b2_chain_polygon_contact.cpp \
   thirdparty/box2d/src/dynamics/b2_chain_polygon_contact.h \
   thirdparty/box2d/src/dynamics/b2_circle_contact.cpp \
   thirdparty/box2d/src/dynamics/b2_circle_contact.h \
   thirdparty/box2d/src/dynamics/b2_contact.cpp \
   thirdparty/box2d/src/dynamics/b2_contact_manager.cpp \
   thirdparty/box2d/src/dynamics/b2_contact_solver.cpp \
   thirdparty/box2d/src/dynamics/b2_contact_solver.h \
   thirdparty/box2d/src/dynamics/b2_distance_joint.cpp \
   thirdparty/box2d/src/dynamics/b2_edge_circle_contact.cpp \
   thirdparty/box2d/src/dynamics/b2_edge_circle_contact.h \
   thirdparty/box2d/src/dynamics/b2_edge_polygon_contact.cpp \
   thirdparty/box2d/src/dynamics/b2_edge_polygon_contact.h \
   thirdparty/box2d/src/dynamics/b2_fixture.cpp \
   thirdparty/box2d/src/dynamics/b2_friction_joint.cpp \
   thirdparty/box2d/src/dynamics/b2_gear_joint.cpp \
   thirdparty/box2d/src/dynamics/b2_island.cpp \
   thirdparty/box2d/src/dynamics/b2_island.h \
   thirdparty/box2d/src/dynamics/b2_joint.cpp \
   thirdparty/box2d/src/dynamics/b2_motor_joint.cpp \
   thirdparty/box2d/src/dynamics/b2_mouse_joint.cpp \
   thirdparty/box2d/src/dynamics/b2_polygon_circle_contact.cpp \
   thirdparty/box2d/src/dynamics/b2_polygon_circle_contact.h \
   thirdparty/box2d/src/dynamics/b2_polygon_contact.cpp \
   thirdparty/box2d/src/dynamics/b2_polygon_contact.h \
   thirdparty/box2d/src/dynamics/b2_prismatic_joint.cpp \
   thirdparty/box2d/src/dynamics/b2_pulley_joint.cpp \
   thirdparty/box2d/src/dynamics/b2_revolute_joint.cpp \
   thirdparty/box2d/src/dynamics/b2_weld_joint.cpp \
   thirdparty/box2d/src/dynamics/b2_wheel_joint.cpp \
   thirdparty/box2d/src/dynamics/b2_world.cpp \
   thirdparty/box2d/src/dynamics/b2_world_callbacks.cpp \
   thirdparty/box2d/src/rope/b2_rope.cpp

SOURCES += \
   thirdparty/imgui/imgui-SFML.cpp \
   thirdparty/imgui/imgui.cpp \
   thirdparty/imgui/imgui_draw.cpp \
   thirdparty/imgui/imgui_tables.cpp \
   thirdparty/imgui/imgui_widgets.cpp

HEADERS += \
   thirdparty/imgui/imconfig-SFML.h \
   thirdparty/imgui/imconfig.h \
   thirdparty/imgui/imgui-SFML.h \
   thirdparty/imgui/imgui-SFML_export.h \
   thirdparty/imgui/imgui.h \
   thirdparty/imgui/imgui_internal.h \
   thirdparty/imgui/imstb_rectpack.h \
   thirdparty/imgui/imstb_textedit.h \
   thirdparty/imgui/imstb_truetype.h

OTHER_FILES += \
    data/shaders/parallax_frag.glsl \
    data/shaders/parallax_vert.glsl \
    data/shaders/bumpmap_frag.glsl \
    data/shaders/bumpmap_vert.glsl

DISTFILES += \
    README.md \
    data/config/camera.json \
    data/config/savestate.json \
    data/config/tweaks.json \
    data/game/ingame_ui.json \
    data/level-crypt/metroid.json \
    data/level-crypt/metroid.tmx \
    data/level-demo/demolevel.tmx \
    data/level-malte/malte.json \
    data/level-malte/malte.tmx \
    data/level.tmx \
    data/menus/achievements.psd \
    data/menus/audio.psd \
    data/menus/controls.psd \
    data/menus/credits.psd \
    data/menus/fileselect.psd \
    data/menus/game.psd \
    data/menus/nameselect.psd \
    data/menus/options.psd \
    data/menus/pause.psd \
    data/menus/titlescreen.psd \
    data/menus/video.psd \
    data/scripts/enemies/arrowtrap.lua \
    data/scripts/enemies/arrowtrap_2.lua \
    data/scripts/enemies/deprecated_bat.lua \
    data/scripts/enemies/bat_2.lua \
    data/scripts/enemies/blob_2.lua \
    data/scripts/enemies/bonefish.lua \
    data/scripts/enemies/cannon_2.lua \
    data/scripts/enemies/critter.lua \
    data/scripts/enemies/deprecated_dumb.lua \
    data/config/game.json \
    data/config/physics.json \
    data/scripts/enemies/deprecated_blob.lua \
    data/scripts/enemies/deprecated_endboss_1.lua \
    data/scripts/enemies/ghost.lua \
    data/scripts/enemies/interpolation.lua \
    data/scripts/enemies/deprecated_klonk.lua \
    data/scripts/enemies/klonk_2.lua \
    data/scripts/enemies/deprecated_skeleton.lua \
    data/scripts/enemies/minik_bomber.lua \
    data/scripts/enemies/nukumaru.lua \
    data/scripts/enemies/rat.lua \
    data/scripts/enemies/shadow_monk.lua \
    data/scripts/enemies/spiky.lua \
    data/scripts/enemies/vector2.lua \
    data/scripts/enemies/watermine.lua \
    data/shaders/death.frag \
    data/shaders/death.vert \
    data/scripts/enemies/deprecated_cannon.lua \
    data/shaders/flash.frag \
    data/shaders/light.frag \
    data/shaders/raycast.frag \
    data/shaders/raycast.vert \
    data/shaders/water.frag \
    data/scripts/enemies/landmine.lua \
    data/scripts/enemies/constants.lua \
    data/scripts/enemies/helpers.lua \
    data/scripts/enemies/vectorial2.lua \
    data/config/levels.json \
    data/shaders/waterfall.frag \
    data/shaders/waterfall.vert \
    data/sprites/animations.json \
    data/shaders/blur.frag \
    data/shaders/brightness.frag \
    data/shaders/pixelate.frag \
    data/shaders/water.frag \
    data/shaders/wave.vert \
    doc/lasers/lasers.md \
    doc/level_design.md \
    doc/level_design/advanced_topics.md \
    doc/level_design/designing_a_level.md \
    doc/level_design/enemies.md \
    doc/level_design/mechanisms.md \
    doc/level_design/readme.md \
    doc/level_design/visual_effects.md \
    doc/level_scripts/readme.md \
    doc/lua_interface/readme.md \
    doc/physics/readme.md
