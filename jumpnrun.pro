TEMPLATE = app
TARGET = deceptus
CONFIG_APP_NAME = deceptus

DEFINES += _USE_MATH_DEFINES
DEFINES += USE_GL

CONFIG += c++17
CONFIG -= debug_and_release

OBJECTS_DIR = .obj

# get rid of all qt
CONFIG -= QT
CONFIG -= app_bundle
QT -= gui
QT -= core
LIBS -= -lQtGui
LIBS -= -lQtCore

#debug {
   CONFIG += console
#}

# code requires c++17
win32 {
   # visual studio wants 'latest' for C++17 support
   # QMAKE_CXXFLAGS += -std:c++latest

   # link debug symbols
   QMAKE_CFLAGS_RELEASE += -Zi
   QMAKE_CXXFLAGS_RELEASE += -Zi
   QMAKE_LFLAGS_RELEASE += /DEBUG /OPT:REF
}

linux {
   QMAKE_CXXFLAGS += -std=c++17
   QMAKE_CXXFLAGS += -lc++fs
   QMAKE_CXXFLAGS += -Wno-multichar
}

# openmp support
# msvc:QMAKE_CXXFLAGS_RELEASE += /openmp
# QMAKE_CXXFLAGS += -openmp


win32 {
   LIBS += -Llib64

   LIBS += -LSDL\lib\x64
   LIBS += -lSDL2
   LIBS += -lglu32
   LIBS += -lopengl32
   LIBS += -llua53

   # LIBS += -lvcomp

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
   # apt install:
   # libsfml-dev libsdl2-dev liblua5.3-dev
   LIBS += -lstdc++fs
   LIBS += -llua5.3
   LIBS += -lSDL2
   LIBS += -lGL

   LIBS += -Lsfml/lib
   LIBS += -lsfml-audio
   LIBS += -lsfml-graphics
   LIBS += -lsfml-network
   LIBS += -lsfml-window
   LIBS += -lsfml-system
}


#sfml
INCLUDEPATH += sfml/include
DEPENDPATH += sfml/include


INCLUDEPATH += .
INCLUDEPATH += src
INCLUDEPATH += src/game


