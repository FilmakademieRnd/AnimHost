set(target_name AnimHost)

qt_add_executable(${target_name}
    main.cpp
)

target_include_directories(${target_name} PRIVATE
    ${CMAKE_SOURCE_DIR}/animHost_Plugins/PluginInterface
    ${CMAKE_SOURCE_DIR}/core/QTNodes/include
    ${CMAKE_SOURCE_DIR}/core
    ../../glm
)

target_link_directories(${target_name} PRIVATE 
    ${CMAKE_SOURCE_DIR}/core/QTNodes/lib
)

target_link_libraries(${target_name} PRIVATE
    QTNodes
    Qt::Core
    Qt::Gui
    Qt::Widgets
    AnimHostCore
)

install(TARGETS ${target_name}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET AnimHost
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

#Copy QTNodes dll
# Define the paths to the prebuilt libraries
set(PREBUILT_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/core/QTNodes/lib)

if(WIN32)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(PREBUILT_LIBRARY_PATH ${PREBUILT_LIBRARY_DIR}/debug/QTNodes.dll)
    else()
        set(PREBUILT_LIBRARY_PATH ${PREBUILT_LIBRARY_DIR}/debug/QTNodes.dll)
    endif()
elseif(APPLE)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(PREBUILT_LIBRARY_PATH ${PREBUILT_LIBRARY_DIR}/Debug/QTNodes.dylib)
    else()
        set(PREBUILT_LIBRARY_PATH ${PREBUILT_LIBRARY_DIR}/Release/QTNodes.dylib)
    endif()
endif()

# Copy the prebuilt library to the output directory after the build is completed
add_custom_command(TARGET AnimHost POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PREBUILT_LIBRARY_PATH}
        $<TARGET_FILE_DIR:AnimHost>
)

add_custom_command(TARGET AnimHost POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/../assimp/bin/Debug/assimp-vc143-mtd.dll
        $<TARGET_FILE_DIR:AnimHost>
)
