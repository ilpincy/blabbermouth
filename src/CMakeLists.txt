cmake_minimum_required(VERSION 2.8.12)

# Set module directory
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

# Dependency checks
find_package(Pthreads REQUIRED)
add_definitions(${PTHREADS_DEFINITIONS})
include_directories(${PTHREADS_INCLUDE_DIR})
find_package(Bluez)
if(BLUEZ_FOUND)
  include_directories(${BLUEZ_INCLUDE_DIRS})
endif(BLUEZ_FOUND)

# Compilation flags
add_definitions(-Wall)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Source files
set(SOURCES
  bm_datastream.h bm_datastream.c
  bm_tcp_datastream.h bm_tcp_datastream.c
  bm_udp_datastream.h bm_udp_datastream.c
  bm_dispatcher.h bm_dispatcher.c
  main.c)
if(BLUEZ_FOUND)
  set(SOURCES ${SOURCES}
    bm_bt_datastream.h bm_bt_datastream.c)
endif(BLUEZ_FOUND)

# Generate config.h file
configure_file(config.h.in config.h @ONLY)

# Target compilation
add_executable(blabbermouth ${SOURCES})
target_link_libraries(blabbermouth ${PTHREADS_LIBRARY})
if(BLUEZ_FOUND)
target_link_libraries(blabbermouth ${BLUEZ_LIBRARIES})
endif(BLUEZ_FOUND)
