cmake_minimum_required(VERSION 2.8.12)

# Set module directory
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

# Dependency checks
find_package(Pthreads REQUIRED)
find_package(Bluez REQUIRED)

# Compilation flags
include_directories(${PTHREADS_INCLUDE_DIR} ${BLUEZ_INCLUDE_DIRS})
add_definitions(-Wall ${PTHREADS_DEFINITIONS})

# Target compilation
add_executable(blabbermouth
  bm_datastream.h bm_datastream.c
  bm_tcp_datastream.h bm_tcp_datastream.c
  bm_bt_datastream.h bm_bt_datastream.c
  bm_dispatcher.h bm_dispatcher.c
  main.c)
target_link_libraries(blabbermouth
  ${PTHREADS_LIBRARY}
  ${BLUEZ_LIBRARIES})
