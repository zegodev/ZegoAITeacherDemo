cmake_minimum_required(VERSION 3.5.2)
project (zegoliveroomdemo)

include_directories("./include")

link_directories("./libs")

link_libraries(zegoliveroom rt pthread)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -g")

add_compile_options(
    -std=c++11
    )

set (SRC_LIST main.cpp CallbackImpl.cpp)

message (STATUS "ENABLE EXTERNAL CAPTURE FEATURE")
list (APPEND SRC_LIST media_publisher_event_callback.cpp)

if (USE_EXTERNAL_RENDER)
    message (STATUS "ENABLE EXTERNAL RENDER FEATURE")
    add_definitions(-DENABLE_EXTERNAL_RENDER_FEATURE)
    list (APPEND SRC_LIST ExternalRender.cpp)
endif()

if (USE_OFFICE_APP_KEY)
    add_definitions(-DUSE_OFFICE_APP_KEY)
endif()

if (ENABLE_MEDIA_SIDE_INFO_FEATURE)
    add_definitions(-DENABLE_MEDIA_SIDE_INFO_FEATURE)
endif()

add_executable(aidemo ${SRC_LIST})
