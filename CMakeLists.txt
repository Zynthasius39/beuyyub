cmake_minimum_required(VERSION 3.22)

project(beuyyub VERSION 1.1.1 DESCRIPTION "Official IT1162i Discord bot")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/youtubeapi.cpp
)

find_package(DPP REQUIRED)
find_package(CURL REQUIRED)
find_package(MPG123 REQUIRED)

target_link_libraries(${PROJECT_NAME}
    ${DPP_LIBRARIES}
    ${CURL_LIBRARIES}
    ${MPG123_LIBRARIES}
    ogg
    jsoncpp
    "/usr/lib/libopus.so"
    "/usr/lib/libopusfile.so"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${DPP_INCLUDE_DIR}
    ${MPG123_INCLUDE_DIR}
    "/usr/include/opus"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDPP_CORO=ON")