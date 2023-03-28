#! [0]
#QT             -= gui
TEMPLATE        = lib
CONFIG         += c++11

TARGET          = PluginInterface
DEFINES += PLUGININTERFACE_LIBRARY

HEADERS += \
    plugininterface.h \
    plugininterface_global.h

SOURCES += 


DESTDIR         = $$PWD/lib #../../AnimHost/plugins
#! [0]

# install
#target.path = ../../AnimHost/plugins
INSTALLS += target

CONFIG += install_ok  # Do not cargo-cult this!
