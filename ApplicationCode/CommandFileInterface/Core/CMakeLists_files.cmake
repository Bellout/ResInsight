
set (SOURCE_GROUP_HEADER_FILES
${CMAKE_CURRENT_LIST_DIR}/RicfCommandObject.h
${CMAKE_CURRENT_LIST_DIR}/RicfFieldCapability.h
${CMAKE_CURRENT_LIST_DIR}/RicfFieldHandle.h
${CMAKE_CURRENT_LIST_DIR}/RicfObjectCapability.h
${CMAKE_CURRENT_LIST_DIR}/RifcCommandFileReader.h
)

set (SOURCE_GROUP_SOURCE_FILES
${CMAKE_CURRENT_LIST_DIR}/RicfCommandObject.cpp
${CMAKE_CURRENT_LIST_DIR}/RicfFieldCapability.cpp
${CMAKE_CURRENT_LIST_DIR}/RicfFieldHandle.cpp
${CMAKE_CURRENT_LIST_DIR}/RicfObjectCapability.cpp
${CMAKE_CURRENT_LIST_DIR}/RifcCommandFileReader.cpp
)

list(APPEND CODE_HEADER_FILES
${SOURCE_GROUP_HEADER_FILES}
)

list(APPEND CODE_SOURCE_FILES
${SOURCE_GROUP_SOURCE_FILES}
)

source_group( "CommandFileInterface\\Core" FILES ${SOURCE_GROUP_HEADER_FILES} ${SOURCE_GROUP_SOURCE_FILES} ${CMAKE_CURRENT_LIST_DIR}/CMakeLists_files.cmake )