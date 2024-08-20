qt_add_library(AnimHostCore SHARED
    animhostcore.h animhostcore.cpp
    animhostnode.h animhostnode.cpp
    animhostinputnode.h animhostinputnode.cpp
    animhostoperationnode.h animhostoperationnode.cpp
    commondatatypes.h commondatatypes.cpp
    nodedatatypes.h
    sourcedatanode.h sourcedatanode.cpp
    animhosthelper.h animhosthelper.cpp
    ZMQMessageHandler.h ZMQMessageHandler.cpp
    UIUtils.h UIUtils.cpp
    FileHandler.h FileHandler.cpp
    FrameRange.h FrameRange.cpp
    MathUtils.h MathUtils.cpp
    PluginNodeInterface/pluginnodeinterface.h
    PluginNodeInterface/pluginnodeinterface.cpp
    animhostcore_global.h
    TimeSeries.h
)


set_target_properties (AnimHostCore PROPERTIES
    FOLDER Core
)

target_include_directories(AnimHostCore PUBLIC
    ../../glm
    ./QTNodes/include
    ../animHost_Plugins/PluginInterface
    ./PluginNodeInterface
    ./
)

target_link_directories(AnimHostCore PUBLIC 
    $<$<CONFIG:Debug>:${CMAKE_SOURCE_DIR}/core/QTNodes/lib/debug>
    $<$<CONFIG:Release>:${CMAKE_SOURCE_DIR}/core/QTNodes/lib/release>
)



find_package(cppzmq CONFIG REQUIRED)
find_package(Matplot++ REQUIRED)

target_compile_definitions(AnimHostCore PRIVATE
    ANIMHOSTCORE_LIBRARY
)

target_link_libraries(AnimHostCore PUBLIC  
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    cppzmq
    QTNodes
    Matplot++::matplot
)

target_include_directories(AnimHostCore INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


install(TARGETS AnimHostCore
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)



