"""
PlatformIO script to enable ccache for faster rebuilds.
Only activates if ccache is available on the system.
"""
import os
import shutil

Import("env")

def configure_ccache(env):
    """Configure ccache wrapper for the C/C++ compilers if available."""
    ccache_path = shutil.which("ccache")
    if not ccache_path:
        print("ccache not found, building without cache")
        return

    # Get current compiler paths
    cc = env.get("CC", "gcc")
    cxx = env.get("CXX", "g++")

    # Only wrap if not already wrapped
    if "ccache" not in str(cc):
        env.Replace(CC=f"ccache {cc}")
        env.Replace(CXX=f"ccache {cxx}")
        print(f"ccache enabled: {ccache_path}")

configure_ccache(env)
