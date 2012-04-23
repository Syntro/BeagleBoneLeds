TEMPLATE = app

TARGET = BoneLedDisplay
 
DESTDIR = Output 

QT += core gui network

CONFIG += release link_pkgconfig

# for now don't bundle Syntro apps
macx:CONFIG -= app_bundle

PKGCONFIG += syntro

DEFINES += QT_NETWORK_LIB

INCLUDEPATH += GeneratedFiles

DEPENDPATH +=

MOC_DIR += GeneratedFiles/release

OBJECTS_DIR += release

UI_DIR += GeneratedFiles

RCC_DIR += GeneratedFiles

include(BoneLedDisplay.pri)