SOURCES += \
   src/effects/blur.cpp \
   src/effects/effect.cpp \
   src/effects/pixelate.cpp \
   src/effects/raycastlight.cpp \
   src/effects/smokeeffect.cpp \
   src/effects/staticlight.cpp \
   src/game/actioncontrollermap.cpp \
   src/game/ambientocclusion.cpp \
   src/game/animation.cpp \
   src/game/animationpool.cpp \
   src/game/animationsettings.cpp \
   src/game/atmosphere.cpp \
   src/game/atmosphereshader.cpp \
   src/game/audio.cpp \
   src/game/bitmapfont.cpp \
   src/game/blurshader.cpp \
   src/game/boomeffect.cpp \
   src/game/bow.cpp \
   src/game/bullet.cpp \
   src/game/bullethitanimation.cpp \
   src/game/callbackmap.cpp \
   src/game/camerapane.cpp \
   src/game/camerasystem.cpp \
   src/game/camerasystemconfiguration.cpp \
   src/game/checkpoint.cpp \
   src/game/checksum.cpp \
   src/game/console.cpp \
   src/game/controlleroverlay.cpp \
   src/game/deathshader.cpp \
   src/game/debugdraw.cpp \
   src/game/dialogue.cpp \
   src/game/displaymode.cpp \
   src/game/enemy.cpp \
   src/game/enemydescription.cpp \
   src/game/extrahealth.cpp \
   src/game/extraitem.cpp \
   src/game/extramanager.cpp \
   src/game/extraskill.cpp \
   src/game/extratable.cpp \
   src/game/fbm.cpp \
   src/game/fixturenode.cpp \
   src/game/forestscene.cpp \
   src/game/game.cpp \
   src/game/gameconfiguration.cpp \
   src/game/gamecontactlistener.cpp \
   src/game/gamecontrollerdata.cpp \
   src/game/gamecontrollerintegration.cpp \
   src/game/gamemechanism.cpp \
   src/game/gamenode.cpp \
   src/game/gamestate.cpp \
   src/game/gammashader.cpp \
   src/game/globalclock.cpp \
   src/game/hermitecurve.cpp \
   src/game/imagelayer.cpp \
   src/game/infolayer.cpp \
   src/game/inventory.cpp \
   src/game/inventoryitem.cpp \
   src/game/inventorylayer.cpp \
   src/game/jumpplatform.cpp \
   src/game/level.cpp \
   src/game/leveldescription.cpp \
   src/game/levelmap.cpp \
   src/game/levels.cpp \
   src/game/luainterface.cpp \
   src/game/luanode.cpp \
   src/game/maptools.cpp \
   src/game/mechanisms/bouncer.cpp \
   src/game/mechanisms/conveyorbelt.cpp \
   src/game/mechanisms/crusher.cpp \
   src/game/mechanisms/deathblock.cpp \
   src/game/mechanisms/door.cpp \
   src/game/mechanisms/fan.cpp \
   src/game/mechanisms/laser.cpp \
   src/game/mechanisms/lever.cpp \
   src/game/mechanisms/movingplatform.cpp \
   src/game/mechanisms/portal.cpp \
   src/game/mechanisms/spikeball.cpp \
   src/game/mechanisms/spikes.cpp \
   src/game/meshtools.cpp \
   src/game/messagebox.cpp \
   src/game/moveablebox.cpp \
   src/game/pathinterpolation.cpp \
   src/game/physics.cpp \
   src/game/physicsconfiguration.cpp \
   src/game/player.cpp \
   src/game/playerclimb.cpp \
   src/game/playerconfiguration.cpp \
   src/game/playercontrols.cpp \
   src/game/playerinfo.cpp \
   src/game/playerjump.cpp \
   src/game/progresssettings.cpp \
   src/game/rainoverlay.cpp \
   src/game/room.cpp \
   src/game/savestate.cpp \
   src/game/scriptproperty.cpp \
   src/game/sfmlmath.cpp \
   src/game/squaremarcher.cpp \
   src/game/test.cpp \
   src/game/texturepool.cpp \
   src/game/tilemap.cpp \
   src/game/timer.cpp \
   src/game/weapon.cpp \
   src/game/weaponsystem.cpp \
   src/game/weather.cpp \
   src/game/weatheroverlay.cpp \
   src/game/worldconfiguration.cpp \
   src/image/image.cpp \
   src/image/layer.cpp \
   src/image/psd.cpp \
   src/image/tga.cpp \
   src/joystick/gamecontroller.cpp \
   src/joystick/gamecontrollerinfo.cpp \
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
   src/poly2tri/common/shapes.cc \
   src/poly2tri/sweep/advancing_front.cc \
   src/poly2tri/sweep/cdt.cc \
   src/poly2tri/sweep/sweep.cc \
   src/poly2tri/sweep/sweep_context.cc \
   src/tinyxml2/tinyxml2.cpp \
   src/tmxparser/tmxanimation.cpp \
   src/tmxparser/tmxchunk.cpp \
   src/tmxparser/tmxelement.cpp \
   src/tmxparser/tmxframe.cpp \
   src/tmxparser/tmximage.cpp \
   src/tmxparser/tmximagelayer.cpp \
   src/tmxparser/tmxlayer.cpp \
   src/tmxparser/tmxobject.cpp \
   src/tmxparser/tmxobjectgroup.cpp \
   src/tmxparser/tmxparser.cpp \
   src/tmxparser/tmxpolygon.cpp \
   src/tmxparser/tmxpolyline.cpp \
   src/tmxparser/tmxproperties.cpp \
   src/tmxparser/tmxproperty.cpp \
   src/tmxparser/tmxtile.cpp \
   src/tmxparser/tmxtileset.cpp \
   src/tmxparser/tmxtools.cpp \


# add box2d

