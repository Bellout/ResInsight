
# Use this workaround until we're on 2.8.3 on all platforms and can use CMAKE_CURRENT_LIST_DIR directly 
if (${CMAKE_VERSION} VERSION_GREATER "2.8.2")
    set(CEE_CURRENT_LIST_DIR  ${CMAKE_CURRENT_LIST_DIR}/)
endif()

set (SOURCE_GROUP_HEADER_FILES
${CEE_CURRENT_LIST_DIR}RiaApplication.h
${CEE_CURRENT_LIST_DIR}RiaCompletionTypeCalculationScheduler.h
${CEE_CURRENT_LIST_DIR}RiaDefines.h
${CEE_CURRENT_LIST_DIR}RiaFractureDefines.h
${CEE_CURRENT_LIST_DIR}RiaPreferences.h
${CEE_CURRENT_LIST_DIR}RiaPorosityModel.h
${CEE_CURRENT_LIST_DIR}RiaSummaryCurveDefinition.h
${CEE_CURRENT_LIST_DIR}RiaRftPltCurveDefinition.h
${CEE_CURRENT_LIST_DIR}RiaViewRedrawScheduler.h
)

set (SOURCE_GROUP_SOURCE_FILES
${CEE_CURRENT_LIST_DIR}RiaApplication.cpp
${CEE_CURRENT_LIST_DIR}RiaCompletionTypeCalculationScheduler.cpp
${CEE_CURRENT_LIST_DIR}RiaDefines.cpp
${CEE_CURRENT_LIST_DIR}RiaFractureDefines.cpp
${CEE_CURRENT_LIST_DIR}RiaMain.cpp
${CEE_CURRENT_LIST_DIR}RiaPreferences.cpp
${CEE_CURRENT_LIST_DIR}RiaPorosityModel.cpp
${CEE_CURRENT_LIST_DIR}RiaSummaryCurveDefinition.cpp
${CEE_CURRENT_LIST_DIR}RiaRftPltCurveDefinition.cpp
${CEE_CURRENT_LIST_DIR}RiaViewRedrawScheduler.cpp
)

list(APPEND CODE_HEADER_FILES
${SOURCE_GROUP_HEADER_FILES}
)

list(APPEND CODE_SOURCE_FILES
${SOURCE_GROUP_SOURCE_FILES}
)

set (QT_MOC_HEADERS
${QT_MOC_HEADERS}
${CEE_CURRENT_LIST_DIR}RiaApplication.h
${CEE_CURRENT_LIST_DIR}RiaCompletionTypeCalculationScheduler.h
${CEE_CURRENT_LIST_DIR}RiaViewRedrawScheduler.h
)


source_group( "Application" FILES ${SOURCE_GROUP_HEADER_FILES} ${SOURCE_GROUP_SOURCE_FILES} ${CEE_CURRENT_LIST_DIR}CMakeLists_files.cmake )
