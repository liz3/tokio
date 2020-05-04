cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0042 NEW)
set (CMAKE_CXX_STANDARD 17)
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
project (dis_light)
message(INFO ${CMAKE_BINARY_DIR})
include_directories(${CMAKE_JS_INC} /usr/local/include/ /usr/local/Cellar/libsodium/1.0.18_1/include /usr/local/opt/mad/include /usr/local/Cellar/opusfile/0.11_1/include /usr/local/Cellar/libogg/1.3.4/include /usr/local/include/opus /usr/include/opus)
link_directories(/usr/local/lib/ /usr/local/Cellar/libsodium/1.0.18_1/lib /usr/local/opt/mad/lib /usr/local/Cellar/opusfile/0.11_1/lib)
file(GLOB SOURCE_FILES "lib/bindings.cc")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC} lib/instance.cpp lib/instance.h lib/socket/utils.h lib/socket/discord_utils.h lib/socket/dis_websocket.cpp lib/socket/dis_websocket.h lib/zstr/zstr.h lib/zstr/strict_fstream.h lib/socket/dis_voice_websocket.h lib/socket/dis_voice_websocket.cpp lib/voice/opus/opus_wrapper.h lib/voice/opus/opus_wrapper.cc lib/voice/voice_connection.h lib/voice/voice_connection.cpp lib/voice/stringutils.h)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
find_package(nlohmann_json 3.2.0 REQUIRED)
if(APPLE)
target_link_libraries(${PROJECT_NAME} PRIVATE opus IXWebSocket nlohmann_json::nlohmann_json sodium mad m opusfile)
else ()
target_link_libraries(${PROJECT_NAME} PRIVATE opus ixwebsocket nlohmann_json::nlohmann_json sodium mad m opusfile)
endif()
# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

# Define NPI_VERSION
add_definitions(-DNAPI_VERSION=4)
