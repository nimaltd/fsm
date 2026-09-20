#!/usr/bin/env python3
"""
Install a NimaLTD library into your STM32 project.

Run it from the root of your project. It works two ways, depending on whether
this file has a library sitting next to it.

    python fsm/install.py
        You downloaded this repository into your project, so the library is
        already here. Nothing is asked. The repository folder becomes a plain
        library folder: the header and source move to the top, your config file
        is created, and everything belonging to the repository rather than your
        firmware is removed.

        That last part matters. STM32CubeIDE compiles every .c file under your
        project, and this repository ships a test suite with its own main(),
        which would break your build.

    python install.py
        This file is on its own, downloaded from a library's repository, so it
        fetches that library from GitHub. You are asked which folder to put it
        in, and only the files the library actually needs are downloaded.
        Afterwards this file deletes itself.

        python install.py --ref 1.20.0      a released version, not the newest
        python install.py nimaltd/spif      a different library altogether

Your own <library>_config.h is never overwritten, so either form is also how you
update.

Nothing is installed on your machine. The installer itself is fetched into a
temporary folder, used, and deleted. No pip, no packages, no leftovers, and you
always get the current version because there is never an old one lying around.
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

# Which library this copy installs when it is downloaded on its own, away from
# its repository, and which branch to take it from.
#
# Written as "owner/name" rather than a bare name so that a fork under someone
# else's account works by changing this one line. A full GitHub URL works too.
# The copy in the stm32-installer repository leaves LIBRARY as None, because
# that one is not tied to any particular library.
LIBRARY = "nimaltd/fsm"
BRANCH = "master"

# Where the installer itself comes from. Anyone maintaining their own libraries
# with this tool points these at their own repositories and changes nothing else.
SOURCE = "https://github.com/nimaltd/stm32-installer/archive/refs/heads/main.zip"

MODULE = "stm32_installer"
MANIFEST = "library.yml"
TIMEOUT_SECONDS = 30

HERE = Path(__file__).resolve().parent


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
    reached. It is here so a missing package is something this solves rather
    than something it asks you to go and fix.
    """
    print("Falling back to pip ...", flush=True)

    try:
        subprocess.check_call(
            [sys.executable, "-m", "pip", "install", "--quiet", "--upgrade", SOURCE]
        )
    except (subprocess.CalledProcessError, OSError):
        return None

    return load()


def get_installer(staging):
    """The installer module, however it can be got hold of."""
    print("Fetching the installer ...", flush=True)

    installer = load(fetch_to(staging))

    if installer is None:
        # An already installed copy, for a machine that is offline but has had
        # the installer put there some other way.
        installer = load()

    if installer is None:
        installer = install_with_pip()

    return installer


# Options that swallow the argument after them. Without this, the "v2.0.0" in
# "--ref v2.0.0" reads as a library name and the wrong thing gets installed.
VALUE_OPTIONS = ("--ref", "--dir", "--project", "--local", "--ide")


def _library_names(argv):
    """The arguments that actually name a library, ignoring options and values."""
    names = []
    skip = False

    for arg in argv:
        if skip:
            skip = False
            continue

        if arg.startswith("-"):
            skip = arg in VALUE_OPTIONS
            continue

        names.append(arg)

    return names


def remove_self():
    """
    Delete this file once it has finished.

    Python reads the whole script before running it and closes the file, so this
    is safe even on Windows, where a file in use normally cannot be deleted.
    """
    try:
        Path(__file__).resolve().unlink()
        print(f"Removed {Path(__file__).name}, it has done its job.")
    except OSError:
        # Not worth failing an install that already succeeded.
        pass


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv

    # Options are not a library name. "--ref v2.0.0" still means "the library
    # this copy belongs to", just at a different version.
    named = _library_names(argv)
    beside_a_library = (HERE / MANIFEST).is_file()

    # Downloaded on its own, with no library named. This copy knows which one it
    # came from, which is what makes the one line command work.
    if not beside_a_library and not named and LIBRARY:
        argv = [LIBRARY] + argv

        # Only when the caller has not chosen a version of their own.
        if "--ref" not in argv:
            argv += ["--ref", BRANCH]

    if not beside_a_library and not argv:
        print(
            f"There is no {MANIFEST} next to this file, so there is no library here "
            "to install.\n"
            "Say which one you want, for example:\n\n"
            f"    python {Path(__file__).name} fsm\n",
            file=sys.stderr,
        )
        return 2

    staging = Path(tempfile.mkdtemp(prefix="stm32-install-"))

    try:
        installer = get_installer(staging)

        if installer is None:
            print(
                "\nCould not get the installer.\n"
                "Check that this machine can reach github.com and try again.",
                file=sys.stderr,
            )
            return 2

        if beside_a_library and not named:
            return installer.main(library_root=HERE)

        code = installer.main(argv=argv)

        # A copy downloaded on its own has done its job and would only be
        # clutter in the project from here on. One that was given a library to
        # install is being used as a tool, so it stays for the next one.
        if code == 0 and not named:
            remove_self()

        return code
    finally:
        shutil.rmtree(staging, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
