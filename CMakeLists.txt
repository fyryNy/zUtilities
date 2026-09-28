cmake_minimum_required(VERSION 3.25)

cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0141 NEW)

project(zUtilities LANGUAGES CXX RC)

set(UNION_SDK_ROOT "$ENV{HOME}/my_msvc/opt/Union/1.0m" CACHE PATH
    "Root containing Union/v1.0m and DirectX7")
set(UNION_GAME "MP_X4" CACHE STRING
    "Union target: G1, G1A, G2, G2A, MP_X2, or MP_X4")
set_property(CACHE UNION_GAME PROPERTY STRINGS G1 G1A G2 G2A MP_X2 MP_X4)
set(UNION_RUNTIME "MT" CACHE STRING "MSVC runtime: MT or MD")
set_property(CACHE UNION_RUNTIME PROPERTY STRINGS MT MD)
set(UNION_EXTRA_LIBRARIES "" CACHE STRING
    "Extra Windows libraries required by this plugin")
set(UNION_EXTRA_INCLUDE_DIRECTORIES "" CACHE STRING
    "Extra include directories required by this plugin")
set(UNION_EXTRA_LIBRARY_DIRECTORIES "" CACHE STRING
    "Extra library directories required by this plugin")

set(_plugin_dir "${CMAKE_CURRENT_SOURCE_DIR}/zUtilities")
set(_vcxproj "${_plugin_dir}/zUtilities.vcxproj")
set(_union_include_source "${UNION_SDK_ROOT}/Union/v1.0m/Include")
set(_union_lib "${UNION_SDK_ROOT}/Union/v1.0m/Lib")
set(_directx_include "${UNION_SDK_ROOT}/DirectX7/include")
set(_directx_lib "${UNION_SDK_ROOT}/DirectX7/lib")
set(_union_include "${_union_include_source}")

foreach(_required
        "${_vcxproj}"
        "${_union_include_source}/Memory/Shw32.h"
        "${_union_lib}/Shw32.lib"
        "${_union_lib}/Vdfs32g.lib"
        "${_directx_include}/d3d.h")
    if(NOT EXISTS "${_required}")
        message(FATAL_ERROR "Required file not found: ${_required}")
    endif()
endforeach()

# Remove SDK copies produced by older versions. The installed Union headers and
# the corrected project-local UnionAfx/ZenGin template are used directly.
file(REMOVE_RECURSE
    "${CMAKE_BINARY_DIR}/union-sdk-include"
    "${CMAKE_BINARY_DIR}/plugin-sdk-overlay")

# The old template keeps its real build inputs in the vcxproj. Read them here,
# so this file also follows project-specific .cpp and .rc changes automatically.
file(READ "${_vcxproj}" _vcxproj_xml)
string(REGEX MATCHALL "<ClCompile[ \t\r\n]+Include=\"[^\"]+\"" _compile_tags
    "${_vcxproj_xml}")
string(REGEX MATCHALL "<ResourceCompile[ \t\r\n]+Include=\"[^\"]+\"" _resource_tags
    "${_vcxproj_xml}")

set(_game_definitions)
set(_enabled_engine_source_dirs)
if(UNION_GAME STREQUAL "G1")
    list(APPEND _game_definitions __G1)
    list(APPEND _enabled_engine_source_dirs "ZenGin/Gothic_I_Classic")
elseif(UNION_GAME STREQUAL "G1A")
    list(APPEND _game_definitions __G1A)
    list(APPEND _enabled_engine_source_dirs "ZenGin/Gothic_I_Addon")
elseif(UNION_GAME STREQUAL "G2")
    list(APPEND _game_definitions __G2)
    list(APPEND _enabled_engine_source_dirs "ZenGin/Gothic_II_Classic")
elseif(UNION_GAME STREQUAL "G2A")
    list(APPEND _game_definitions __G2A)
    list(APPEND _enabled_engine_source_dirs "ZenGin/Gothic_II_Addon")
elseif(UNION_GAME STREQUAL "MP_X2")
    list(APPEND _game_definitions __G1 __G2A)
    list(APPEND _enabled_engine_source_dirs
        "ZenGin/Gothic_I_Classic"
        "ZenGin/Gothic_II_Addon")
elseif(UNION_GAME STREQUAL "MP_X4")
    list(APPEND _game_definitions __G1 __G1A __G2 __G2A)
    list(APPEND _enabled_engine_source_dirs
        "ZenGin/Gothic_I_Classic"
        "ZenGin/Gothic_I_Addon"
        "ZenGin/Gothic_II_Classic"
        "ZenGin/Gothic_II_Addon")
else()
    message(FATAL_ERROR "Unsupported UNION_GAME: ${UNION_GAME}")
endif()

