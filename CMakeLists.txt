cmake_minimum_required(VERSION 3.21)

project(DAOPhotoCam VERSION 0.1.0 LANGUAGES CXX)

option(PHOTOCAM_BUILD_TESTS "Build the unit tests (no game required)" ON)
option(PHOTOCAM_SANITIZE "Build with AddressSanitizer (for the tests)" OFF)

# Dragon Age: Origins is a 32-bit x86 executable, so the plugin the loader
# loads into it must be too. CMakePresets.json selects the x86 toolset and
# the x86-windows-static-md vcpkg triplet; this catches a configure from a
# 64-bit shell with a sentence rather than a DLL the game cannot load.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(
        FATAL_ERROR
        "photocam.dll must be built for 32-bit x86 (the game's architecture); "
        "configure with a preset from CMakePresets.json, through tools/build.ps1"
    )
endif()

# AddressSanitizer covers every translation unit or none, so it is global
# and set before any target, DAOLib's among them. Only the tests run under
# it; a sanitized DLL in the game would need the ASan runtime beside the exe.
# vcpkg's libraries are built without the STL's container annotations, so
# they are off (DAOLib's CMakeLists has the LNK2038 this avoids).
if(PHOTOCAM_SANITIZE)
    add_compile_options(/fsanitize=address /Oy-)
    add_compile_definitions(
        _DISABLE_STRING_ANNOTATION
        _DISABLE_VECTOR_ANNOTATION
    )
    add_link_options(/INCREMENTAL:NO)
    string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()

# ---------------------------------------------------------------------------
# DAOLib, built with us the way Skyrim plugins build CommonLibSSE: a source
# tree added as a subdirectory, which gives DAOLib::daolib, add_daolib_plugin
# and dao_apply_warnings. Its dependencies (detours, spdlog) come through our
# vcpkg.json, since a manifest build reads only the top-level manifest.
#
# The tree is the one DAOLIB_DIR names if it names one, else extern/DAOLib,
# a git submodule, else the sibling checkout ..\DAOLib.
# ---------------------------------------------------------------------------
set(DAOLIB_DIR
    ""
    CACHE PATH
    "DAOLib's source tree (default extern/DAOLib, else ../DAOLib)"
)
if(DAOLIB_DIR)
    set(PHOTOCAM_DAOLIB "${DAOLIB_DIR}")
elseif(DEFINED ENV{DAOLIB_DIR})
    set(PHOTOCAM_DAOLIB "$ENV{DAOLIB_DIR}")
elseif(EXISTS "${PROJECT_SOURCE_DIR}/extern/DAOLib/CMakeLists.txt")
    set(PHOTOCAM_DAOLIB "${PROJECT_SOURCE_DIR}/extern/DAOLib")
else()
    set(PHOTOCAM_DAOLIB "${PROJECT_SOURCE_DIR}/../DAOLib")
endif()
if(NOT EXISTS "${PHOTOCAM_DAOLIB}/cmake/DAOLib.cmake")
    message(
        FATAL_ERROR
        "No DAOLib with add_daolib_plugin at ${PHOTOCAM_DAOLIB}. Add the submodule "
        "(git submodule update --init --recursive) or set DAOLIB_DIR."
    )
endif()
message(STATUS "DAOLib: ${PHOTOCAM_DAOLIB}")
add_subdirectory(
    "${PHOTOCAM_DAOLIB}"
    "${CMAKE_BINARY_DIR}/extern/DAOLib"
    EXCLUDE_FROM_ALL
)

# The format and lint targets, DAOLib's own module run over this tree: it
# reads the top-level source directory, our .clang-format and .clang-tidy,
# and tools/check-powershell.ps1.
include("${PHOTOCAM_DAOLIB}/cmake/Quality.cmake")

# ---------------------------------------------------------------------------
# photocam_core -- the flight, when photo cam may start, and its tools'
# JSON for Eluvian. It never
# includes <Windows.h>, never links DAOLib and never sees a dao:: type, which
# is what makes it testable without the game.
# ---------------------------------------------------------------------------
find_package(nlohmann_json CONFIG REQUIRED)
add_library(
    photocam_core
    STATIC
    src/core/FlyCamera.cpp
    src/core/Session.cpp
    src/core/Tools.cpp
)
target_include_directories(photocam_core PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_compile_features(photocam_core PUBLIC cxx_std_20)
target_link_libraries(photocam_core PUBLIC nlohmann_json::nlohmann_json)
dao_apply_warnings(photocam_core)

# ---------------------------------------------------------------------------
# photocam.dll -- the plugin, a folder of its own:
# build\<preset>\daolib\plugins\photocam\ has the dll and its pdb, which
# tools/deploy.ps1 copies as it is.
# ---------------------------------------------------------------------------
add_daolib_plugin(
    photocam
    SOURCES
        src/plugin/PhotoCam.cpp
        src/plugin/GameView.cpp
        src/plugin/Eluvian.cpp
        src/plugin/Panel.cpp
    # The panel's movie, built outside CMake by tools/movie.py (it needs Java and FFDec), served
    # to the game from beside the dll (dao/Gui.h). One not built yet warns and is skipped.
    FILES build/ui/photocam.swf build/ui/photocam_navbar.swf
)
target_link_libraries(photocam PRIVATE photocam_core)
dao_apply_warnings(photocam)

if(PHOTOCAM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
