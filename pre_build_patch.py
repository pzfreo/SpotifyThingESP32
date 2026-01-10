Import("env")
import os

# Patch roo_io to skip ESP-IDF specific SDMMC code that doesn't compile with Arduino
def patch_roo_io(source, target, env):
    libdeps_dir = env.subst("$PROJECT_LIBDEPS_DIR")
    env_name = env.subst("$PIOENV")

    problematic_files = [
        os.path.join(libdeps_dir, env_name, "roo_io", "src", "roo_io", "fs", "esp32", "esp-idf", "sdmmc.cpp"),
        os.path.join(libdeps_dir, env_name, "roo_io", "src", "roo_io", "fs", "esp32", "esp-idf", "sdspi.cpp"),
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
