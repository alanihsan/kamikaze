QT += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = hi_no_kirin
TEMPLATE = app

CONFIG += c++11
CONFIG += no_keywords

QMAKE_CXXFLAGS += -O3 -msse -msse2 -msse3

QMAKE_CXXFLAGS_DEBUG += -g -Wall -Og -Wno-error=unused-function \
	-Wextra -Wno-missing-field-initializers -Wno-sign-compare -Wno-type-limits  \
	-Wno-unknown-pragmas -Wno-unused-parameter -Wno-ignored-qualifiers          \
	-Wmissing-format-attribute -Wno-delete-non-virtual-dtor                     \
	-Wsizeof-pointer-memaccess -Wformat=2 -Wno-format-nonliteral -Wno-format-y2k\
	-fstrict-overflow -Wstrict-overflow=2 -Wno-div-by-zero -Wwrite-strings      \
	-Wlogical-op -Wundef -DDEBUG_THREADS -Wnonnull -Wstrict-aliasing=2          \
	-fno-omit-frame-pointer -Wno-error=unused-result -Wno-error=clobbered       \
	-fstack-protector-all --param=ssp-buffer-size=4 -Wno-maybe-uninitialized    \
	-Wunused-macros -Wmissing-include-dirs -Wuninitialized -Winit-self          \
	-Wtype-limits -fno-common -fno-nonansi-builtins -Wformat-extra-args         \
	-Wno-error=unused-local-typedefs -DWARN_PEDANTIC -Winit-self -Wdate-time    \
	-Warray-bounds -Werror -fdiagnostics-color=always -fsanitize=address

QMAKE_LFLAGS += -fsanitize=address

SOURCES += \
	main.cc \
	objects/cube.cc \
	objects/grid.cc \
	objects/levelset.cc \
    objects/object.cc \
	objects/volume.cc \
    objects/volumebase.cc \
	render/camera.cc \
	render/scene.cc \
	render/viewer.cc \
    ui/mainwindow.cc \
    ui/timelinewidget.cc \
	util/util_openvdb.cc \
	util/utils.cc \
    ui/levelsetdialog.cc \
    sculpt/brush.cc \
    sculpt/sculpt.cc \
    ui/xyzspinbox.cc \
	smoke/forces.cc \
	smoke/pressure.cc \
    smoke/smokesimulation.cc

HEADERS += \
	objects/cube.h \
	objects/grid.h \
	objects/levelset.h \
    objects/object.h \
	objects/volume.h \
    objects/volumebase.h \
	render/camera.h \
	render/scene.h \
	render/viewer.h \
    ui/mainwindow.h \
    ui/timelinewidget.h \
    util/util_input.h \
	util/util_openvdb.h \
	util/utils.h \
    util/util_render.h \
    ui/levelsetdialog.h \
    sculpt/brush.h \
    util/util_openvdb_process.h \
    sculpt/sculpt.h \
    ui/xyzspinbox.h \
	smoke/advection.h \
	smoke/forces.h \
	smoke/globals.h \
	smoke/types.h \
    smoke/smokesimulation.h \
    smoke/util_smoke.h

OTHER_FILES += \
	render/shaders/flat_shader.frag \
	render/shaders/flat_shader.vert \
	render/shaders/object.frag \
	render/shaders/object.vert \
    render/shaders/volume.frag \
    render/shaders/volume.vert \
    render/shaders/tree_topology.frag \
    render/shaders/tree_topology.vert

DEFINES += DWREAL_IS_DOUBLE=0
DEFINES += GLM_FORCE_RADIANS

INCLUDEPATH += objects/ render/ ui/ util/
INCLUDEPATH += /opt/lib/openvdb/include /opt/lib/openexr/include
INCLUDEPATH += /opt/lib/ego/include

LIBS += -lGL -lglut -lGLEW
LIBS += -L/opt/lib/openvdb/lib -lopenvdb -ltbb
LIBS += -L/opt/lib/openexr/lib -lHalf
LIBS += -L/opt/lib/blosc/lib -lblosc -lz
LIBS += -L/opt/lib/ego/lib -lego

unix {
	copy_files.commands = cp -r ../render/shaders/ .
}

QMAKE_EXTRA_TARGETS += copy_files
POST_TARGETDEPS += copy_files

FORMS += \
    ui/mainwindow.ui \
    ui/levelsetdialog.ui
