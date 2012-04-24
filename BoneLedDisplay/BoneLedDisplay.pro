TEMPLATE = app

TARGET = BoneLedDisplay

win32* {
    DESTDIR = Release
}
else {
    DESTDIR = Output
}

QT += core gui network

CONFIG += release

# No debug in release builds
unix:QMAKE_CXXFLAGS_RELEASE -= -g

unix {
	CONFIG += link_pkgconfig
	macx:CONFIG -= app_bundle
	PKGCONFIG += syntro
}

DEFINES += QT_NETWORK_LIB

INCLUDEPATH += GeneratedFiles

win32-g++:LIBS += -L"$(SYNTRODIR)/bin"

win32-msvc*:LIBS += -L"$(SYNTRODIR)/lib"

win32 {
        DEFINES += _CRT_SECURE_NO_WARNINGS
        INCLUDEPATH += $(SYNTRODIR)/include
        LIBS += -lSyntroLib -lSyntroGUI
}

MOC_DIR += GeneratedFiles/release

OBJECTS_DIR += release

UI_DIR += GeneratedFiles

RCC_DIR += GeneratedFiles

include(BoneLedDisplay.pri)
