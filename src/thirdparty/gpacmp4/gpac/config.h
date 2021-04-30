// hack so that we can define GPAC_HAVE_CONFIG_H to make GPAC not use the default configuration.h

// We don't use configuration.h because:
// - It defines GPAC_MEMORY_TRACKING on Windows which would mean we would have to pull in alloc.c if we did that
// - It doesn't support Linux, which makes it fail to compile with an "Unknown target platform" error