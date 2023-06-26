#general configuration of Qt and app
QT += gui \
      widgets
QT += sql

CONFIG += c++11 console
CONFIG += sdk_no_version_check
CONFIG -= app_bundle

#../../../nodeeditor/include

HEADERS += \
    animhost.h \
    animhostnode.h \
    commondatatypes.h \

SOURCES += \
        animhost.cpp \
        animhostnode.cpp \
        main.cpp

#include analyser plugin interface
win32:LIBS += -L$$PWD/../animHost_Plugins/PluginInterface/lib -lPluginInterface

INCLUDEPATH += $$PWD/../animHost_Plugins/PluginInterface \
#DEPENDPATH += $$PWD/../animHost_analyserPlugins/PluginInterface

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


#QtNodes
macx: LIBS += -L$$PWD/QTNodes/lib/release -lQtNodes
win32: LIBS += -L$$PWD/QTNodes/lib/release -lQtNodes

INCLUDEPATH += $$PWD/QTNodes/include
DEPENDPATH += $$PWD/QTNodes/include
