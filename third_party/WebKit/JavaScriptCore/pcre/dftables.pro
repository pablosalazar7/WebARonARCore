TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle qt
SOURCES = dftables.cpp
TARGET = dftables
DESTDIR = tmp

INCLUDEPATH += $$PWD/.. $$PWD/../wtf

gtk-port {
  DEFINES += BUILDING_GTK__ BUILDING_CAIRO__
} else {
  DEFINES += BUILDING_QT__
}
