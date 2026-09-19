#!/usr/bin/env python3
"""
Install this library into your STM32 project.

You already downloaded this repository into your project, so there is nothing to
choose. Running this turns the repository into a plain library folder: the header
and source move up to the top, a config file is created for you, and everything
that belongs to the repository rather than to your firmware is removed.

That last part matters. STM32CubeIDE compiles every .c file under your project,
and this repository ships a test suite with its own main(), which would break
your build.

    python install.py

Your own fsm_config.h is never overwritten, so this is also how you update.
"""

import subprocess
import sys
from pathlib import Path

MODULE = "stm32_installer"

# Installed straight from GitHub, so there is nothing to publish and nothing for
# you to sign up to. Swap this for "stm32-installer" if it ever reaches PyPI.
SOURCE = "https://github.com/nimaltd/stm32-installer/archive/refs/heads/main.zip"


def ensure_installer():
    """Import the installer, fetching it from GitHub the first time."""
    try:
        return __import__(MODULE)
    except ImportError:
        pass

    print("Fetching the installer from GitHub ...", flush=True)

    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "--quiet", SOURCE])
    except subprocess.CalledProcessError:
        print(
            "\nCould not install the installer automatically.\n"
            "Run this yourself and try again:\n\n"
            f"    {sys.executable} -m pip install {SOURCE}\n",
            file=sys.stderr,
        )
        return None

    # A fresh install is not on the import path of a process that already looked.
    import importlib
    import site

    importlib.reload(site)
    importlib.invalidate_caches()

    try:
        return importlib.import_module(MODULE)
    except ImportError:
        print(
            "\nThe installer was downloaded but could not be loaded.\n"
            "Run 'python install.py' once more, which usually settles it.",
            file=sys.stderr,
        )
        return None


def main():
    installer = ensure_installer()

    if installer is None:
        return 2

    return installer.main(library_root=Path(__file__).resolve().parent)


if __name__ == "__main__":
    sys.exit(main())
