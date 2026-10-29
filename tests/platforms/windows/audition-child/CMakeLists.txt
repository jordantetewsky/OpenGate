cmake_minimum_required(VERSION 3.20)
project(UnboundAuditionChild LANGUAGES CXX RC)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(UNBOUND_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
include("${UNBOUND_ROOT}/src/platforms/windows/windowing.cmake")

# Generate an isolated overlay of current production sources. Fail on stale
# replacement anchors rather than silently building an unchanged experiment.
function(replace_checked variable before after)
    string(FIND "${${variable}}" "${before}" location)
    if(location EQUAL -1)
        message(FATAL_ERROR "Audition experiment anchor missing: ${before}")
    endif()
    string(REPLACE "${before}" "${after}" changed "${${variable}}")
    set(${variable} "${changed}" PARENT_SCOPE)
endfunction()
set(overlay "${CMAKE_CURRENT_BINARY_DIR}/overlay")
file(MAKE_DIRECTORY "${overlay}")
file(READ "${UNBOUND_ROOT}/src/core/sidebarfloatsession.h" session)
replace_checked(session [=[host->setParent(slot, Qt::Window);]=]
    [=[host->setParent(m_root, Qt::Widget);]=])
replace_checked(session [=[inheritPaneTaskbarIdentity(host, slot->window());]=]
    [=[if (host->isWindow()) inheritPaneTaskbarIdentity(host, slot->window());]=])
replace_checked(session [=[if (!m_savedGeometry.isEmpty()) host->restoreGeometry(m_savedGeometry);]=]
    [=[if (!host->isWindow()) {
            fadeIn=false;
            host->move(24,48);
        } else if (!m_savedGeometry.isEmpty()) host->restoreGeometry(m_savedGeometry);]=])
replace_checked(session [=[applyPaneDarkCaption(host, slot->window());]=]
    [=[if (host->isWindow()) applyPaneDarkCaption(host, slot->window());]=])
replace_checked(session [=[applyPaneDarkCaption(m_host, m_root->window());]=]
    [=[if (m_host->isWindow()) applyPaneDarkCaption(m_host, m_root->window());]=])
replace_checked(session [=[        host->show();]=]
    [=[        host->show();
        if (!host->isWindow()) host->raise();]=])
replace_checked(session [=[        if (!m_host) return;]=]
    [=[        if (!m_host || !m_host->isWindow()) return;]=])
file(WRITE "${overlay}/sidebarfloatsession.h" "${session}")

file(READ "${UNBOUND_ROOT}/src/core/controller.cpp" controller)
replace_checked(controller [=[Unbound::PaneSettings m_settings;]=]
    [=[Unbound::PaneSettings m_settings{qEnvironmentVariable("UNBOUND_SETTINGS_FILE",
        QDir(qEnvironmentVariable("LOCALAPPDATA")).filePath("Unbound/audition-child.json"))};]=])
file(WRITE "${overlay}/controller.cpp" "${controller}")

include("${UNBOUND_ROOT}/infrastructure/windows/version.cmake")
string(APPEND UNBOUND_VERSION "-audition-child")
set(native "${UNBOUND_ROOT}/src/platforms/windows")
set(UNBOUND_FILE_TYPE VFT_APP)
set(UNBOUND_FILENAME Unbound.exe)
configure_file("${native}/resources/version.rc.in" launcher.rc @ONLY)
set(UNBOUND_FILE_TYPE VFT_DLL)
set(UNBOUND_FILENAME Unbound.dll)
configure_file("${native}/resources/version.rc.in" helper.rc @ONLY)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/icon.rc" "1 ICON \"${native}/resources/unbound.ico\"\n")
add_executable(Unbound WIN32 "${native}/launcher.cpp" "${CMAKE_CURRENT_BINARY_DIR}/launcher.rc" "${CMAKE_CURRENT_BINARY_DIR}/icon.rc")
set_property(TARGET Unbound PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
target_compile_definitions(Unbound PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX UNBOUND_VERSION=L"${UNBOUND_VERSION}")
target_link_libraries(Unbound PRIVATE version)
add_library(UnboundHelper SHARED "${overlay}/controller.cpp" "${native}/bootstrap.cpp" "${CMAKE_CURRENT_BINARY_DIR}/helper.rc")
set_target_properties(UnboundHelper PROPERTIES OUTPUT_NAME Unbound)
target_link_libraries(UnboundHelper PRIVATE UnboundWindowsWindowing)

# Reuse all nine pane lifecycle checks, changing the Media Pool expectation
# from a top-level window to a child in the original page/window.
file(READ "${UNBOUND_ROOT}/tests/core/pane-float/fairlighttest.h" fixture)
replace_checked(fixture [=[auto host=contents[i]->window();]=]
    [=[auto host=contents[i]->parentWidget();]=])
replace_checked(fixture [=[host->isWindow() && host->parentWidget()==paneSlots[i] && reachesPage]=]
    [=[!host->isWindow() && host->parentWidget()==page && reachesPage
                && contents[i]->window()==&main && page->isAncestorOf(contents[i])]=])
replace_checked(fixture [=[Floating audition lost its Fairlight ancestor path]=]
    [=[Child audition lost original window or same-window page ancestry]=])
file(WRITE "${overlay}/fairlighttest.h" "${fixture}")
file(WRITE "${overlay}/main.cpp" "#include \"fixtures.h\"\n#include \"fairlighttest.h\"\nint main(int argc,char **argv) { initializePlatformTests(); QApplication app(argc,argv); return runFairlightTest(app) ? 0 : 1; }\n")
add_executable(audition-child-smoke "${overlay}/main.cpp" "${UNBOUND_ROOT}/tests/platforms/windows/windowing/platformtests.cpp")
target_include_directories(audition-child-smoke PRIVATE "${UNBOUND_ROOT}/tests/core/pane-float")
target_link_libraries(audition-child-smoke PRIVATE UnboundWindowsWindowing)
