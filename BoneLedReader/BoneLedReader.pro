TEMPLATE = app

TARGET = BoneLedReader

DESTDIR = Output

QT += core gui network

CONFIG += release link_pkgconfig
PKGCONFIG += syntro

# No debug in release builds
unix:QMAKE_CXXFLAGS_RELEASE -= -g

DEFINES += QT_NETWORK_LIB

INCLUDEPATH += GeneratedFiles

MOC_DIR += GeneratedFiles/release

OBJECTS_DIR += release

UI_DIR += GeneratedFiles

RCC_DIR += GeneratedFiles

include(BoneLedReader.pri)
