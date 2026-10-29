cmake_minimum_required(VERSION 3.20)
project(UnboundAuditionTrace LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Audition tracing requires Windows x64 and MSVC")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt5 5.15.2 EXACT REQUIRED COMPONENTS Widgets)
get_filename_component(UNBOUND_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
add_library(AuditionTrace SHARED trace.cpp trace.h)
set_target_properties(AuditionTrace PROPERTIES OUTPUT_NAME AuditionTrace2)
target_link_libraries(AuditionTrace PRIVATE Qt5::Widgets Qt5::CorePrivate)

# Use the existing read-only probe injector, with a distinct DLL and four
# explicit commands. Existing Unbound is allowed; GammaRay remains excluded.
file(READ "${UNBOUND_ROOT}/tests/platforms/windows/inspector-probe/launcher.cpp" launcher)
function(replace_checked before after)
    string(FIND "${launcher}" "${before}" location)
    if(location EQUAL -1)
        message(FATAL_ERROR "Trace launcher anchor missing: ${before}")
    endif()
    string(REPLACE "${before}" "${after}" launcher "${launcher}")
    set(launcher "${launcher}" PARENT_SCOPE)
endfunction()
replace_checked("InspectorProbe.dll" "AuditionTrace2.dll")
replace_checked("int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR command, int)" "int wmain(int argc, wchar_t **argv)")
replace_checked([=[    try {
        if (std::wstring(command)]=] [=[    const wchar_t *command=argc==2 ? argv[1] : L"";
    try {
        if (std::wstring(command)]=])
replace_checked([=[require(std::wstring(command).empty(), L"Unknown argument. Run Unbound without arguments, or use --version.");]=]
    [=[const ULONG_PTR operation=std::wstring(command)==L"--start" ? 1 : std::wstring(command)==L"--mark" ? 2 : std::wstring(command)==L"--stop" ? 3 : std::wstring(command)==L"--inventory" ? 4 : 0;
        require(operation!=0, L"Use --start, --mark, --stop or --inventory.");]=])
replace_checked([=[            pid = resolveProcess();]=]
    [=[            pid = resolveProcess();
            require(pid!=0,L"Start Resolve and load the Fairlight page before tracing.");]=])
replace_checked([=[            nullptr) == 1,]=] [=[            reinterpret_cast<void *>(operation)) == 1,]=])
replace_checked([=[        return 0;
    } catch]=] [=[        std::puts("Trace command queued. JSONL captures are in the adjacent logs folder.");
        return 0;
    } catch]=])
replace_checked([=[        MessageBoxW(nullptr, error.c_str(), L"Unbound", MB_OK | MB_ICONERROR);]=]
    [=[        std::fwprintf(stderr,L"%ls\n",error.c_str());]=])
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/launcher.cpp" "#include <cstdio>\n${launcher}")
add_executable(audition-trace "${CMAKE_CURRENT_BINARY_DIR}/launcher.cpp")
set_target_properties(audition-trace PROPERTIES AUTOMOC OFF MSVC_RUNTIME_LIBRARY MultiThreaded)
target_compile_definitions(audition-trace PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX UNBOUND_VERSION=L"AuditionTrace")
target_link_libraries(audition-trace PRIVATE version)
add_executable(audition-trace-tests tests.cpp trace.cpp trace.h)
target_link_libraries(audition-trace-tests PRIVATE Qt5::Widgets Qt5::CorePrivate)
