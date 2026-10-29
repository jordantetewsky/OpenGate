#include <dlfcn.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 2) return 1;
    void *library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) { fprintf(stderr, "%s\n", dlerror()); return 1; }
    int (*count)(void) = (int (*)(void))dlsym(library, "OfxGetNumberOfPlugins");
    void *(*plugin)(int) = (void *(*)(int))dlsym(library, "OfxGetPlugin");
    int failed = !count || !plugin;
    if (!failed) failed = count() != 0 || plugin(0) != NULL;
    dlclose(library);
    return failed;
}