# cpptools cannot combine compileCommands with its forcedInclude setting and
# does not reliably honor MSVC /FI from the database. Generate only the active
# engine definitions and expose them through a stable ignored symlink consumed
# by the shipped cmake/IntelliSenseContext.h.
set(_intellisense_active_definitions
    "#pragma once\n\n// Generated from the most recently configured CMake preset.\n")
foreach(_definition IN LISTS _game_definitions)
    string(APPEND _intellisense_active_definitions
        "#ifndef ${_definition}\n#define ${_definition}\n#endif\n")
endforeach()
set(_intellisense_active_dir "${CMAKE_BINARY_DIR}/intellisense")
set(_intellisense_active_header
    "${_intellisense_active_dir}/IntelliSenseActivePreset.h")
set(_intellisense_active_link
    "${CMAKE_SOURCE_DIR}/cmake/IntelliSenseActivePreset.h")
file(MAKE_DIRECTORY "${_intellisense_active_dir}")
file(WRITE "${_intellisense_active_header}"
    "${_intellisense_active_definitions}")
if(IS_SYMLINK "${_intellisense_active_link}")
    file(REMOVE "${_intellisense_active_link}")
elseif(EXISTS "${_intellisense_active_link}")
    message(FATAL_ERROR
        "${_intellisense_active_link} exists and is not a symlink. "
        "Remove or rename it so CMake can manage the active editor context.")
endif()
file(CREATE_LINK
    "${_intellisense_active_header}"
    "${_intellisense_active_link}"
    SYMBOLIC
    RESULT _intellisense_active_link_result)
if(NOT _intellisense_active_link_result STREQUAL "0")
    message(FATAL_ERROR
        "Could not select the active IntelliSense engine context: "
        "${_intellisense_active_link_result}")
endif()

set(_project_sources)
foreach(_tag IN LISTS _compile_tags _resource_tags)
    string(REGEX REPLACE ".*Include=\"([^\"]+)\".*" "\\1" _relative_source "${_tag}")
    string(REPLACE "\\" "/" _relative_source "${_relative_source}")

    set(_source_enabled TRUE)
    if(_relative_source MATCHES "^ZenGin/Gothic_(I|II)_(Classic|Addon)/")
        set(_source_enabled FALSE)
        foreach(_engine_dir IN LISTS _enabled_engine_source_dirs)
            string(FIND "${_relative_source}" "${_engine_dir}/" _engine_dir_position)
            if(_engine_dir_position EQUAL 0)
                set(_source_enabled TRUE)
                break()
            endif()
        endforeach()
    endif()

    if(NOT _source_enabled)
        continue()
    elseif(EXISTS "${_plugin_dir}/${_relative_source}")
        list(APPEND _project_sources "${_plugin_dir}/${_relative_source}")
    else()
        message(WARNING "vcxproj input does not exist: ${_relative_source}")
    endif()
endforeach()

if(NOT _project_sources)
    message(FATAL_ERROR "No compilable inputs found in ${_vcxproj}")
endif()

add_library(zUtilities SHARED ${_project_sources})

# Preserve the language mode from the equivalent Visual Studio configuration.
if(UNION_GAME STREQUAL "MP_X2")
    set(_vs_game "MP x2")
elseif(UNION_GAME STREQUAL "MP_X4")
    set(_vs_game "MP x4")
else()
    set(_vs_game "${UNION_GAME}")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_vs_configuration "${_vs_game} Debug")
else()
    set(_vs_configuration "${_vs_game} ${UNION_RUNTIME} Release")
endif()
string(FIND "${_vcxproj_xml}" "=='${_vs_configuration}|Win32'" _config_position)
set(_configuration_xml "")
if(NOT _config_position EQUAL -1)
    string(SUBSTRING "${_vcxproj_xml}" ${_config_position} -1 _configuration_tail)
    string(FIND "${_configuration_tail}" "</ItemDefinitionGroup>" _config_end)
    if(NOT _config_end EQUAL -1)
        string(SUBSTRING "${_configuration_tail}" 0 ${_config_end} _configuration_xml)
    endif()
endif()

if(_configuration_xml MATCHES "<LanguageStandard>stdcpplatest</LanguageStandard>")
    target_compile_options(zUtilities PRIVATE /std:c++latest)
    set(_intellisense_standard_option /std:c++latest)
elseif(_configuration_xml MATCHES "<LanguageStandard>stdcpp23</LanguageStandard>")
    set_property(TARGET zUtilities PROPERTY CXX_STANDARD 23)
    set(_intellisense_cxx_standard 23)
elseif(_configuration_xml MATCHES "<LanguageStandard>stdcpp20</LanguageStandard>")
    set_property(TARGET zUtilities PROPERTY CXX_STANDARD 20)
    set(_intellisense_cxx_standard 20)
elseif(_configuration_xml MATCHES "<LanguageStandard>stdcpp17</LanguageStandard>")
    set_property(TARGET zUtilities PROPERTY CXX_STANDARD 17)
    set(_intellisense_cxx_standard 17)
