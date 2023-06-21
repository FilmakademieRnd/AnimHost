#! [0]
#QT             -= gui
TEMPLATE        = lib
CONFIG         += c++11

TARGET          = ExamplePlugin

DEFINES += EXAMPLEPLUGIN_LIBRARY

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../PluginInterface/lib/release/ -lPluginInterface
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../PluginInterface/lib/debug/ -lPluginInterface
else:unix: LIBS += -L$$PWD/../PluginInterface/lib/ -lPluginInterface

INCLUDEPATH += $$PWD/../PluginInterface
DEPENDPATH += $$PWD/../PluginInterface

HEADERS += \
    exampleplugin_global.h \
    exampleplugin.h

SOURCES += \
    exampleplugin.cpp

DESTDIR = ../../core/plugins
#! [0]

EXAMPLE_FILES = exampleplugin.json

# install
#target.path = ../../core/plugins
INSTALLS += target

CONFIG += install_ok  # Do not cargo-cult this!

#QtNodes
macx: LIBS += -L$$PWD/../../core/QTNodes/lib/release/ -lQtNodes

INCLUDEPATH += $$PWD/../../core/QTNodes/include
DEPENDPATH += $$PWD/../../core/QTNodes/include
