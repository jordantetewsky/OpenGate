cmake_minimum_required(VERSION 3.20)
project(UnboundAuditionScope LANGUAGES CXX)
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Audition context test requires Windows x64 and MSVC")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt5 5.15.2 EXACT REQUIRED COMPONENTS Widgets)
get_filename_component(UNBOUND_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../.." ABSOLUTE)
function(replace_checked variable before after)
    string(FIND "${${variable}}" "${before}" location)
    if(location EQUAL -1)
        message(FATAL_ERROR "Scope test anchor missing: ${before}")
    endif()
    string(REPLACE "${before}" "${after}" changed "${${variable}}")
    set(${variable} "${changed}" PARENT_SCOPE)
endfunction()

# Isolated variant of the existing passive recorder. Keep its baseline intact.
set(overlay "${CMAKE_CURRENT_BINARY_DIR}/overlay")
file(MAKE_DIRECTORY "${overlay}")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../audition-trace/trace.h" header)
replace_checked(header "#include <atomic>" "#include <atomic>\n#include \"bridge.h\"")
replace_checked(header "    QString m_file;" "    QString m_file;\n    QPointer<AuditionScopeBridge> m_bridge;\n    QPointer<QObject> m_scopeManager;\n    bool m_scopeAllowed=false;")
file(WRITE "${overlay}/trace.h" "${header}")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../audition-trace/trace.cpp" recorder)
replace_checked(recorder [=[qApp->installEventFilter(this); m_deadline.start(); flush();]=]
    [=[qApp->installEventFilter(this); m_deadline.start();
        if (m_scopeAllowed) {
            QWidget *preview=nullptr;
            for (auto player:m_players) if (player && player->objectName()=="previewPlayer") preview=player;
            m_bridge=new AuditionScopeBridge(m_main,m_page,preview,[this] {
                return scopeNativeState(m_scopeManager);
            },[this](const QString &kind,QJsonObject data) {
                data.insert("state",state()); record(kind,data);
            },this);
        }
        record("scope-ready",{{"enabled",m_bridge && m_bridge->enabled()}});
        if (!m_bridge || !m_bridge->enabled()) finish("scope-prerequisites-missing");
        flush();]=])
replace_checked(recorder [=[m_finishing=true; m_deadline.stop(); qApp->removeEventFilter(this);]=]
    [=[m_finishing=true; m_deadline.stop(); qApp->removeEventFilter(this);
    if (m_bridge) { m_bridge->stop(); m_bridge->deleteLater(); m_bridge=nullptr; }]=])
# Window activation is propagated to many child widgets: retain only the window
# event. Hide/show is not needed for this intervention. Avoid premature log caps.
replace_checked(recorder [=[case QEvent::Show: case QEvent::Hide: case QEvent::KeyPress: break;]=]
    [=[case QEvent::KeyPress: break;]=])
replace_checked(recorder [=[auto widget=static_cast<QWidget *>(object);]=]
    [=[auto widget=static_cast<QWidget *>(object);
    if ((event->type()==QEvent::WindowActivate || event->type()==QEvent::WindowDeactivate) && !widget->isWindow()) return false;]=])
replace_checked(recorder [=[#include "trace.h"]=] [=[#include "trace.h"
#include "native.h"]=])
replace_checked(recorder [=[m_main=nullptr; m_page=nullptr; m_slot=nullptr;]=]
    [=[m_main=nullptr; m_page=nullptr; m_slot=nullptr; m_scopeManager=nullptr; m_scopeAllowed=false;]=])
replace_checked(recorder [=[    snapshot(observe ? "start" : "inventory");]=]
    [=[    QWidget *preview=nullptr;
    for (auto player:m_players) if (player && player->objectName()=="previewPlayer") preview=player;
    if (m_managers.size()==1 && m_main && m_page && m_slot && preview) {
        const auto map=AuditionMap::snapshot(m_managers.first(),preview,m_page,m_slot);
        record("scope-inventory",map);
        m_scopeAllowed=map.value("valid").toBool() && scopeCodeSupported();
        if (m_scopeAllowed) m_scopeManager=m_managers.first();
    }
    record("scope-gate",{{"allowed",m_scopeAllowed}});
    snapshot(observe ? "start" : "inventory");]=])
replace_checked(recorder [=[    data.insert("kind",kind);]=]
    [=[    if (m_scopeAllowed && m_scopeManager) data.insert("nativeState",scopeNativeState(m_scopeManager));
    data.insert("kind",kind);]=])
file(WRITE "${overlay}/trace.cpp" "${recorder}")
add_library(AuditionScope SHARED "${overlay}/trace.cpp" "${overlay}/trace.h" bridge.h "${CMAKE_CURRENT_SOURCE_DIR}/../audition-map/mapreader.cpp")
target_include_directories(AuditionScope PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../audition-map")
target_link_libraries(AuditionScope PRIVATE Qt5::Widgets Qt5::CorePrivate)

# Same injector and explicit commands as the passive recorder, separate DLL.
file(READ "${UNBOUND_ROOT}/tests/platforms/windows/inspector-probe/launcher.cpp" launcher)
replace_checked(launcher "InspectorProbe.dll" "AuditionScope.dll")
replace_checked(launcher "int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR command, int)" "int wmain(int argc, wchar_t **argv)")
replace_checked(launcher [=[    try {
        if (std::wstring(command)]=] [=[    const wchar_t *command=argc==2 ? argv[1] : L"";
    try {
        if (std::wstring(command)]=])
replace_checked(launcher [=[require(std::wstring(command).empty(), L"Unknown argument. Run Unbound without arguments, or use --version.");]=]
    [=[const ULONG_PTR operation=std::wstring(command)==L"--start" ? 1 : std::wstring(command)==L"--mark" ? 2 : std::wstring(command)==L"--stop" ? 3 : std::wstring(command)==L"--inventory" ? 4 : 0;
        require(operation!=0, L"Use --start, --mark, --stop or --inventory.");]=])
replace_checked(launcher [=[            pid = resolveProcess();]=]
    [=[            pid = resolveProcess();
            require(pid!=0,L"Start Resolve and load the Fairlight page before testing.");]=])
replace_checked(launcher [=[            nullptr) == 1,]=] [=[            reinterpret_cast<void *>(operation)) == 1,]=])
replace_checked(launcher [=[        return 0;
    } catch]=] [=[        std::puts("Scope test command queued. Captures are in the adjacent logs folder.");
        return 0;
    } catch]=])
replace_checked(launcher [=[        MessageBoxW(nullptr, error.c_str(), L"Unbound", MB_OK | MB_ICONERROR);]=]
    [=[        std::fwprintf(stderr,L"%ls\n",error.c_str());]=])
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/launcher.cpp" "#include <cstdio>\n${launcher}")
add_executable(audition-scope "${CMAKE_CURRENT_BINARY_DIR}/launcher.cpp")
set_target_properties(audition-scope PROPERTIES AUTOMOC OFF MSVC_RUNTIME_LIBRARY MultiThreaded)
target_compile_definitions(audition-scope PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX UNBOUND_VERSION=L"AuditionScope")
target_link_libraries(audition-scope PRIVATE version)
add_executable(audition-scope-tests tests.cpp)
target_link_libraries(audition-scope-tests PRIVATE Qt5::Widgets)
