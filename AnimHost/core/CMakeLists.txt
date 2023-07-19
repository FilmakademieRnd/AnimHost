qt_add_library(AnimHostCore
    animhostcore.h animhostcore.cpp
    animhostnode.h animhostnode.cpp
    commondatatypes.h commondatatypes.cpp
    nodedatatypes.h
    sourcedatanode.h sourcedatanode.cpp
)

target_compile_definitions(AnimHostCore PUBLIC
    ANIMHOSTCORE_LIBRARY
)

target_include_directories(AnimHostCore PUBLIC
    ../../glm
    ./QTNodes/include
    ../animHost_Plugins/PluginInterface
)



target_link_directories(AnimHostCore PUBLIC 
    ./QTNodes/lib
)

target_link_libraries(AnimHostCore PUBLIC
    QTNodes
    Qt::Core
    Qt::Gui
)


