#-------------------------------------------------
#
# Project created by QtCreator 2014-05-15T16:31:50
#
#-------------------------------------------------

QT       -= gui

TARGET = xCairo
TEMPLATE = lib
CONFIG += staticlib

SOURCES +=

HEADERS +=
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
