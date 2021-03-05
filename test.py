#!/usr/bin/env python3

import better_exchook
from subprocess import check_call
import ast
import tempfile
import os
import pickle


_CppFilename = "py-to-pickle.cpp"
_BinFilename = "py-to-pickle.bin"


def cpp_compile():
    check_call(["c++", _CppFilename, "-o", _BinFilename])


def py_to_pickle(s: str):
    with tempfile.NamedTemporaryFile("w", suffix=".py") as f_py:
        f_py.write(s)
        f_py.write("\n")
        f_py.flush()

        with tempfile.NamedTemporaryFile("w", suffix=".pkl", prefix=os.path.basename(f_py.name)) as f_pkl:
            check_call(["./" + _BinFilename, f_py.name, f_pkl.name])
            return open(f_pkl.name, "rb").read()


def assert_equal(a, b, _msg=""):
    assert type(a) == type(b)
    if isinstance(a, dict):
        assert len(a) == len(b)
        for key, a_value in a:
            b_value = b[key]
            assert_equal(a_value, b_value, _msg + f"[{key}]")
    elif isinstance(a, (list, tuple)):
        assert len(a) == len(b)
        for i, (a_value, b_value) in enumerate(a, zip(a, b)):
            assert_equal(a_value, b_value, _msg + f"[{i}]")
    else:
        assert a == b, f"{a} != {b} in {_msg}"


def check(s: str):
    a = eval(s)
    b = ast.literal_eval(s)
    assert_equal(a, b)

    c_bin = py_to_pickle(s)
    c = pickle.loads(c_bin)
    assert_equal(b, c)


def test_int0():
    check("0")


def test_int1():
    check("1")


def test_float0():
    check("0.0")


def test_float123():
    check("1.23")


def main():
    cpp_compile()

    for key, func in list(globals().items()):
        if key.startswith("test_") and callable(func):
            print(f"** {key}()")
            func()
            print()
    print("All passed!")


if __name__ == "__main__":
    better_exchook.install()
    main()

