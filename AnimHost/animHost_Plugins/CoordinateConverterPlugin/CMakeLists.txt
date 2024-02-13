
qt_add_library(CoordinateConverterPlugin
    CoordinateConverterPlugin.cpp CoordinateConverterPlugin.h
    CoordinateConverterPlugin_global.h
)
target_include_directories(CoordinateConverterPlugin PUBLIC
    ../PluginNodeInterface
    ../../../glm
)

target_compile_definitions(CoordinateConverterPlugin PUBLIC
    COORDINATECONVERTERPLUGIN_LIBRARY
)

target_link_libraries(CoordinateConverterPlugin PUBLIC
    PluginNodeInterface
    AnimHostCore
)
