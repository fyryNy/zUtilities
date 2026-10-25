cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_LIST_DIR}/../VisualStudioConfiguration.cmake")

# A PropertyGroup match must not capture the first unrelated compiler group.
set(_xml [=[
<Project>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='G1 MT Release|Win32'" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='G2A MT Release|Win32'" />
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='G1 MT Release|Win32'">
    <ClCompile><LanguageStandard>stdcpp17</LanguageStandard></ClCompile>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='G2A MT Release|Win32'">
    <ClCompile><LanguageStandard>stdcpp20</LanguageStandard></ClCompile>
  </ItemDefinitionGroup>
</Project>
]=])
zutilities_vcxproj_configuration(_selected "${_xml}" "G2A MT Release")
if(NOT _selected MATCHES "<LanguageStandard>stdcpp20</LanguageStandard>" OR
        _selected MATCHES "stdcpp17")
    message(FATAL_ERROR "Selected compiler settings came from the wrong configuration")
endif()
zutilities_vcxproj_configuration(_selected "${_xml}" "Missing Debug")
if(NOT _selected STREQUAL "")
    message(FATAL_ERROR "Missing configuration inherited another configuration's settings")
endif()

file(READ "${CMAKE_CURRENT_LIST_DIR}/../../zUtilities/zUtilities.vcxproj" _xml)
foreach(_game G1 G1A G2 G2A "MP x2" "MP x4")
    zutilities_vcxproj_configuration(_selected "${_xml}" "${_game} Debug")
    if(NOT _selected MATCHES "<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>" OR
            NOT _selected MATCHES "_DEBUG")
        message(FATAL_ERROR "${_game} Debug requires matching _DEBUG and debug DLL runtime")
    endif()
    foreach(_runtime MT MD)
        zutilities_vcxproj_configuration(_selected "${_xml}" "${_game} ${_runtime} Release")
        if(_selected MATCHES "MultiThreadedDebug" OR _selected MATCHES "_DEBUG")
            message(FATAL_ERROR "${_game} ${_runtime} Release inherited debug settings")
        endif()
    endforeach()
endforeach()
message(STATUS "Visual Studio configuration regression checks passed")
