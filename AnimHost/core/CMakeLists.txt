qt_add_library(AnimHostCore
    animhostcore.h animhostcore.cpp
    animhostnode.h animhostnode.cpp
    animhostinputnode.h animhostinputnode.cpp
    animhostoperationnode.h animhostoperationnode.cpp
    commondatatypes.h commondatatypes.cpp
    nodedatatypes.h
    sourcedatanode.h sourcedatanode.cpp
    animhosthelper.h animhosthelper.cpp
)

target_compile_definitions(AnimHostCore PUBLIC
    ANIMHOSTCORE_LIBRARY
)

target_include_directories(AnimHostCore PUBLIC
    ../../glm
    ./QTNodes/include
    ../animHost_Plugins/PluginInterface
    ../animHost_Plugins/PluginNodeInterface
    ./
)



target_link_directories(AnimHostCore PUBLIC 
    ./QTNodes/lib
)

target_link_libraries(AnimHostCore PUBLIC
    QTNodes
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


