
import tempfile
import os
import sys
from subprocess import check_call, CalledProcessError
import ctypes


_CppFilename = "py-to-pickle.cpp"
_BinFilename = "py-to-pickle.bin"


def cpp_compile():
    check_call(["c++", "-std=c++11", _CppFilename, "-o", _BinFilename])


def py_to_pickle_tmp(s: str) -> bytes:
    with tempfile.NamedTemporaryFile("w", suffix=".py") as f_py:
        f_py.write(s)
        f_py.write("\n")
        f_py.flush()

        with tempfile.NamedTemporaryFile("w", suffix=".pkl", prefix=os.path.basename(f_py.name)) as f_pkl:
            try:
                check_call(["./" + _BinFilename, f_py.name, f_pkl.name])
            except CalledProcessError as exc:
                print(f"{_BinFilename} returned error code {exc.returncode}")
                sys.exit(1)
            return open(f_pkl.name, "rb").read()


def py_to_pickle(s: str) -> bytes:
    # len(s) should always be enough
    buf = ctypes.create_string_buffer(len(s))


py_to_pickle = py_to_pickle_tmp  # until we have implemented the other version...
