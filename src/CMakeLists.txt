cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 14)

project(X11_Clippy)

set(CMAKE_BUILD_TYPE Debug)


add_executable(xtargets xtargets.cpp)
add_executable(xwriter xwriter.cpp)
add_executable(xreader xreader.cpp)
add_executable(clippy clippy.cpp clippyutils.cpp)

target_link_libraries(xtargets -lX11)
target_link_libraries(xwriter -lX11)
target_link_libraries(xreader -lX11)

target_include_directories(clippy PRIVATE /usr/include/gdk-pixbuf-2.0 /usr/include/glib-2.0 /usr/lib/glib-2.0/include /usr/include/libmount /usr/include/blkid)

# rdynamic required for backtrace
target_link_libraries(clippy -lX11 -rdynamic -pthread -lnotify -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0)