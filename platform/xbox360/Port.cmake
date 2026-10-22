# Included by the top-level project only for the xsdk toolchain.
set(ES_DEPS "$ENV{HOME}/xbox-build/es-deps" CACHE PATH "Pinned third-party source checkouts")
set(ES_BOOST_HEADERS "${ES_DEPS}/boost" CACHE PATH "Directory containing header-only Boost 1.83 boost/")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_definitions(ES_USE_SDL3 EIGEN_DONT_ALIGN EIGEN_DONT_VECTORIZE BOOST_DATE_TIME_NO_LIB)

set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory("${ES_DEPS}/SDL" dependencies/SDL EXCLUDE_FROM_ALL)

set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
add_subdirectory("${ES_DEPS}/freetype" dependencies/freetype EXCLUDE_FROM_ALL)

set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
add_subdirectory("${ES_DEPS}/mbedtls" dependencies/mbedtls EXCLUDE_FROM_ALL)
set(MBEDTLS_INCLUDE_DIR "${ES_DEPS}/mbedtls/include" CACHE PATH "")
set(MBEDTLS_LIBRARY mbedtls CACHE STRING "")
set(MBEDX509_LIBRARY mbedx509 CACHE STRING "")
set(MBEDCRYPTO_LIBRARY mbedcrypto CACHE STRING "")
set(CURL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory("${ES_DEPS}/curl" dependencies/curl EXCLUDE_FROM_ALL)

set(COMMON_INCLUDE_DIRS
    "${CMAKE_SOURCE_DIR}/external" "${CMAKE_SOURCE_DIR}/es-core/src"
    "${CMAKE_SOURCE_DIR}/platform/xbox360" "${ES_DEPS}/eigen"
    "${ES_DEPS}/stb" "${ES_BOOST_HEADERS}")
set(COMMON_LIBRARIES SDL3::SDL3-static freetype CURL::libcurl mbedtls pugixml nanosvg)
add_subdirectory(external)
add_subdirectory(es-core)
add_subdirectory(es-app)
target_sources(emulationstation PRIVATE "${CMAKE_SOURCE_DIR}/platform/xbox360/FrontendState.cpp")

target_sources(es-core PRIVATE
    "${CMAKE_SOURCE_DIR}/platform/xbox360/Renderer_init_xsdk.cpp"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/SDLCompat.cpp"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/AudioManager_sdl3.cpp"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/ImageIO_stb.cpp"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/ImageResize_stb.cpp"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/platform_xsdk.cpp"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/LaunchCommand.cpp"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/SalviaLaunch.cpp"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/SessionReturn.cpp"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/SessionReturn_xsdk.cpp"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/SessionReturn_frontend.cpp"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/LaunchFiles.cpp"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/es_gl.c"
    "${CMAKE_SOURCE_DIR}/platform/xbox360/es_gpu.c")

set(ES_SHADER_HEADER "${CMAKE_BINARY_DIR}/es_shaders.h")
find_program(ES_GLSLANG NAMES glslangValidator glslang REQUIRED)
add_custom_command(OUTPUT "${ES_SHADER_HEADER}"
    COMMAND python3 "${XSDK_ROOT}/tools/build_shaders.py" --glslang "${ES_GLSLANG}"
        --output "${ES_SHADER_HEADER}" "${CMAKE_SOURCE_DIR}/platform/xbox360/es.vert" "${CMAKE_SOURCE_DIR}/platform/xbox360/es.frag"
    COMMAND python3 "${CMAKE_SOURCE_DIR}/platform/xbox360/tests/check_shaders.py" "${ES_SHADER_HEADER}"
    DEPENDS platform/xbox360/es.vert platform/xbox360/es.frag platform/xbox360/tests/check_shaders.py
        "${XSDK_ROOT}/build/host/libxsc.so" VERBATIM)
add_custom_target(es_xbox_shaders DEPENDS "${ES_SHADER_HEADER}")
add_dependencies(es-core es_xbox_shaders)
target_include_directories(es-core PRIVATE "${CMAKE_BINARY_DIR}")
configure_file("${ES_DEPS}/mbedtls/programs/xsdk/cacert.pem" "${CMAKE_BINARY_DIR}/cacert.pem" COPYONLY)
