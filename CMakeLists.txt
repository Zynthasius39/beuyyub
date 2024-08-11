cmake_minimum_required(VERSION 3.22)

project(beuyyub VERSION 1.2.0 DESCRIPTION "Official IT1162i Discord bot")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/youtubeapi.cpp
)

find_package(DPP REQUIRED)
find_package(CURL REQUIRED)
find_package(MPG123 REQUIRED)
find_package(Opus REQUIRED)
find_package(JsonCpp REQUIRED)

target_link_libraries(${PROJECT_NAME}
    ${DPP_LIBRARIES}
    ${CURL_LIBRARIES}
    ${MPG123_LIBRARIES}
    ${OGG_LIBRARIES}
    ${OPUS_LIBRARIES}
    ${JSONCPP_LIBRARY}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${DPP_INCLUDE_DIRS}
    ${MPG123_INCLUDE_DIRS}
    ${OGG_INCLUDE_DIRS}
    ${OPUS_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDPP_CORO=ON")
