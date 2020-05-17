cmake_minimum_required(VERSION 3.9)
if(WIN32)
 #set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
 set(CMAKE_TOOLCHAIN_FILE "C:/Users/liz3/Desktop/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()
cmake_policy(SET CMP0042 NEW)
set (CMAKE_CXX_STANDARD 17)
include(${CMAKE_ROOT}/Modules/ExternalProject.cmake)
project (dis_light)
message(INFO ${CMAKE_BINARY_DIR})
include_directories(${CMAKE_JS_INC} /usr/local/include/ /usr/local/Cellar/libsodium/1.0.18_1/include /usr/local/opt/mad/include /usr/local/Cellar/opusfile/0.11_1/include /usr/local/Cellar/libogg/1.3.4/include /usr/local/include/opus /usr/include/opus)
link_directories(/usr/local/lib/ /usr/local/Cellar/libsodium/1.0.18_1/lib /usr/local/opt/mad/lib /usr/local/Cellar/opusfile/0.11_1/lib)
file(GLOB SOURCE_FILES "lib/bindings.cc")
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC} 
    lib/instance.cpp 
    lib/instance.h 
    lib/socket/http/DisHttpClient.h 
    lib/socket/http/DisHttpClient.cpp 
    lib/socket/discord_utils.h
    lib/socket/dis_websocket.cpp 
    lib/socket/dis_websocket.h 
    lib/zstr/zstr.h
    lib/zstr/strict_fstream.h 
    lib/socket/dis_voice_websocket.h 
    lib/socket/dis_voice_websocket.cpp 
    lib/voice/opus/opus_wrapper.h
    lib/voice/opus/opus_wrapper.cc
    lib/voice/voice_connection.h 
    lib/voice/voice_connection.cpp 
    lib/voice/win_mmap/windows-mmap.h
    lib/voice/win_mmap/windows-mmap.c
    lib/voice/stringutils.h)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
if(WIN32)
    find_package(nlohmann_json CONFIG REQUIRED)
    find_package(Opus CONFIG REQUIRED)
    find_library(MAD_LIBRARY mad)
    find_library(OGG_LIBRARY ogg)
    find_library(MBEDCRYPTO_LIBRARY mbedcrypto)
    find_library(MBEDTLS_LIBRARY mbedtls)
    find_library(MBEDX_LIBRARY mbedx509)
    find_path(IXWEBSOCKET_INCLUDE_DIR ixwebsocket/IXWebSocket.h)
    find_library(IXWEBSOCKET_LIBRARY ixwebsocket)
    include_directories(${IXWEBSOCKET_INCLUDE_DIR})
    find_path(OPUSFILE_INCLUDE_DIR opus/opusfile.h)
    find_library(OPUSFILE_LIBRARY opusfile)
    include_directories(${OPUSFILE_INCLUDE_DIR})
    find_package(unofficial-sodium CONFIG REQUIRED)
    message(${MBEDTLS_LIBRARY})
    target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json nlohmann_json::nlohmann_json unofficial-sodium::sodium Opus::opus ws2_32 ${MBEDX_LIBRARY} ${MBEDTLS_LIBRARY} ${MBEDCRYPTO_LIBRARY} ${IXWEBSOCKET_LIBRARY} ${OGG_LIBRARY} ${OPUSFILE_LIBRARY} ${MAD_LIBRARY} ${CMAKE_JS_LIB})
else()
  find_package(nlohmann_json 3.2.0 REQUIRED)
  target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})
  if(APPLE)
      target_link_libraries(${PROJECT_NAME} PRIVATE opus IXWebSocket nlohmann_json::nlohmann_json sodium mad m opusfile)
  else ()
      target_link_libraries(${PROJECT_NAME} PRIVATE opus ixwebsocket nlohmann_json::nlohmann_json sodium mad m opusfile)
   endif()
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
