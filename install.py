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

Nothing is installed on your machine. The installer is fetched into a temporary
folder, used, and deleted. No pip, no packages, no leftovers, and you always get
the current version because there is never an old one lying around.
"""

import io
import shutil
import subprocess
import sys
import tempfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

MODULE = "stm32_installer"
SOURCE = "https://github.com/nimaltd/stm32-installer/archive/refs/heads/main.zip"
TIMEOUT_SECONDS = 30


def fetch_to(folder):
    """
    Download the installer and unpack it. Returns the folder to import from.

    None when it could not be fetched, which is not fatal on its own: an already
    installed copy is tried next.
    """
    try:
        with urllib.request.urlopen(SOURCE, timeout=TIMEOUT_SECONDS) as response:
            raw = response.read()
    except (urllib.error.URLError, OSError) as error:
        print(f"Could not reach GitHub: {error}", file=sys.stderr)
        return None

    try:
        zipfile.ZipFile(io.BytesIO(raw)).extractall(folder)
    except (zipfile.BadZipFile, OSError) as error:
        print(f"The download was not a usable archive: {error}", file=sys.stderr)
        return None

    # The archive holds one top level folder named after the repository and branch.
    found = sorted(folder.glob("*/src"))

    if not found:
        print("The download did not contain the installer.", file=sys.stderr)
        return None

    return found[0]


def load(extra_path=None):
    """Import the installer, optionally from a folder added to the path first."""
    if extra_path is not None and str(extra_path) not in sys.path:
        sys.path.insert(0, str(extra_path))

    try:
        return __import__(MODULE)
    except ImportError:
        return None


def install_with_pip():
    """
    Last resort: let pip do it, which also pulls in anything else that is needed.

    The installer is written to need nothing but Python, so this should never be
    reached. It is here so that a missing package is something the script solves
    rather than something it asks you to go and fix.
    """
    print("Falling back to pip ...", flush=True)

    try:
        subprocess.check_call(
            [sys.executable, "-m", "pip", "install", "--quiet", "--upgrade", SOURCE]
        )
    except (subprocess.CalledProcessError, OSError):
        return None

    return load()


def main():
    staging = Path(tempfile.mkdtemp(prefix="stm32-install-"))

    try:
        print("Fetching the installer ...", flush=True)

        installer = load(fetch_to(staging))

        if installer is None:
            # An already installed copy, for a machine that is offline but has
            # had the installer put there some other way.
            installer = load()

        if installer is None:
            installer = install_with_pip()

        if installer is None:
            print(
                "\nCould not get the installer.\n"
                "Check that this machine can reach github.com and try again.",
                file=sys.stderr,
            )
            return 2

        return installer.main(library_root=Path(__file__).resolve().parent)
    finally:
        shutil.rmtree(staging, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
