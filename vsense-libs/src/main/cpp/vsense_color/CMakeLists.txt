PROJECT(vsense_color)

IF(MSVC)
	FILE(GLOB INC_FILES ${VSENSE_INCLUDE_DIR}/vsense/color/*.h)    
ENDIF()

FILE(GLOB SRC_FILES vsense/color/*.cpp)

ADD_LIBRARY(${PROJECT_NAME} STATIC ${SRC_FILES} ${INC_FILES})

IF(ANDROID)
	SET_TARGET_PROPERTIES(${PROJECT_NAME}
                      PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY
                      "${VSENSE_INCLUDE_DIR}/../lib/color/${ANDROID_ABI}")
ENDIF()