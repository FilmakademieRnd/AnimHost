qt_add_library(AnimHostCore
    animhostcore.h animhostcore.cpp
    animhostnode.h animhostnode.cpp
    animhostinputnode.h animhostinputnode.cpp
    animhostoperationnode.h animhostoperationnode.cpp
    commondatatypes.h commondatatypes.cpp
    nodedatatypes.h
    sourcedatanode.h sourcedatanode.cpp
    DataLoader/assimploadernode.h DataLoader/assimploadernode.cpp
    DataLoader/assimphelper.h DataLoader/assimphelper.cpp
)

target_compile_definitions(AnimHostCore PUBLIC
    ANIMHOSTCORE_LIBRARY
)

target_include_directories(AnimHostCore PUBLIC
    ../../glm
    ../../assimp/include
    ./QTNodes/include
    ../animHost_Plugins/PluginInterface
    ./
    ./DataLoader
)



target_link_directories(AnimHostCore PUBLIC 
    ./QTNodes/lib
    ../../assimp/lib/Debug
)

target_link_libraries(AnimHostCore PUBLIC
    QTNodes
    Qt::Core
    Qt::Gui
    Qt::Widgets
    assimp-vc142-mtd
)


