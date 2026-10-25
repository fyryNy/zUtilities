cmake_minimum_required(VERSION 3.25)

include("${CMAKE_CURRENT_LIST_DIR}/../IntelliSenseProperties.cmake")
if(NOT DEFINED TEST_BINARY_DIR)
    set(TEST_BINARY_DIR "${CMAKE_CURRENT_LIST_DIR}/../../out/tests/intellisense-properties")
endif()
get_filename_component(TEST_BINARY_DIR "${TEST_BINARY_DIR}" ABSOLUTE)
file(MAKE_DIRECTORY "${TEST_BINARY_DIR}")

function(assert_equal actual expected)
    if(NOT "${actual}" STREQUAL "${expected}")
        message(FATAL_ERROR "Expected [${expected}], got [${actual}]")
    endif()
endfunction()

# Discover newer toolsets under a nondefault root, without invoking a compiler.
set(MSVC_WINE_ROOT "${TEST_BINARY_DIR}/alternate MSVC root")
foreach(_version 14.9.1 14.51.36231 14.99.12345)
    file(MAKE_DIRECTORY "${MSVC_WINE_ROOT}/VC/Tools/MSVC/${_version}")
endforeach()
foreach(_version 10.0.9.0 10.0.26100.0 10.0.99999.0)
    file(MAKE_DIRECTORY "${MSVC_WINE_ROOT}/Windows Kits/10/Include/${_version}")
endforeach()
foreach(_tool cl link rc mt)
    file(WRITE "${MSVC_WINE_ROOT}/bin/x86/${_tool}" "")
endforeach()
set(CMAKE_BINARY_DIR "${TEST_BINARY_DIR}/toolchain")
include("${CMAKE_CURRENT_LIST_DIR}/../msvc-wine-x86.cmake")
list(GET CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES 0 _msvc_include)
list(GET CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES 2 _sdk_include)
assert_equal("${_msvc_include}"
    "${MSVC_WINE_ROOT}/VC/Tools/MSVC/14.99.12345/atlmfc/include")
assert_equal("${_sdk_include}"
    "${MSVC_WINE_ROOT}/Windows Kits/10/Include/10.0.99999.0/shared")

set(_source "${TEST_BINARY_DIR}/project \"quoted\" root")
set(_union_root "${TEST_BINARY_DIR}/alternate Union root")
set(_project_includes
    "${_source}/zUtilities"
    "${_source}/zUtilities/ZenGin/Gothic_UserAPI"
    "${_union_root}/Union/v1.0m/Include"
    "${_union_root}/DirectX7/include"
    "relative extra include"
    "${TEST_BINARY_DIR}/extra \"quoted\" include")
set(_expected_includes ${_project_includes} ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES})
set(_compiler "${TEST_BINARY_DIR}/compiler \"quoted\"/clang-cl")
set(_first "${TEST_BINARY_DIR}/preset one/c_cpp_properties.json")
zutilities_generate_cpptools_config("${_first}" "${_source}"
    COMPILER "${_compiler}" INCLUDE_DIRECTORIES ${_expected_includes})
file(READ "${_first}" _json)
string(JSON _count LENGTH "${_json}" configurations 0 includePath)
list(LENGTH _expected_includes _expected_count)
assert_equal("${_count}" "${_expected_count}")
set(_index 0)
foreach(_expected IN LISTS _expected_includes)
    get_filename_component(_expected "${_expected}" ABSOLUTE BASE_DIR "${_source}")
    string(JSON _actual GET "${_json}" configurations 0 includePath ${_index})
    assert_equal("${_actual}" "${_expected}")
    math(EXPR _index "${_index} + 1")
endforeach()
string(JSON _actual GET "${_json}" configurations 0 compilerPath)
assert_equal("${_actual}" "${_compiler}")
string(JSON _actual GET "${_json}" configurations 0 forcedInclude 0)
assert_equal("${_actual}" "\${workspaceFolder}/cmake/IntelliSenseContext.h")
string(JSON _unused ERROR_VARIABLE _missing GET "${_json}" configurations 0 compileCommands)
if(_missing STREQUAL "NOTFOUND")
    message(FATAL_ERROR "cpptools must keep native forcedInclude without compileCommands")
endif()

set(_escaped "back\\slash \"quote\"\nnew line\ttab\rcarriage return")
zutilities_json_string(_encoded "${_escaped}")
string(JSON _decoded GET "[${_encoded}]" 0)
assert_equal("${_decoded}" "${_escaped}")

# A tracked relative pointer remains unchanged while the ignored second hop switches.
file(MAKE_DIRECTORY "${_source}/.vscode" "${_source}/cmake")
set(_active "${_source}/cmake/IntelliSenseActiveProperties.json")
set(_workspace_link "${_source}/.vscode/c_cpp_properties.json")
if(NOT IS_SYMLINK "${_workspace_link}")
    file(CREATE_LINK "../cmake/IntelliSenseActiveProperties.json" "${_workspace_link}" SYMBOLIC)
endif()
zutilities_select_cpptools_config("${_first}" "${_active}")
file(READ "${_workspace_link}" _selected)
assert_equal("${_selected}" "${_json}")
set(_second "${TEST_BINARY_DIR}/preset two/c_cpp_properties.json")
zutilities_generate_cpptools_config("${_second}" "${_source}"
    COMPILER "${_compiler}" INCLUDE_DIRECTORIES "${TEST_BINARY_DIR}/different SDK")
zutilities_select_cpptools_config("${_second}" "${_active}")
file(READ "${_workspace_link}" _selected)
string(JSON _actual GET "${_selected}" configurations 0 includePath 0)
assert_equal("${_actual}" "${TEST_BINARY_DIR}/different SDK")
file(READ_SYMLINK "${_workspace_link}" _pointer)
assert_equal("${_pointer}" "../cmake/IntelliSenseActiveProperties.json")

# A user-owned regular file must survive a failed attempt to select a preset.
set(_custom "${TEST_BINARY_DIR}/custom.json")
file(WRITE "${_custom}" "custom configuration")
set(_guard "${TEST_BINARY_DIR}/check-custom.cmake")
file(WRITE "${_guard}"
    "include(\"${CMAKE_CURRENT_LIST_DIR}/../IntelliSenseProperties.cmake\")\n"
    "zutilities_select_cpptools_config(\"${_first}\" \"${_custom}\")\n")
execute_process(COMMAND "${CMAKE_COMMAND}" -P "${_guard}"
    RESULT_VARIABLE _result OUTPUT_QUIET ERROR_QUIET)
if(_result EQUAL 0)
    message(FATAL_ERROR "A regular configuration file was overwritten")
endif()
file(READ "${_custom}" _actual)
assert_equal("${_actual}" "custom configuration")
message(STATUS "IntelliSense configuration regression checks passed")
