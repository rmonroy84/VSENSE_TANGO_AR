PROJECT(vsense_depth)

IF(MSVC)
	FILE(GLOB INC_FILES ${VSENSE_INCLUDE_DIR}/vsense/depth/*.h)
ELSEIF(ANDROID)
    FIND_PACKAGE(Tango REQUIRED)

    INCLUDE_DIRECTORIES(${Tango_INCLUDE_DIRS})
ENDIF()

FILE(GLOB SRC_FILES vsense/depth/*.cpp)

ADD_LIBRARY(${PROJECT_NAME} STATIC ${SRC_FILES} ${INC_FILES})

IF(ANDROID)
	SET_TARGET_PROPERTIES(${PROJECT_NAME}
                      PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY
                      "${VSENSE_INCLUDE_DIR}/../lib/depth/${ANDROID_ABI}")
ENDIF()