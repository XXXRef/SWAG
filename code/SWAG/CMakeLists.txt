# CMakeList.txt : CMake project for SWAG, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

#set (HEADERS "swag.hpp" "_threadsafe_queue.hpp" "threadsafe_outputter.hpp" "sha1.hpp" "threadsafe_queue.hpp")
include_directories(./)

set (SOURCES "main.cpp" "swag.cpp" "threadsafe_outputter.cpp")

if(WIN32)
    set(PLATFORM_ID PLATFORM_WINDOWS)
elseif(UNIX)
    set(PLATFORM_ID PLATFORM_NIX)

    #pthread
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    
elseif(APPLE)
    set(PLATFORM_ID PLATFORM_MACOS)
endif()

add_definitions(-D${PLATFORM_ID})

# Add source to this project's executable.
add_executable (SWAG ${SOURCES} ${HEADERS})

if(UNIX)
    target_link_libraries(SWAG PRIVATE Threads::Threads)
endif()
