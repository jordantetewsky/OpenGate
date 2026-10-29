#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* A file marker proves startup loading without touching Resolve's UI. */
__attribute__((constructor)) static void record_load(void)
{
    const char *path = getenv("UNBOUND_PROBE_LOG");
    if (!path || !*path) return;
    FILE *file = fopen(path, "a");
    if (!file) return;
    fprintf(file, "Unbound loading probe: pid=%d\n", getpid());
    fclose(file);
}
