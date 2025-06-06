cmake_minimum_required(VERSION 3.19)

project(3DGraphicsRendering)

include(CMake/CommonMacros.txt)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(DEPS_CACHE_FILE "${ROOT_DIR}/deps/.bootstrap.json")

if($ENV{CLION_IDE})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_DIR})
endif()

find_package(Python3 COMPONENTS Interpreter)

execute_process(COMMAND ${Python3_EXECUTABLE} deploy_deps.py WORKING_DIRECTORY ${ROOT_DIR} COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${Python3_EXECUTABLE} deploy_deps.py WORKING_DIRECTORY ${ROOT_DIR}/deps/src/lightweightvk COMMAND_ERROR_IS_FATAL ANY)

add_custom_command(COMMAND ${Python3_EXECUTABLE} deploy_deps.py OUTPUT ${DEPS_CACHE_FILE}
                   DEPENDS "${ROOT_DIR}/deps/bootstrap.json" WORKING_DIRECTORY ${ROOT_DIR})
add_custom_target(Dependencies DEPENDS ${DEPS_CACHE_FILE})
set_property(TARGET Dependencies PROPERTY FOLDER "third-party")

if(NOT EXISTS ${DEPS_CACHE_FILE})
  message(FATAL_ERROR "Cannot bootstrap dependencies.")
endif()

# cmake-format: off
add_subdirectory(deps/src/lightweightvk/third-party/deps/src/ldrutils)
set(KTX_FEATURE_DOC            OFF CACHE BOOL "")
set(KTX_FEATURE_GL_UPLOAD      OFF CACHE BOOL "")
set(KTX_FEATURE_JNI            OFF CACHE BOOL "")
set(KTX_FEATURE_KTX1            ON CACHE BOOL "")
set(KTX_FEATURE_KTX2            ON CACHE BOOL "")
set(KTX_FEATURE_LOADTEST_APPS  OFF CACHE BOOL "")
set(KTX_FEATURE_STATIC_LIBRARY  ON CACHE BOOL "")
set(KTX_FEATURE_TESTS          OFF CACHE BOOL "")
set(KTX_FEATURE_TOOLS          OFF CACHE BOOL "")
set(KTX_FEATURE_VK_UPLOAD      OFF CACHE BOOL "")
add_subdirectory(deps/src/lightweightvk/third-party/deps/src/ktx-software)
set(LVK_WITH_SAMPLES   OFF CACHE BOOL "")
set(LVK_WITH_TRACY     ON  CACHE BOOL "")
set(LVK_WITH_TRACY_GPU ON  CACHE BOOL "")
add_subdirectory(deps/src/lightweightvk)
lvk_set_folder(ktx "third-party/ktx-software")
lvk_set_folder(ktx_read "third-party/ktx-software")
lvk_set_folder(ktx_version "third-party/ktx-software")
lvk_set_folder(obj_basisu_cbind "third-party/ktx-software")
lvk_set_folder(objUtil "third-party/ktx-software")
if(TARGET astcenc-avx2-static)
  lvk_set_folder(astcenc-avx2-static "third-party/ktx-software")
endif()
# cmake-format: on

set(ASSIMP_NO_EXPORT ON CACHE BOOL "")
set(ASSIMP_BUILD_DRACO OFF CACHE BOOL "")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "")
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "")
set(ASSIMP_INSTALL_PDB OFF CACHE BOOL "")
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "")
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "")
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "")

add_subdirectory(deps/src/assimp)
if(APPLE)
  target_compile_options(assimp PRIVATE -Wno-deprecated-declarations)
endif()

set(MESHOPT_BUILD_DEMO OFF CACHE BOOL "")
set(MESHOPT_BUILD_TOOLS OFF CACHE BOOL "")
set(MESHOPT_BUILD_SHARED_LIBS OFF CACHE BOOL "")
add_subdirectory(deps/src/lightweightvk/third-party/deps/src/meshoptimizer)

if(WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

include_directories(.)
include_directories(deps/src)
include_directories(deps/src/lightweightvk/third-party/deps/src/taskflow)
include_directories(deps/src/lightweightvk/third-party/deps/src/glm)
include_directories(deps/src/lightweightvk/third-party/deps/src/imgui)
add_subdirectory(deps/cmake/ImGuizmo)

set_property(TARGET assimp          PROPERTY FOLDER "third-party")
set_property(TARGET LUtils          PROPERTY FOLDER "third-party")
set_property(TARGET meshoptimizer   PROPERTY FOLDER "third-party")
if(WIN32)
  if(TARGET zlibstatic)
    set_property(TARGET zlibstatic  PROPERTY FOLDER "third-party")
  endif()
  if(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs)
    set_property(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTY FOLDER "third-party")
  endif()
endif()
set_property(TARGET uninstall       PROPERTY FOLDER "third-party")

# disable warnings in third-party code
if(MSVC)
	target_compile_options(assimp PRIVATE /wd4267)
	target_compile_options(SPIRV  PRIVATE /wd4267)
endif()

# lvk tracy
if(LVK_WITH_TRACY)
  target_compile_definitions(LVKVulkan PUBLIC "LVK_WITH_TRACY=1")
  if(LVK_WITH_TRACY_GPU)
    target_compile_definitions(LVKVulkan PUBLIC "LVK_WITH_TRACY_GPU=1")
    target_include_directories(LVKVulkan PUBLIC "${ROOT_DIR}/deps/src/lightweightvk/third-party/deps/src/tracy/public")
  endif()
endif()

SETUP_APP(3DGraphicsRendering)

target_link_libraries(3DGraphicsRendering assimp)
target_link_libraries(3DGraphicsRendering glslang)
target_link_libraries(3DGraphicsRendering glfw)
target_link_libraries(3DGraphicsRendering LUtils)
target_link_libraries(3DGraphicsRendering LVKLibrary)
target_link_libraries(3DGraphicsRendering LVKstb)
target_link_libraries(3DGraphicsRendering ktx)
target_link_libraries(3DGraphicsRendering meshoptimizer)

target_sources(3DGraphicsRendering PRIVATE "${ROOT_DIR}/deps/src/lightweightvk/third-party/deps/src/imgui/imgui_demo.cpp")
target_sources(3DGraphicsRendering PRIVATE "${ROOT_DIR}/deps/src/lightweightvk/third-party/deps/src/implot/implot_demo.cpp")

add_dependencies(3DGraphicsRendering Dependencies)