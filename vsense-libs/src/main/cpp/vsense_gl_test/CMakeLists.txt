PROJECT(vsense_gl_test)

SET(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
SET(BIN_FOLDERS "${_qt5Widgets_install_prefix}/bin;${BIN_FOLDERS}")

FIND_PACKAGE(glm REQUIRED)
INCLUDE_DIRECTORIES(${GLM_INCLUDE_DIRS})

FILE(GLOB SRC_FILES *.cpp)

FILE(GLOB INC_FILES *.h)

ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_FILES} ${INC_FILES})
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/project.vcxproj.user.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.vcxproj.user @ONLY) 

qt5_use_modules(${PROJECT_NAME} Core OpenGL Widgets)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} vsense_color)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} vsense_depth)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} vsense_gl)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} vsense_io)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} vsense_pc)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} vsense_sh)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} vsense_em)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} opengl32 glu32)