endif()

set(_project_includes
    "${_plugin_dir}"
    "${_plugin_dir}/ZenGin/Gothic_UserAPI"
    "${_union_include}"
    "${_directx_include}"
    ${UNION_EXTRA_INCLUDE_DIRECTORIES})

target_include_directories(zUtilities PRIVATE ${_project_includes})
target_link_directories(zUtilities PRIVATE
    "${_union_lib}" "${_directx_lib}" ${UNION_EXTRA_LIBRARY_DIRECTORIES})

set(_common_definitions
    WIN32 _WINDOWS _USRDLL GOTHICPLUGIN_EXPORTS _WINDLL _MBCS
    ${_game_definitions})
target_compile_definitions(zUtilities PRIVATE ${_common_definitions})

if(UNION_RUNTIME STREQUAL "MT")
    set_property(TARGET zUtilities PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded")
elseif(UNION_RUNTIME STREQUAL "MD")
    # Old template Debug configurations deliberately use /MD, not /MDd.
    set_property(TARGET zUtilities PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
else()
    message(FATAL_ERROR "Unsupported UNION_RUNTIME: ${UNION_RUNTIME}")
endif()

target_compile_options(zUtilities PRIVATE
    # Union's XCALL macro restores ESP from EBP before tail-jumping into
    # ZenGin. Match the old Visual Studio release code-generation settings.
    /W3 /sdl /Oi /Oy- /GL /GS /Gy /fp:precise
    /Zc:wchar_t /Zc:forScope /Zc:inline /wd4244
    "/FI${_plugin_dir}/UnionAfx.h")
target_link_options(zUtilities PRIVATE
    /SUBSYSTEM:WINDOWS
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/OPT:ICF>
    $<$<CONFIG:Release>:/LTCG:incremental>)
target_link_libraries(zUtilities PRIVATE
    Shw32 Vdfs32g Psapi Gdiplus Ws2_32 ${UNION_EXTRA_LIBRARIES})
set_target_properties(zUtilities PROPERTIES
    PREFIX ""
    OUTPUT_NAME "zUtilities"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Source fragments are included through Sources.h and must not be linked as
# separate translation units. This excluded target only emits compile database
# entries, giving clangd/cpptools the right forced context when a fragment opens.
set(_sources_header "${_plugin_dir}/Sources.h")
set(_intellisense_fragments)
if(EXISTS "${_sources_header}")
    file(READ "${_sources_header}" _sources_text)
    string(REGEX MATCHALL "#[ \t]*include[ \t]*\"[^\"]+\\.cpp\"" _fragment_tags
        "${_sources_text}")
    foreach(_tag IN LISTS _fragment_tags)
        string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" _fragment "${_tag}")
        string(REPLACE "\\" "/" _fragment "${_fragment}")
        if(EXISTS "${_plugin_dir}/${_fragment}")
            list(APPEND _intellisense_fragments "${_plugin_dir}/${_fragment}")
        endif()
    endforeach()
endif()

if(_intellisense_fragments)
    add_library(zUtilities_IntelliSense OBJECT EXCLUDE_FROM_ALL
        ${_intellisense_fragments})
    target_include_directories(zUtilities_IntelliSense PRIVATE ${_project_includes})
    target_compile_definitions(zUtilities_IntelliSense PRIVATE ${_common_definitions})
    target_compile_options(zUtilities_IntelliSense PRIVATE
        /W3 ${_intellisense_standard_option}
        "/FI${CMAKE_CURRENT_SOURCE_DIR}/cmake/IntelliSenseContext.h")
    if(_intellisense_cxx_standard)
        set_property(TARGET zUtilities_IntelliSense PROPERTY
            CXX_STANDARD ${_intellisense_cxx_standard})
    endif()
    set_property(TARGET zUtilities_IntelliSense PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

# Make clangd and cpptools follow whichever real preset was configured most
# recently instead of hard-coding one game/runtime/build configuration.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_EXPORT_COMPILE_COMMANDS)
    set(_editor_compile_commands "${CMAKE_SOURCE_DIR}/compile_commands.json")
    if(IS_SYMLINK "${_editor_compile_commands}")
        file(REMOVE "${_editor_compile_commands}")
    elseif(EXISTS "${_editor_compile_commands}")
        message(FATAL_ERROR
            "${_editor_compile_commands} exists and is not a symlink. "
            "Remove or rename it so CMake can manage the active editor database.")
    endif()
    file(CREATE_LINK
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${_editor_compile_commands}"
        SYMBOLIC
        RESULT _editor_compile_commands_result)
    if(NOT _editor_compile_commands_result STREQUAL "0")
        message(FATAL_ERROR
            "Could not select the active editor compilation database: "
            "${_editor_compile_commands_result}")
    endif()
    unset(_editor_compile_commands)
    unset(_editor_compile_commands_result)
endif()
