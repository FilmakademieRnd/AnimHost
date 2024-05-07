set(target_name CoordinateConverterPlugin)

qt_add_library(${target_name}
    CoordinateConverterPlugin.cpp CoordinateConverterPlugin.h
    CoordinateConverterPlugin_global.h
)
target_include_directories(${target_name} PUBLIC
    ../PluginNodeInterface
    ../../../glm
)

target_compile_definitions(${target_name} PUBLIC
    COORDINATECONVERTERPLUGIN_LIBRARY
)

target_link_libraries(${target_name} PUBLIC
    PluginNodeInterface
    AnimHostCore
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
