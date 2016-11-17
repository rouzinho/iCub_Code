cmake_minimum_required(VERSION 3.0)
set(PROJECTNAME Era)

project(${PROJECTNAME})
set(QT_USE_QTNETWORK true)

find_package(Qt4 COMPONENTS QtCore QtGui QtNetwork)
find_package(YARP REQUIRED)
find_package(ImageMagick COMPONENTS Magick++)

INCLUDE(${QT_USE_FILE})

include_directories(${QT_INCLUDES}
		    ${YARP_INCLUDE_DIRS})


add_executable(
	Era
	src/Node.hpp
	src/Node.cpp
	src/som.hpp
	src/som.cpp
	src/Era.h
	src/Era.cpp
	src/main.cpp
)

target_link_libraries(${PROJECTNAME} ${QT_LIBRARIES} ${YARP_LIBRARIES} ${ImageMagick_LIBRARIES} )
