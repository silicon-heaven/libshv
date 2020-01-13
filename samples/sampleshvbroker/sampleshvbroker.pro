message("including $$PWD")

isEmpty(SHV_PROJECT_TOP_BUILDDIR) {
        SHV_PROJECT_TOP_BUILDDIR = $$OUT_PWD/../..
}
else {
        message ( SHV_PROJECT_TOP_BUILDDIR is not empty and set to $$SHV_PROJECT_TOP_BUILDDIR )
        message ( This is obviously done in file $$SHV_PROJECT_TOP_SRCDIR/.qmake.conf )
}
message ( SHV_PROJECT_TOP_BUILDDIR == '$$SHV_PROJECT_TOP_BUILDDIR' )

LIBSHV_SRC_DIR = $$PWD/../..

QT -= gui
QT += core network

with-shvwebsockets {
	QT += websockets
	DEFINES += WITH_SHV_WEBSOCKETS
}

CONFIG += c++11

TEMPLATE = app
TARGET = sampleshvbroker

DESTDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin
unix:LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/lib
win32:LIBDIR = $$SHV_PROJECT_TOP_BUILDDIR/bin

LIBS += \
        -L$$LIBDIR \
        -L$$SHV_PROJECT_TOP_BUILDDIR/3rdparty/necrolog/lib \

LIBS += \
    -lnecrolog \
    -lshvchainpack \
    -lshvcore \
    -lshvcoreqt \
    -lshviotqt \
    -lshvbroker \

unix {
        LIBS += \
                -Wl,-rpath,\'\$\$ORIGIN/../lib\'
}

INCLUDEPATH += \
    $$LIBSHV_SRC_DIR/3rdparty/necrolog/include \
    $$LIBSHV_SRC_DIR/libshvchainpack/include \
    $$LIBSHV_SRC_DIR/libshvcore/include \
    $$LIBSHV_SRC_DIR/libshvcoreqt/include \
    $$LIBSHV_SRC_DIR/libshviotqt/include \
    $$LIBSHV_SRC_DIR/libshvbroker/include \

RESOURCES += \
        #shvbroker.qrc \


TRANSLATIONS += \

include (src/src.pri)


