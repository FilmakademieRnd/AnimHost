#general configuration of Qt and app
QT -= gui
QT += sql

CONFIG += c++11 console
CONFIG += sdk_no_version_check
CONFIG -= app_bundle

HEADERS += \
    animhost.h \
    commondatatypes.h \

SOURCES += \
        animhost.cpp \
        main.cpp

#include analyser plugin interface
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../animHost_Plugins/PluginInterface/lib/release/ -lPluginInterface
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../animHost_Plugins/PluginInterface/lib/debug/ -lPluginInterface
else:unix: LIBS += -L$$PWD/../animHost_Plugins/PluginInterface/lib/ -lPluginInterface

INCLUDEPATH += $$PWD/../animHost_Plugins/PluginInterface
DEPENDPATH += $$PWD/../animHost_analyserPlugins/PluginInterface

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


