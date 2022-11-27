TEMPLATE = app
TARGET = deceptus
CONFIG_APP_NAME = deceptus

DEFINES += _USE_MATH_DEFINES
DEFINES += USE_GL

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

debug {
   CONFIG += console
   DEFINES += DEBUG
}

win32-msvc {
   # link debug symbols
   message("configured for msvc")
   QMAKE_CXXFLAGS_RELEASE += /Zi
   QMAKE_LFLAGS_RELEASE += /DEBUG /OPT:REF /OPT:ICF
}

linux|win32-g++ {
   message("configured for g++")
   QMAKE_CXXFLAGS += -std=c++20
   QMAKE_CXXFLAGS += -lc++fs
   QMAKE_CXXFLAGS += -lfmt
}

win32 {
   LIBS += -Llib64
   LIBS += -LSDL\lib\x64
   LIBS += -lSDL2
   LIBS += -lglu32
   LIBS += -lopengl32
   LIBS += -llua53

   # sfml
   LIBS += -Lsfml\lib
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


#sfml
INCLUDEPATH += sfml/include
DEPENDPATH += sfml/include


INCLUDEPATH += .
INCLUDEPATH += src
INCLUDEPATH += src/game
INCLUDEPATH += src/thirdparty

SOURCES += \
   src/framework/image/image.cpp \
   src/framework/image/layer.cpp \
   src/framework/image/psd.cpp \
   src/framework/image/tga.cpp \
   src/framework/joystick/gamecontroller.cpp \
   src/framework/joystick/gamecontrollerinfo.cpp \
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
   src/game/bitmapfont.cpp \
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
   src/game/chainshapeanalyzer.cpp \
   src/game/chunk.cpp \
   src/game/console.cpp \
   src/game/cutscene.cpp \
   src/game/debugdraw.cpp \
   src/game/detonationanimation.cpp \
   src/game/displaymode.cpp \
   src/game/drawstates.cpp \
   src/game/effects/dust.cpp \
   src/game/effects/lightsystem.cpp \
   src/game/effects/smokeeffect.cpp \
   src/game/effects/staticlight.cpp \
   src/game/enemydescription.cpp \
   src/game/eventserializer.cpp \
   src/game/extrahealth.cpp \
   src/game/extraitem.cpp \
   src/game/extramanager.cpp \
   src/game/extraskill.cpp \
   src/game/extratable.cpp \
   src/game/fadetransitioneffect.cpp \
   src/game/fixturenode.cpp \
   src/game/forestscene.cpp \
   src/game/game.cpp \
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
   src/game/hitbox.cpp \
   src/game/imagelayer.cpp \
   src/game/infolayer.cpp \
   src/game/ingamemenu.cpp \
   src/game/ingamemenuarchives.cpp \
   src/game/ingamemenuinventory.cpp \
   src/game/ingamemenumap.cpp \
   src/game/ingamemenupage.cpp \
   src/game/inventory.cpp \
   src/game/inventoryitem.cpp \
   src/game/layerdata.cpp \
   src/game/level.cpp \
   src/game/leveldescription.cpp \
   src/game/levels.cpp \
   src/game/levelscript.cpp \
   src/game/luainterface.cpp \
   src/game/luanode.cpp \
   src/game/mechanisms/bouncer.cpp \
   src/game/mechanisms/bubblecube.cpp \
   src/game/mechanisms/checkpoint.cpp \
   src/game/mechanisms/collapsingplatform.cpp \
   src/game/mechanisms/controllerhelp.cpp \
   src/game/mechanisms/conveyorbelt.cpp \
   src/game/mechanisms/crusher.cpp \
   src/game/mechanisms/deathblock.cpp \
   src/game/mechanisms/dialogue.cpp \
   src/game/mechanisms/door.cpp \
   src/game/mechanisms/fan.cpp \
   src/game/mechanisms/laser.cpp \
   src/game/mechanisms/lever.cpp \
   src/game/mechanisms/moveablebox.cpp \
   src/game/mechanisms/movingplatform.cpp \
    src/game/mechanisms/onoffblock.cpp \
   src/game/mechanisms/portal.cpp \
   src/game/mechanisms/rope.cpp \
   src/game/mechanisms/ropewithlight.cpp \
    src/game/mechanisms/rotatingblade.cpp \
   src/game/mechanisms/sensorrect.cpp \
   src/game/mechanisms/shaderlayer.cpp \
   src/game/mechanisms/spikeball.cpp \
   src/game/mechanisms/spikeblock.cpp \
   src/game/mechanisms/spikes.cpp \
   src/game/meshtools.cpp \
   src/game/messagebox.cpp \
   src/game/onewaywall.cpp \
   src/game/overlays/controlleroverlay.cpp \
   src/game/overlays/rainoverlay.cpp \
   src/game/overlays/thunderstormoverlay.cpp \
   src/game/overlays/weatheroverlay.cpp \
   src/game/physics/physics.cpp \
   src/game/physics/physicsconfiguration.cpp \
   src/game/player/player.cpp \
   src/game/player/playeranimation.cpp \
   src/game/player/playerbelt.cpp \
   src/game/player/playerclimb.cpp \
   src/game/player/playercontrols.cpp \
   src/game/player/playerinfo.cpp \
   src/game/player/playerinput.cpp \
   src/game/player/playerjump.cpp \
   src/game/player/timerlock.cpp \
   src/game/preloader.cpp \
   src/game/projectile.cpp \
   src/game/projectilehitanimation.cpp \
   src/game/room.cpp \
   src/game/savestate.cpp \
   src/game/screentransition.cpp \
   src/game/screentransitioneffect.cpp \
   src/game/scriptproperty.cpp \
   src/game/shaders/atmosphereshader.cpp \
   src/game/shaders/blurshader.cpp \
   src/game/shaders/deathshader.cpp \
   src/game/shaders/gammashader.cpp \
   src/game/squaremarcher.cpp \
   src/game/stenciltilemap.cpp \
   src/game/sword.cpp \
   src/game/test.cpp \
   src/game/texturepool.cpp \
   src/game/tilemap.cpp \
   src/game/tilemapfactory.cpp \
   src/game/tmxenemy.cpp \
   src/game/tweaks.cpp \
   src/game/waterbubbles.cpp \
   src/game/weapon.cpp \
   src/game/weaponfactory.cpp \
   src/game/weaponsystem.cpp \
   src/game/weather.cpp \
   src/game/worldquery.cpp \
   src/main.cpp \
   src/menus/menu.cpp \
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
   src/game/boomeffectenvelope.h \
   src/game/boomeffectenveloperandom.h \
   src/game/boomeffectenvelopesine.h \
   src/game/boomsettings.h \
   src/game/chainshapeanalyzer.h \
   src/game/chunk.h \
   src/game/drawstates.h \
   src/game/gamemechanismdeserializerconstants.h \
   src/game/ambientocclusion.h \
   src/game/animation.h \
   src/game/animationframedata.h \
   src/game/animationplayer.h \
   src/game/animationpool.h \
   src/game/animationsettings.h \
   src/game/arrow.h \
   src/game/atmosphere.h \
   src/game/audio.h \
   src/game/bitmapfont.h \
   src/game/boomeffect.h \
   src/game/bow.h \
   src/game/bulletplayer.h \
   src/game/camerapanorama.h \
   src/game/cameraroomlock.h \
   src/game/camerasystem.h \
   src/game/camerasystemconfiguration.h \
   src/game/console.h \
   src/game/constants.h \
   src/game/cutscene.h \
   src/game/debugdraw.h \
   src/game/detonationanimation.h \
   src/game/displaymode.h \
   src/game/effects/dust.h \
   src/game/enemydescription.h \
   src/game/eventserializer.h \
   src/game/extra.h \
   src/game/extrahealth.h \
   src/game/extraitem.h \
   src/game/extramanager.h \
   src/game/extraskill.h \
   src/game/extratable.h \
   src/game/fadetransitioneffect.h \
   src/game/fixturenode.h \
   src/game/forestscene.h \
   src/game/game.h \
   src/game/gameclock.h \
   src/game/gameconfiguration.h \
   src/game/gamecontactlistener.h \
   src/game/gamecontrollerdata.h \
   src/game/gamecontrollerdetection.h \
   src/game/gamecontrollerintegration.h \
   src/game/gamedeserializedata.h \
   src/game/gamemechanism.h \
   src/game/gamemechanismdeserializer.h \
   src/game/gamenode.h \
   src/game/gamestate.h \
   src/game/gun.h \
   src/game/hitbox.h \
   src/game/imagelayer.h \
   src/game/infolayer.h \
   src/game/ingamemenu.h \
   src/game/ingamemenuarchives.h \
   src/game/ingamemenuinventory.h \
   src/game/ingamemenumap.h \
   src/game/ingamemenupage.h \
   src/game/inventory.h \
   src/game/inventoryitem.h \
   src/game/laser.h \
   src/game/layerdata.h \
   src/game/level.h \
   src/game/leveldescription.h \
   src/game/levels.h \
   src/game/levelscript.h \
   src/game/luaconstants.h \
   src/game/luainterface.h \
   src/game/luanode.h \
   src/game/mechanisms/bouncer.h \
   src/game/mechanisms/bubblecube.h \
   src/game/mechanisms/checkpoint.h \
   src/game/mechanisms/collapsingplatform.h \
   src/game/mechanisms/controllerhelp.h \
   src/game/mechanisms/conveyorbelt.h \
   src/game/mechanisms/crusher.h \
   src/game/mechanisms/deathblock.h \
   src/game/mechanisms/dialogue.h \
   src/game/mechanisms/door.h \
   src/game/mechanisms/fan.h \
   src/game/mechanisms/lever.h \
   src/game/mechanisms/moveablebox.h \
   src/game/mechanisms/movingplatform.h \
    src/game/mechanisms/onoffblock.h \
   src/game/mechanisms/portal.h \
   src/game/mechanisms/rope.h \
   src/game/mechanisms/ropewithlight.h \
    src/game/mechanisms/rotatingblade.h \
   src/game/mechanisms/sensorrect.h \
   src/game/mechanisms/shaderlayer.h \
   src/game/mechanisms/spikeball.h \
   src/game/mechanisms/spikeblock.h \
   src/game/mechanisms/spikes.h \
   src/game/meshtools.h \
   src/game/messagebox.h \
   src/game/onewaywall.h \
   src/game/overlays/controlleroverlay.h \
   src/game/overlays/rainoverlay.h \
   src/game/overlays/thunderstormoverlay.h \
   src/game/overlays/weatheroverlay.h \
   src/game/physics/physics.h \
   src/game/physics/physicsconfiguration.h \
   src/game/player/player.h \
   src/game/player/playeranimation.h \
   src/game/player/playerattack.h \
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
   src/game/preloader.h \
   src/game/projectile.h \
   src/game/projectilehitanimation.h \
   src/game/room.h \
   src/game/savestate.h \
   src/game/screentransition.h \
   src/game/screentransitioneffect.h \
   src/game/scriptproperty.h \
   src/game/shaders/atmosphereshader.h \
   src/game/shaders/blurshader.h \
   src/game/shaders/deathshader.h \
   src/game/shaders/gammashader.h \
   src/game/squaremarcher.h \
   src/game/stenciltilemap.h \
   src/game/sword.h \
   src/game/test.h \
   src/game/texturepool.h \
   src/game/tilemap.h \
   src/game/tilemapfactory.h \
   src/game/tmxenemy.h \
   src/game/tweaks.h \
   src/game/waterbubbles.h \
   src/game/weapon.h \
   src/game/weaponfactory.h \
   src/game/weaponsystem.h \
   src/game/weather.h \
   src/game/worldquery.h \
   src/menus/menu.h \
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
   src/thirdparty/tinyxml2/tinyxml2.cpp \


HEADERS += \
   src/thirdparty/tinyxml2/tinyxml2.h \
   src/thirdparty/json/json.hpp \



# add box2d

SOURCES += \
    src/thirdparty/Box2D/Collision/Shapes/b2ChainShape.cpp \
    src/thirdparty/Box2D/Collision/Shapes/b2CircleShape.cpp \
    src/thirdparty/Box2D/Collision/Shapes/b2EdgeShape.cpp \
    src/thirdparty/Box2D/Collision/Shapes/b2PolygonShape.cpp \
    src/thirdparty/Box2D/Common/b2BlockAllocator.cpp \
    src/thirdparty/Box2D/Common/b2Draw.cpp \
    src/thirdparty/Box2D/Common/b2Math.cpp \
    src/thirdparty/Box2D/Common/b2Settings.cpp \
    src/thirdparty/Box2D/Common/b2StackAllocator.cpp \
    src/thirdparty/Box2D/Common/b2Timer.cpp \
    src/thirdparty/Box2D/Collision/b2BroadPhase.cpp \
    src/thirdparty/Box2D/Collision/b2CollideCircle.cpp \
    src/thirdparty/Box2D/Collision/b2CollideEdge.cpp \
    src/thirdparty/Box2D/Collision/b2CollidePolygon.cpp \
    src/thirdparty/Box2D/Collision/b2Collision.cpp \
    src/thirdparty/Box2D/Collision/b2Distance.cpp \
    src/thirdparty/Box2D/Collision/b2DynamicTree.cpp \
    src/thirdparty/Box2D/Collision/b2TimeOfImpact.cpp \
    src/thirdparty/Box2D/Dynamics/b2Body.cpp \
    src/thirdparty/Box2D/Dynamics/b2ContactManager.cpp \
    src/thirdparty/Box2D/Dynamics/b2Fixture.cpp \
    src/thirdparty/Box2D/Dynamics/b2Island.cpp \
    src/thirdparty/Box2D/Dynamics/b2World.cpp \
    src/thirdparty/Box2D/Dynamics/b2WorldCallbacks.cpp \
    src/thirdparty/Box2D/Rope/b2Rope.cpp \
    src/thirdparty/Box2D/Dynamics/Joints/b2DistanceJoint.cpp \
    src/thirdparty/Box2D/Dynamics/Joints/b2FrictionJoint.cpp \
    src/thirdparty/Box2D/Dynamics/Joints/b2GearJoint.cpp \
    src/thirdparty/Box2D/Dynamics/Joints/b2Joint.cpp \
    src/thirdparty/Box2D/Dynamics/Joints/b2MotorJoint.cpp \
    src/thirdparty/Box2D/Dynamics/Joints/b2MouseJoint.cpp \
    src/thirdparty/Box2D/Dynamics/Joints/b2PrismaticJoint.cpp \
    src/thirdparty/Box2D/Dynamics/Joints/b2PulleyJoint.cpp \
    src/thirdparty/Box2D/Dynamics/Joints/b2RevoluteJoint.cpp \
    src/thirdparty/Box2D/Dynamics/Joints/b2RopeJoint.cpp \
    src/thirdparty/Box2D/Dynamics/Joints/b2WeldJoint.cpp \
    src/thirdparty/Box2D/Dynamics/Joints/b2WheelJoint.cpp \
    src/thirdparty/Box2D/Dynamics/Contacts/b2ChainAndCircleContact.cpp \
    src/thirdparty/Box2D/Dynamics/Contacts/b2ChainAndPolygonContact.cpp \
    src/thirdparty/Box2D/Dynamics/Contacts/b2CircleContact.cpp \
    src/thirdparty/Box2D/Dynamics/Contacts/b2Contact.cpp \
    src/thirdparty/Box2D/Dynamics/Contacts/b2ContactSolver.cpp \
    src/thirdparty/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.cpp \
    src/thirdparty/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.cpp \
    src/thirdparty/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.cpp \
    src/thirdparty/Box2D/Dynamics/Contacts/b2PolygonContact.cpp

HEADERS += \
    src/thirdparty/Box2D/Collision/Shapes/b2ChainShape.h \
    src/thirdparty/Box2D/Collision/Shapes/b2CircleShape.h \
    src/thirdparty/Box2D/Collision/Shapes/b2EdgeShape.h \
    src/thirdparty/Box2D/Collision/Shapes/b2PolygonShape.h \
    src/thirdparty/Box2D/Collision/Shapes/b2Shape.h \
    src/thirdparty/Box2D/Box2D.h \
    src/thirdparty/Box2D/Common/b2BlockAllocator.h \
    src/thirdparty/Box2D/Common/b2Draw.h \
    src/thirdparty/Box2D/Common/b2GrowableStack.h \
    src/thirdparty/Box2D/Common/b2Math.h \
    src/thirdparty/Box2D/Common/b2Settings.h \
    src/thirdparty/Box2D/Common/b2StackAllocator.h \
    src/thirdparty/Box2D/Common/b2Timer.h \
    src/thirdparty/Box2D/Collision/b2BroadPhase.h \
    src/thirdparty/Box2D/Collision/b2Collision.h \
    src/thirdparty/Box2D/Collision/b2Distance.h \
    src/thirdparty/Box2D/Collision/b2DynamicTree.h \
    src/thirdparty/Box2D/Collision/b2TimeOfImpact.h \
    src/thirdparty/Box2D/Dynamics/b2Body.h \
    src/thirdparty/Box2D/Dynamics/b2ContactManager.h \
    src/thirdparty/Box2D/Dynamics/b2Fixture.h \
    src/thirdparty/Box2D/Dynamics/b2Island.h \
    src/thirdparty/Box2D/Dynamics/b2TimeStep.h \
    src/thirdparty/Box2D/Dynamics/b2World.h \
    src/thirdparty/Box2D/Dynamics/b2WorldCallbacks.h \
    src/thirdparty/Box2D/Rope/b2Rope.h \
    src/thirdparty/Box2D/Dynamics/Joints/b2DistanceJoint.h \
    src/thirdparty/Box2D/Dynamics/Joints/b2FrictionJoint.h \
    src/thirdparty/Box2D/Dynamics/Joints/b2GearJoint.h \
    src/thirdparty/Box2D/Dynamics/Joints/b2Joint.h \
    src/thirdparty/Box2D/Dynamics/Joints/b2MotorJoint.h \
    src/thirdparty/Box2D/Dynamics/Joints/b2MouseJoint.h \
    src/thirdparty/Box2D/Dynamics/Joints/b2PrismaticJoint.h \
    src/thirdparty/Box2D/Dynamics/Joints/b2PulleyJoint.h \
    src/thirdparty/Box2D/Dynamics/Joints/b2RevoluteJoint.h \
    src/thirdparty/Box2D/Dynamics/Joints/b2RopeJoint.h \
    src/thirdparty/Box2D/Dynamics/Joints/b2WeldJoint.h \
    src/thirdparty/Box2D/Dynamics/Joints/b2WheelJoint.h \
    src/thirdparty/Box2D/Dynamics/Contacts/b2ChainAndCircleContact.h \
    src/thirdparty/Box2D/Dynamics/Contacts/b2ChainAndPolygonContact.h \
    src/thirdparty/Box2D/Dynamics/Contacts/b2CircleContact.h \
    src/thirdparty/Box2D/Dynamics/Contacts/b2Contact.h \
    src/thirdparty/Box2D/Dynamics/Contacts/b2ContactSolver.h \
    src/thirdparty/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.h \
    src/thirdparty/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.h \
    src/thirdparty/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.h \
    src/thirdparty/Box2D/Dynamics/Contacts/b2PolygonContact.h

OTHER_FILES += \
    data/shaders/parallax_frag.glsl \
    data/shaders/parallax_vert.glsl \
    data/shaders/bumpmap_frag.glsl \
    data/shaders/bumpmap_vert.glsl

DISTFILES += \
    data/config/camera.json \
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
    data/scripts/enemies/rat.lua \
    data/scripts/enemies/vector2.lua \
    data/scripts/enemies/watermine.lua \
    data/shaders/death.frag \
    data/shaders/death.vert \
    data/scripts/enemies/deprecated_cannon.lua \
    data/shaders/light.frag \
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
    doc/lua_interface/readme.md \
    doc/physics/readme.md