HEADERS += \
   src/easings/easings.h \
   src/effects/blur.h \
   src/effects/effect.h \
   src/effects/pixelate.h \
   src/effects/raycastlight.h \
   src/effects/smokeeffect.h \
   src/effects/staticlight.h \
   src/game/actioncontrollermap.h \
   src/game/ambientocclusion.h \
   src/game/animation.h \
   src/game/animationpool.h \
   src/game/animationsettings.h \
   src/game/atmosphere.h \
   src/game/atmosphereshader.h \
   src/game/audio.h \
   src/game/bitmapfont.h \
   src/game/blurshader.h \
   src/game/boomeffect.h \
   src/game/bow.h \
   src/game/bullet.h \
   src/game/bullethitanimation.h \
   src/game/bulletplayer.h \
   src/game/callbackmap.h \
   src/game/camerapane.h \
   src/game/camerasystem.h \
   src/game/camerasystemconfiguration.h \
   src/game/checkpoint.h \
   src/game/checksum.h \
   src/game/console.h \
   src/game/constants.h \
   src/game/controlleroverlay.h \
   src/game/deathshader.h \
   src/game/debugdraw.h \
   src/game/dialogue.h \
   src/game/displaymode.h \
   src/game/enemy.h \
   src/game/enemydescription.h \
   src/game/extra.h \
   src/game/extrahealth.h \
   src/game/extraitem.h \
   src/game/extramanager.h \
   src/game/extraskill.h \
   src/game/extratable.h \
   src/game/fbm.h \
   src/game/fixturenode.h \
   src/game/forestscene.h \
   src/game/game.h \
   src/game/gameconfiguration.h \
   src/game/gamecontactlistener.h \
   src/game/gamecontrollerdata.h \
   src/game/gamecontrollerintegration.h \
   src/game/gamemechanism.h \
   src/game/gamenode.h \
   src/game/gamestate.h \
   src/game/gammashader.h \
   src/game/globalclock.h \
   src/game/hermitecurve.h \
   src/game/hermitecurvekey.h \
   src/game/imagelayer.h \
   src/game/infolayer.h \
   src/game/inventory.h \
   src/game/inventoryitem.h \
   src/game/inventorylayer.h \
   src/game/jumpplatform.h \
   src/game/laser.h \
   src/game/level.h \
   src/game/leveldescription.h \
   src/game/levelmap.h \
   src/game/levels.h \
   src/game/luaconstants.h \
   src/game/luainterface.h \
   src/game/luanode.h \
   src/game/maptools.h \
   src/game/mechanisms/bouncer.h \
   src/game/mechanisms/conveyorbelt.h \
   src/game/mechanisms/crusher.h \
   src/game/mechanisms/deathblock.h \
   src/game/mechanisms/door.h \
   src/game/mechanisms/fan.h \
   src/game/mechanisms/lever.h \
   src/game/mechanisms/movingplatform.h \
   src/game/mechanisms/portal.h \
   src/game/mechanisms/spikeball.h \
   src/game/mechanisms/spikes.h \
   src/game/meshtools.h \
   src/game/messagebox.h \
   src/game/moveablebox.h \
   src/game/pathinterpolation.h \
   src/game/physics.h \
   src/game/physicsconfiguration.h \
   src/game/player.h \
   src/game/playerclimb.h \
   src/game/playerconfiguration.h \
   src/game/playercontrols.h \
   src/game/playerinfo.h \
   src/game/playerjump.h \
   src/game/progresssettings.h \
   src/game/rainoverlay.h \
   src/game/room.h \
   src/game/savestate.h \
   src/game/scriptproperty.h \
   src/game/sfmlmath.h \
   src/game/squaremarcher.h \
   src/game/test.h \
   src/game/texturepool.h \
   src/game/tilemap.h \
   src/game/timer.h \
   src/game/weapon.h \
   src/game/weaponsystem.h \
   src/game/weather.h \
   src/game/weatheroverlay.h \
   src/game/worldconfiguration.h \
   src/image/image.h \
   src/image/layer.h \
   src/image/psd.h \
   src/image/tga.h \
   src/joystick/gamecontroller.h \
   src/joystick/gamecontrollerballvector.h \
   src/joystick/gamecontrollerinfo.h \
   src/json/json.hpp \
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
   src/poly2tri/common/shapes.h \
   src/poly2tri/common/utils.h \
   src/poly2tri/poly2tri.h \
   src/poly2tri/sweep/advancing_front.h \
   src/poly2tri/sweep/cdt.h \
   src/poly2tri/sweep/sweep.h \
   src/poly2tri/sweep/sweep_context.h \
   src/tinyxml2/tinyxml2.h \
   src/tmxparser/tmxanimation.h \
   src/tmxparser/tmxchunk.h \
   src/tmxparser/tmxelement.h \
   src/tmxparser/tmxframe.h \
   src/tmxparser/tmximage.h \
   src/tmxparser/tmximagelayer.h \
   src/tmxparser/tmxlayer.h \
   src/tmxparser/tmxobject.h \
   src/tmxparser/tmxobjectgroup.h \
   src/tmxparser/tmxparser.h \
   src/tmxparser/tmxpolygon.h \
   src/tmxparser/tmxpolyline.h \
   src/tmxparser/tmxproperties.h \
   src/tmxparser/tmxproperty.h \
   src/tmxparser/tmxtile.h \
   src/tmxparser/tmxtileset.h \
   src/tmxparser/tmxtools.h \


