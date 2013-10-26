cmake_minimum_required (VERSION 2.8)
project (PCViewer)
set (PCViewer_VERSION_MAJOR 0)
set (PCViewer_VERSION_MINOR 1)

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
find_package (SDL2 REQUIRED)
find_package (OpenGLES2 REQUIRED)

include_directories (
    "${SDL2_INCLUDE_DIR}"
	"${OPENGLES2_INCLUDE_DIR}"
	"${PROJECT_SOURCE_DIR}/3rdparty/glm"
    )

set (SOURCES
    main.cpp
    )

add_definitions(-DSDL_NO_COMPAT)

add_executable (pcviewer WIN32 ${SOURCES})
target_link_libraries (pcviewer
	${SDL2_LIBRARIES}
	${OPENGLES2_LIBRARIES} ${EGL_LIBRARIES}
   	${ADDITIONAL_DEPS}
	)
