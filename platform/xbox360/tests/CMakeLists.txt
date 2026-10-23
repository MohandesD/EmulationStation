cmake_minimum_required(VERSION 3.20)
project(es_xbox360_host_tests C CXX)
set(CMAKE_CXX_STANDARD 17)
enable_testing()
add_executable(test_es_gl ../es_gl.c test_gl.c)
target_include_directories(test_es_gl PRIVATE ..)
target_compile_options(test_es_gl PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_libraries(test_es_gl PRIVATE m)
option(ES_TEST_SANITIZERS "Use address and undefined behaviour sanitizers" ON)
if(ES_TEST_SANITIZERS AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(test_es_gl PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(test_es_gl PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME es_gl_semantics COMMAND test_es_gl)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME es_gl_source_coverage COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_gl_coverage.py)
add_test(NAME es_platform_catalog COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_catalog.py)

add_executable(test_es_launch test_launch.cpp ../LaunchCommand.cpp ../LaunchFiles.cpp)
target_include_directories(test_es_launch PRIVATE ..)
target_compile_options(test_es_launch PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME es_launch_paths COMMAND test_es_launch)
add_executable(test_es_handoff test_handoff.cpp ../platform_xsdk.cpp ../LaunchCommand.cpp)
target_include_directories(test_es_handoff PRIVATE stubs .. ../../../es-core/src)
target_compile_options(test_es_handoff PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME es_native_handoff COMMAND test_es_handoff)
add_executable(test_es_routes test_routes.cpp ../XboxRoutes.cpp ../LaunchCommand.cpp)
target_include_directories(test_es_routes PRIVATE ..)
target_compile_options(test_es_routes PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME es_xbox_routes COMMAND test_es_routes)
add_executable(test_es_salvia test_salvia.cpp ../SalviaLaunch.cpp ../LaunchCommand.cpp)
target_include_directories(test_es_salvia PRIVATE ..)
target_compile_options(test_es_salvia PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME es_salvia_context COMMAND test_es_salvia)
add_executable(test_es_return test_return.cpp ../SessionReturn.cpp)
target_include_directories(test_es_return PRIVATE ..)
target_compile_options(test_es_return PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME es_session_return COMMAND test_es_return)

# Optional actual-model integration harness. Only headers from the pinned native
# dependencies are needed; no SDL/video/device runtime is linked or contacted.
set(ES_MODEL_DEPS "" CACHE PATH "Pinned es-deps directory; enables production model filesystem tests")
if(ES_MODEL_DEPS)
    add_executable(test_es_model_routes test_model_routes.cpp
        ../XboxRoutes.cpp ../LaunchCommand.cpp
        ../../../es-app/src/SystemData.cpp ../../../es-app/src/Gamelist.cpp
        ../../../es-app/src/FileData.cpp ../../../es-app/src/FileSorts.cpp
        ../../../es-app/src/MetaData.cpp ../../../es-app/src/PlatformId.cpp
        ../../../es-app/src/MameNameMap.cpp
        ../../../es-core/src/Settings.cpp ../../../es-core/src/Util.cpp
        ../../../es-core/src/Log.cpp ../../../external/pugixml/src/pugixml.cpp)
    target_include_directories(test_es_model_routes PRIVATE .. ../../../es-app/src
        ../../../es-core/src ../../../external ../../../external/pugixml
        "${ES_MODEL_DEPS}/boost" "${ES_MODEL_DEPS}/eigen"
        "${ES_MODEL_DEPS}/SDL/include" "${ES_MODEL_DEPS}/freetype/include")
    target_compile_definitions(test_es_model_routes PRIVATE __XSDK__ ES_USE_SDL3
        EIGEN_DONT_ALIGN EIGEN_DONT_VECTORIZE BOOST_DATE_TIME_NO_LIB)
    target_compile_options(test_es_model_routes PRIVATE -UNDEBUG -ffunction-sections -fdata-sections)
    target_link_options(test_es_model_routes PRIVATE -Wl,--gc-sections)
    add_test(NAME es_routed_model COMMAND test_es_model_routes)
    # Invoked explicitly by test_distribution.py with a freshly extracted ZIP.
    add_executable(test_es_distribution test_distribution_model.cpp
        ../XboxRoutes.cpp ../LaunchCommand.cpp ../LaunchFiles.cpp ../SalviaLaunch.cpp
        ../../../es-app/src/SystemData.cpp ../../../es-app/src/Gamelist.cpp
        ../../../es-app/src/FileData.cpp ../../../es-app/src/FileSorts.cpp
        ../../../es-app/src/MetaData.cpp ../../../es-app/src/PlatformId.cpp
        ../../../es-app/src/MameNameMap.cpp ../../../es-core/src/ThemeData.cpp
        ../../../es-core/src/Settings.cpp ../../../es-core/src/Util.cpp
        ../../../es-core/src/Log.cpp ../../../external/pugixml/src/pugixml.cpp)
    target_include_directories(test_es_distribution PRIVATE .. ../../../es-app/src
        ../../../es-core/src ../../../external ../../../external/pugixml
        "${ES_MODEL_DEPS}/boost" "${ES_MODEL_DEPS}/eigen"
        "${ES_MODEL_DEPS}/SDL/include" "${ES_MODEL_DEPS}/freetype/include")
    target_compile_definitions(test_es_distribution PRIVATE __XSDK__ ES_USE_SDL3
        EIGEN_DONT_ALIGN EIGEN_DONT_VECTORIZE EIGEN_MPL2_ONLY BOOST_DATE_TIME_NO_LIB)
    target_compile_options(test_es_distribution PRIVATE -UNDEBUG -ffunction-sections -fdata-sections)
    target_link_options(test_es_distribution PRIVATE -Wl,--gc-sections)
endif()

set(ES_STB "" CACHE PATH "Pinned stb source tree; enables image decoder/resize tests")
if(ES_STB)
    add_executable(test_es_images test_images.cpp ../ImageIO_stb.cpp ../ImageResize_stb.cpp ../../../es-core/src/Log.cpp)
    target_include_directories(test_es_images PRIVATE "${ES_STB}" ../../../es-core/src)
    target_compile_options(test_es_images PRIVATE -UNDEBUG)
    add_test(NAME es_image_contract COMMAND test_es_images)
endif()
foreach(target test_es_launch test_es_handoff test_es_routes test_es_salvia test_es_images test_es_return test_es_model_routes test_es_distribution)
    if(TARGET ${target} AND ES_TEST_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endforeach()