SOURCES += \
    src/Box2D/Collision/Shapes/b2ChainShape.cpp \
    src/Box2D/Collision/Shapes/b2CircleShape.cpp \
    src/Box2D/Collision/Shapes/b2EdgeShape.cpp \
    src/Box2D/Collision/Shapes/b2PolygonShape.cpp \
    src/Box2D/Common/b2BlockAllocator.cpp \
    src/Box2D/Common/b2Draw.cpp \
    src/Box2D/Common/b2Math.cpp \
    src/Box2D/Common/b2Settings.cpp \
    src/Box2D/Common/b2StackAllocator.cpp \
    src/Box2D/Common/b2Timer.cpp \
    src/Box2D/Collision/b2BroadPhase.cpp \
    src/Box2D/Collision/b2CollideCircle.cpp \
    src/Box2D/Collision/b2CollideEdge.cpp \
    src/Box2D/Collision/b2CollidePolygon.cpp \
    src/Box2D/Collision/b2Collision.cpp \
    src/Box2D/Collision/b2Distance.cpp \
    src/Box2D/Collision/b2DynamicTree.cpp \
    src/Box2D/Collision/b2TimeOfImpact.cpp \
    src/Box2D/Dynamics/b2Body.cpp \
    src/Box2D/Dynamics/b2ContactManager.cpp \
    src/Box2D/Dynamics/b2Fixture.cpp \
    src/Box2D/Dynamics/b2Island.cpp \
    src/Box2D/Dynamics/b2World.cpp \
    src/Box2D/Dynamics/b2WorldCallbacks.cpp \
    src/Box2D/Rope/b2Rope.cpp \
    src/Box2D/Dynamics/Joints/b2DistanceJoint.cpp \
    src/Box2D/Dynamics/Joints/b2FrictionJoint.cpp \
    src/Box2D/Dynamics/Joints/b2GearJoint.cpp \
    src/Box2D/Dynamics/Joints/b2Joint.cpp \
    src/Box2D/Dynamics/Joints/b2MotorJoint.cpp \
    src/Box2D/Dynamics/Joints/b2MouseJoint.cpp \
    src/Box2D/Dynamics/Joints/b2PrismaticJoint.cpp \
    src/Box2D/Dynamics/Joints/b2PulleyJoint.cpp \
    src/Box2D/Dynamics/Joints/b2RevoluteJoint.cpp \
    src/Box2D/Dynamics/Joints/b2RopeJoint.cpp \
    src/Box2D/Dynamics/Joints/b2WeldJoint.cpp \
    src/Box2D/Dynamics/Joints/b2WheelJoint.cpp \
    src/Box2D/Dynamics/Contacts/b2ChainAndCircleContact.cpp \
    src/Box2D/Dynamics/Contacts/b2ChainAndPolygonContact.cpp \
    src/Box2D/Dynamics/Contacts/b2CircleContact.cpp \
    src/Box2D/Dynamics/Contacts/b2Contact.cpp \
    src/Box2D/Dynamics/Contacts/b2ContactSolver.cpp \
    src/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.cpp \
    src/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.cpp \
    src/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.cpp \
    src/Box2D/Dynamics/Contacts/b2PolygonContact.cpp

