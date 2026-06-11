Import("env")
import os

# Patch roo_io to skip ESP-IDF specific SDMMC code that doesn't compile with Arduino
def patch_roo_io(source, target, env):
    import glob

    libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
    env_name = env.subst("$PIOENV")

    # The libdeps folder is named after the dependency spec: a pinned
    # "dejwk/roo_io @ 2.1.2" installs as "roo_io@2.1.2", so glob for it.
    roo_io_dirs = glob.glob(os.path.join(libdeps_dir, env_name, "roo_io*"))

    relative_files = [
        ("fs", "esp32", "esp-idf", "sdmmc.cpp"),
        ("fs", "esp32", "esp-idf", "sdspi.cpp"),
        ("fs", "esp32", "arduino", "sdmmc.cpp"),
        ("fs", "esp32", "arduino", "sdmmc.h"),
        ("fs", "esp32", "arduino", "sdspi.cpp"),
        ("fs", "esp32", "arduino", "sdspi.h"),
        ("fs", "arduino", "sdfs.cpp"),
        ("fs", "arduino", "sdfs.h"),
    ]

    problematic_files = [
        os.path.join(d, "src", "roo_io", *rel)
        for d in roo_io_dirs
        for rel in relative_files
    ]

    for filepath in problematic_files:
        if os.path.exists(filepath):
            # Wrap entire file content in #if 0 to disable compilation
            with open(filepath, 'r') as f:
                content = f.read()

            if not content.startswith("// PATCHED"):
                patched = "// PATCHED: Disabled for Arduino compatibility\n#if 0\n" + content + "\n#endif\n"
                with open(filepath, 'w') as f:
                    f.write(patched)
                print(f"Patched: {filepath}")

# Run before building
env.AddPreAction("buildprog", patch_roo_io)
env.AddPreAction(".pio/build/$PIOENV/src/DisplayManager.cpp.o", patch_roo_io)
