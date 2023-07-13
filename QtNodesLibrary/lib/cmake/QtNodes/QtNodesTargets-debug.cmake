#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "QtNodes::QtNodes" for configuration "Debug"
set_property(TARGET QtNodes::QtNodes APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(QtNodes::QtNodes PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/QtNodes.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/QtNodes.dll"
  )

list(APPEND _cmake_import_check_targets QtNodes::QtNodes )
list(APPEND _cmake_import_check_files_for_QtNodes::QtNodes "${_IMPORT_PREFIX}/lib/QtNodes.lib" "${_IMPORT_PREFIX}/bin/QtNodes.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
