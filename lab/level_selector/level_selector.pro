TEMPLATE = app
CONFIG += c++17

CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += "imgui"
INCLUDEPATH += "SDL/include"

LIBS += -lopengl32
LIBS += -lgdi32
LIBS += -LSDL\lib\x64
LIBS += -lSDL2

HEADERS += \
   imgui/imconfig.h \
   imgui/imgui.h \
   imgui_impl_sdl \
   imgui/imgui_internal.h \
   imgui/imstb_rectpack.h \
   imgui/imstb_textedit.h \
   imgui/imstb_truetype.h

SOURCES += \
   imgui/backends/imgui_impl_sdl.cpp \
   imgui/backends/imgui_impl_sdlrenderer.cpp \
   imgui/imgui.cpp \
   imgui/imgui_draw.cpp \
   imgui/imgui_tables.cpp \
   imgui/imgui_widgets.cpp \
   main.cpp


