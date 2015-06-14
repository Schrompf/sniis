include($$_PRO_FILE_PWD_/../../SourceExt/Traumklassen/props.pri)

TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    SNIIS_C.cpp \
    SNIIS_Intern.cpp \
    SNIIS_Win.cpp \
    SNIIS_Win_Joystick.cpp \
    SNIIS_Win_Keyboard.cpp \
    SNIIS_Win_Mouse.cpp \
    SNIIS_Linux.cpp \
    SNIIS_Linux_Mouse.cpp \
    SNIIS_Linux_Keyboard.cpp \
    SNIIS_Linux_Joystick.cpp \
    SNIIS_Mac.cpp \
    SNIIS_Mac_Mouse.cpp \
    SNIIS_Mac_Keyboard.cpp \
    SNIIS_Mac_Joystick.cpp

HEADERS += \
    SNIIS.h \
    SNIIS_C.h \
    SNIIS_Intern.h \
    SNIIS_Linux.h \
    SNIIS_Win.h \
    SNIIS_Mac.h \
    SNIIS_Mac_Helper.h

macx {
  OBJECTIVE_SOURCES += \
    SNIIS_Mac_Helper.m
}
