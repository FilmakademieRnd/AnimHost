set(target_name AnimHost)

qt_add_executable(${target_name}
    main.cpp
)

target_include_directories(${target_name} PRIVATE
    ${CMAKE_SOURCE_DIR}/core/QTNodes/include
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/animHost_Plugins/PluginInterface
    ${CMAKE_SOURCE_DIR}/animHost_Plugins/
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
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)


install(SCRIPT ${deploy_script})

# Copy QTNodes dll
# Define the paths to the prebuilt libraries
set(PREBUILT_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/core/QTNodes/bin)

if(WIN32)
    set(PREBUILT_LIBRARY_PATH ${PREBUILT_LIBRARY_DIR}/$<$<CONFIG:Debug>:Debug/QTNodes.dll>$<$<NOT:$<CONFIG:Debug>>:Release/QTNodes.dll>)
elseif(APPLE)
    set(PREBUILT_LIBRARY_PATH ${PREBUILT_LIBRARY_DIR}/$<$<CONFIG:Debug>:Debug/QTNodes.dylib>$<$<NOT:$<CONFIG:Debug>>:Release/QTNodes.dylib>)
endif()

install(FILES
    ${PREBUILT_LIBRARY_PATH}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Copy the prebuilt QT dll to the output directory after the build is completed
if(WIN32)
    set(TOOL_WINDEPLOYQT ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/Qt6/bin/$<$<CONFIG:Debug>:windeployqt.debug.bat>$<$<NOT:$<CONFIG:Debug>>:windeployqt>)
    add_custom_command(TARGET AnimHost POST_BUILD
    COMMAND ${TOOL_WINDEPLOYQT}
            $<TARGET_FILE:AnimHost>
    COMMENT "Running windeployqt..."
)
else()
    message("Not Windows")
endif()

# Copy the prebuilt library to the output directory after the build is completed
add_custom_command(TARGET AnimHost POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PREBUILT_LIBRARY_PATH}
        ${CMAKE_SOURCE_DIR}/../assimp/bin/Debug/assimp-vc143-mtd.dll
        ${CMAKE_SOURCE_DIR}/animHost_Plugins/BasicOnnxPlugin/onnxruntime/lib/onnxruntime.dll
        $<TARGET_FILE_DIR:AnimHost>
)
