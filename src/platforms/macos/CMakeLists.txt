# Reserved for the native macOS backend. Do not silently build Windows fallbacks.
message(FATAL_ERROR
    "The macOS backend is not implemented yet. See doc/platforms/macos.md. "
    "Windows releases remain available through build-unbound-release.cmd on Windows x64.")
