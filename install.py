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

import importlib
import site
import subprocess
import sys
from pathlib import Path

MODULE = "stm32_installer"

# Installed straight from GitHub. Nothing to publish and nothing to sign up to,
# and whatever is on main is what everyone gets on their next run.
SOURCE = "https://github.com/nimaltd/stm32-installer/archive/refs/heads/main.zip"


def load():
    """Import the installer if it is present, otherwise None."""
    # A package installed moments ago is not on the import path of a process
    # that already looked for it, so the caches have to be dropped first.
    importlib.reload(site)
    importlib.invalidate_caches()

    try:
        return importlib.import_module(MODULE)
    except ImportError:
        return None


def fetch(already_installed):
    """
    Install or update the installer. True when it worked.

    An update forces the reinstall. Pip decides by comparing version numbers,
    and this package keeps the same version across many pushes to main, so a fix
    pushed today would never reach anyone who already ran this once.

    That update skips dependencies, because they are already there and fetching
    them again on every run would be a slow way to achieve nothing.
    """
    command = [sys.executable, "-m", "pip", "install", "--quiet"]

    if already_installed:
        command += ["--force-reinstall", "--no-deps"]

    try:
        subprocess.check_call(command + [SOURCE])
        return True
    except (subprocess.CalledProcessError, OSError):
        return False


def ensure_installer():
    """Get the newest installer that can be reached, and import it."""
    present = load()

    print("Checking for the latest installer ..." if present else "Fetching the installer ...",
          flush=True)

    if not fetch(already_installed=present is not None):
        if present is not None:
            # Being offline is not a reason to refuse. Whatever is already
            # installed still does the job.
            print("Could not reach GitHub, using the installed version.")
            return present

        print(
            "\nCould not install the installer automatically.\n"
            "Run this yourself and try again:\n\n"
            f"    {sys.executable} -m pip install {SOURCE}\n",
            file=sys.stderr,
        )
        return None

    installer = load()

    if installer is None:
        print(
            "\nThe installer was downloaded but could not be loaded.\n"
            "Running 'python install.py' once more usually settles it.",
            file=sys.stderr,
        )

    return installer


def main():
    installer = ensure_installer()

    if installer is None:
        return 2

    version = getattr(installer, "__version__", "")
    print(f"stm32-installer {version}".rstrip())

    return installer.main(library_root=Path(__file__).resolve().parent)


if __name__ == "__main__":
    sys.exit(main())
