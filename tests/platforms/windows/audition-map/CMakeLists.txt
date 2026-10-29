cmake_minimum_required(VERSION 3.20)
project(UnboundAuditionMap LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Audition map diagnostic requires Windows x64 and MSVC")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt5 5.15.2 EXACT REQUIRED COMPONENTS Widgets)
get_filename_component(UNBOUND_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
function(replace_checked variable before after)
    string(FIND "${${variable}}" "${before}" location)
    if(location EQUAL -1)
        message(FATAL_ERROR "Audition map anchor missing: ${before}")
    endif()
    string(REPLACE "${before}" "${after}" changed "${${variable}}")
    set(${variable} "${changed}" PARENT_SCOPE)
endfunction()
set(overlay "${CMAKE_CURRENT_BINARY_DIR}/overlay")
file(MAKE_DIRECTORY "${overlay}")
configure_file("../audition-trace/trace.h" "${overlay}/trace.h" COPYONLY)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../audition-trace/trace.cpp" recorder)
replace_checked(recorder [=[#include "trace.h"]=] [=[#include "trace.h"
#include "mapreader.h"]=])
replace_checked(recorder [=[    snapshot(observe ? "start" : "inventory");]=]
    [=[    QWidget *preview=nullptr;
    for (auto player:m_players) if (player && player->objectName()=="previewPlayer") preview=player;
    if (m_managers.size()==1 && m_page && m_slot && preview)
        record("context-map",AuditionMap::snapshot(m_managers.first(),preview,m_page,m_slot));
    else record("context-map",{{"valid",false},{"error","missing-or-ambiguous-Fairlight-objects"}});
    snapshot("context-map-snapshot");]=])
# This DLL only accepts one-shot captures, never installs ongoing observers.
replace_checked(recorder [=[if (!app || QCoreApplication::closingDown() || operation<1 || operation>4) return 0;]=]
    [=[if (!app || QCoreApplication::closingDown() || operation!=4) return 0;]=])
file(WRITE "${overlay}/trace.cpp" "${recorder}")
add_library(AuditionMap SHARED "${overlay}/trace.cpp" "${overlay}/trace.h" mapreader.cpp mapreader.h)
set_target_properties(AuditionMap PROPERTIES OUTPUT_NAME AuditionMap2)
target_include_directories(AuditionMap PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(AuditionMap PRIVATE Qt5::Widgets Qt5::CorePrivate)

file(READ "${UNBOUND_ROOT}/tests/platforms/windows/inspector-probe/launcher.cpp" launcher)
replace_checked(launcher "InspectorProbe.dll" "AuditionMap2.dll")
replace_checked(launcher "int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR command, int)" "int wmain(int argc, wchar_t **argv)")
replace_checked(launcher [=[    try {
        if (std::wstring(command)]=] [=[    const wchar_t *command=argc==2 ? argv[1] : L"";
    try {
        if (std::wstring(command)]=])
replace_checked(launcher [=[require(std::wstring(command).empty(), L"Unknown argument. Run Unbound without arguments, or use --version.");]=]
    [=[require(argc==2 && std::wstring(command)==L"--capture", L"Use --capture for a read-only context snapshot.");]=])
replace_checked(launcher [=[            pid = resolveProcess();]=]
    [=[            pid = resolveProcess();
            require(pid!=0,L"Start Resolve and load the Fairlight page before capturing.");]=])
replace_checked(launcher [=[            nullptr) == 1,]=] [=[            reinterpret_cast<void *>(4)) == 1,]=])
replace_checked(launcher [=[        return 0;
    } catch]=] [=[        std::puts("Read-only context snapshot queued; output is in the adjacent logs folder.");
        return 0;
    } catch]=])
replace_checked(launcher [=[        MessageBoxW(nullptr, error.c_str(), L"Unbound", MB_OK | MB_ICONERROR);]=]
    [=[        std::fwprintf(stderr,L"%ls\n",error.c_str());]=])
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/launcher.cpp" "#include <cstdio>\n${launcher}")
add_executable(audition-map "${CMAKE_CURRENT_BINARY_DIR}/launcher.cpp")
set_target_properties(audition-map PROPERTIES AUTOMOC OFF MSVC_RUNTIME_LIBRARY MultiThreaded)
target_compile_definitions(audition-map PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX UNBOUND_VERSION=L"AuditionMap")
target_link_libraries(audition-map PRIVATE version)
add_executable(audition-map-tests tests.cpp mapreader.cpp)
target_link_libraries(audition-map-tests PRIVATE Qt5::Widgets)