HEADERS += \
    src/Box2D/Collision/Shapes/b2ChainShape.h \
    src/Box2D/Collision/Shapes/b2CircleShape.h \
    src/Box2D/Collision/Shapes/b2EdgeShape.h \
    src/Box2D/Collision/Shapes/b2PolygonShape.h \
    src/Box2D/Collision/Shapes/b2Shape.h \
    src/Box2D/Box2D.h \
    src/Box2D/Common/b2BlockAllocator.h \
    src/Box2D/Common/b2Draw.h \
    src/Box2D/Common/b2GrowableStack.h \
    src/Box2D/Common/b2Math.h \
    src/Box2D/Common/b2Settings.h \
    src/Box2D/Common/b2StackAllocator.h \
    src/Box2D/Common/b2Timer.h \
    src/Box2D/Collision/b2BroadPhase.h \
    src/Box2D/Collision/b2Collision.h \
    src/Box2D/Collision/b2Distance.h \
    src/Box2D/Collision/b2DynamicTree.h \
    src/Box2D/Collision/b2TimeOfImpact.h \
    src/Box2D/Dynamics/b2Body.h \
    src/Box2D/Dynamics/b2ContactManager.h \
    src/Box2D/Dynamics/b2Fixture.h \
    src/Box2D/Dynamics/b2Island.h \
    src/Box2D/Dynamics/b2TimeStep.h \
    src/Box2D/Dynamics/b2World.h \
    src/Box2D/Dynamics/b2WorldCallbacks.h \
    src/Box2D/Rope/b2Rope.h \
    src/Box2D/Dynamics/Joints/b2DistanceJoint.h \
    src/Box2D/Dynamics/Joints/b2FrictionJoint.h \
    src/Box2D/Dynamics/Joints/b2GearJoint.h \
    src/Box2D/Dynamics/Joints/b2Joint.h \
    src/Box2D/Dynamics/Joints/b2MotorJoint.h \
    src/Box2D/Dynamics/Joints/b2MouseJoint.h \
    src/Box2D/Dynamics/Joints/b2PrismaticJoint.h \
    src/Box2D/Dynamics/Joints/b2PulleyJoint.h \
    src/Box2D/Dynamics/Joints/b2RevoluteJoint.h \
    src/Box2D/Dynamics/Joints/b2RopeJoint.h \
    src/Box2D/Dynamics/Joints/b2WeldJoint.h \
    src/Box2D/Dynamics/Joints/b2WheelJoint.h \
    src/Box2D/Dynamics/Contacts/b2ChainAndCircleContact.h \
    src/Box2D/Dynamics/Contacts/b2ChainAndPolygonContact.h \
    src/Box2D/Dynamics/Contacts/b2CircleContact.h \
    src/Box2D/Dynamics/Contacts/b2Contact.h \
    src/Box2D/Dynamics/Contacts/b2ContactSolver.h \
    src/Box2D/Dynamics/Contacts/b2EdgeAndCircleContact.h \
    src/Box2D/Dynamics/Contacts/b2EdgeAndPolygonContact.h \
    src/Box2D/Dynamics/Contacts/b2PolygonAndCircleContact.h \
    src/Box2D/Dynamics/Contacts/b2PolygonContact.h

OTHER_FILES += \
    data/shaders/parallax_frag.glsl \
    data/shaders/parallax_vert.glsl \
    data/shaders/bumpmap_frag.glsl \
    data/shaders/bumpmap_vert.glsl

DISTFILES += \
    data/config/camera.json \
    data/level-crypt/metroid.json \
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
    data/scripts/enemies/bat.lua \
    data/scripts/enemies/bonefish.lua \
    data/scripts/enemies/cannon_2.lua \
    data/scripts/enemies/critter.lua \
    data/scripts/enemies/dumb.lua \
    data/config/game.json \
    data/config/physics.json \
    data/scripts/enemies/blob.lua \
    data/scripts/enemies/endboss_1.lua \
    data/scripts/enemies/ghost.lua \
    data/scripts/enemies/interpolation.lua \
    data/scripts/enemies/klonk.lua \
    data/scripts/enemies/skeleton.lua \
    data/scripts/enemies/spikeball.lua \
    data/scripts/enemies/vector2.lua \
    data/scripts/enemies/watermine.lua \
    data/shaders/death.frag \
    data/shaders/death.vert \
    data/shaders/raycast.frag \
    data/shaders/raycast.vert \
    data/scripts/enemies/cannon.lua \
    data/shaders/water.frag \
    data/scripts/enemies/landmine.lua \
    data/scripts/enemies/constants.lua \
    data/scripts/enemies/helpers.lua \
    data/scripts/enemies/vectorial2.lua \
    data/config/levels.json \
    data/sprites/animations.json \
    data/shaders/blur.frag \
    data/shaders/brightness.frag \
    data/shaders/pixelate.frag \
    data/shaders/raycast.frag \
    data/shaders/water.frag \
    data/shaders/raycast.vert \
    data/shaders/wave.vert \
    data/level-crypt/metroid.tmx


