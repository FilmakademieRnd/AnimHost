set(target_name core)

qt_add_executable(core MACOSX_BUNDLE
    animhost.cpp animhost.h
    animhostnode.cpp animhostnode.h
    commondatatypes.h
    main.cpp
)
target_include_directories(core PRIVATE
    ../animHost_Plugins/PluginInterface
    ${CMAKE_SOURCE_DIR}/core/QTNodes/include
    ../../glm
)

target_link_directories(${target_name} PRIVATE 
    ${CMAKE_SOURCE_DIR}/core/QTNodes/lib
)

target_link_libraries(core PRIVATE
    QTNodes
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS core
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET core
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
