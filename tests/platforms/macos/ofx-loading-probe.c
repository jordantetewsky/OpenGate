#include <ofxCore.h>
#include <dlfcn.h>
#include <dispatch/dispatch.h>
#include <mach-o/dyld.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

static void record_state(const char *phase, int inspect_windows)
{
    const char *home = getenv("HOME");
    if (!home) return;
    char path[4096];
    int length = snprintf(path, sizeof(path), "%s/Library/Logs/OpenGate/ofx-loading-probe.log", home);
    if (length < 0 || (size_t)length >= sizeof(path)) return;
    FILE *log = fopen(path, "a");
    if (!log) return;
    char executable[4096] = "unknown";
    uint32_t size = sizeof(executable);
    if (_NSGetExecutablePath(executable, &size) != 0)
        snprintf(executable, sizeof(executable), "path exceeds buffer");
    /* Read the exported singleton pointer without linking another Qt runtime. */
    void **application = (void **)dlsym(RTLD_DEFAULT, "_ZN16QCoreApplication4selfE");
    fprintf(log, "time=%lld phase=%s pid=%d main_thread=%d executable=%s qt_symbol=%s qt_application=%s",
            (long long)time(NULL), phase, getpid(), pthread_main_np(), executable,
            application ? "found" : "absent",
            application && *application ? "present" : "absent");
    if (inspect_windows && pthread_main_np() && application && *application) {
        void *(*active_window)(void) = (void *(*)(void))dlsym(RTLD_DEFAULT, "_ZN12QApplication12activeWindowEv");
        fprintf(log, " active_window=%s", active_window && active_window() ? "present" : "absent");
    }
    fputc('\n', log);
    fclose(log);
}

static void inspect_on_main(void *context)
{
    void **application = (void **)dlsym(RTLD_DEFAULT, "_ZN16QCoreApplication4selfE");
    if (!application || !*application) {
        uintptr_t attempt = (uintptr_t)context;
        if (attempt < 120) {
            dispatch_after_f(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 2),
                dispatch_get_main_queue(), (void *)(attempt + 1), inspect_on_main);
        } else {
            record_state("qt-startup-timeout", 0);
        }
        return;
    }
    record_state("main-queue", 1);
    Dl_info info;
    if (!dladdr((const void *)&inspect_on_main, &info)) return;
    char path[4096];
    int length = snprintf(path, sizeof(path), "%s", info.dli_fname);
    if (length < 0 || (size_t)length >= sizeof(path)) return;
    char *name = strrchr(path, '/');
    if (!name) return;
    *name = '\0';
    const char *suffix = "/UnboundWidgetProbe.dylib";
    if (strlen(path) + strlen(suffix) >= sizeof(path)) return;
    strcat(path, suffix);
    if (access(path, R_OK) != 0) return;
    // The optional Qt inspector is loaded only inside the Qt host, on main.
    void *library = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!library) { record_state("widget-library-load-failed", 0); return; }
    void (*inspect)(void) = (void (*)(void))dlsym(library, "unboundInspectWidgets");
    if (inspect) { record_state("widget-inspection-start", 0); inspect(); }
    // Retain the library while its bounded Qt timer callbacks are pending.
}

/* Discovery only: advertise no effects and never change a project or widget. */
__attribute__((visibility("default"))) int OfxGetNumberOfPlugins(void)
{
    record_state("discovery", 0);
    static atomic_flag scheduled = ATOMIC_FLAG_INIT;
    if (dlsym(RTLD_DEFAULT, "_ZN16QCoreApplication4selfE") &&
        !atomic_flag_test_and_set(&scheduled)) {
        /* Keep this callback's code loaded after the host releases the probe. */
        Dl_info info;
        if (dladdr((const void *)&inspect_on_main, &info) &&
            dlopen(info.dli_fname, RTLD_NOW | RTLD_LOCAL)) {
            dispatch_async_f(dispatch_get_main_queue(), NULL, inspect_on_main);
        } else {
            record_state("callback-not-scheduled", 0);
        }
    }
    return 0;
}

__attribute__((visibility("default"))) OfxPlugin *OfxGetPlugin(int index)
{
    (void)index;
    return NULL;
}
