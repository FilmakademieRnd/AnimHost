qt_add_library(AssimpLoader
    assimploaderplugin.cpp assimploaderplugin.h
    assimploaderplugin_global.h
    assimphelper.h assimphelper.cpp
)
target_include_directories(AssimpLoader PUBLIC
    ../PluginNodeInterface
    ../../core/DataLoader
    ../../../glm
    ../../../assimp/include
)

target_compile_definitions(AssimpLoader PUBLIC
    ASSIMPLOADERPLUGIN_LIBRARY
)

target_link_directories(AnimHostCore PUBLIC 
    ../../../assimp/lib/Debug
)

target_link_libraries(AssimpLoader PUBLIC
    PluginNodeInterface
    Qt::Core
    Qt::Gui
    Qt::Widgets
    assimp-vc143-mtd
    AnimHostCore
)



