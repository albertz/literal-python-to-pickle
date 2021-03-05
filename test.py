#!/usr/bin/env python3

import better_exchook
from subprocess import check_call, CalledProcessError
import ast
import tempfile
import os
import sys
import pickle


_CppFilename = "py-to-pickle.cpp"
_BinFilename = "py-to-pickle.bin"


def cpp_compile():
    check_call(["c++", "-std=c++11", _CppFilename, "-o", _BinFilename])


def py_to_pickle(s: str):
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


def assert_equal(a, b, _msg=""):
    assert type(a) == type(b)
    if isinstance(a, dict):
        assert len(a) == len(b)
        for key, a_value in a:
            b_value = b[key]
            assert_equal(a_value, b_value, _msg + f"[{key}]")
    elif isinstance(a, (list, tuple)):
        assert len(a) == len(b)
        for i, (a_value, b_value) in enumerate(zip(a, b)):
            assert_equal(a_value, b_value, _msg + f"[{i}]")
    else:
        assert a == b, f"{a} != {b} in {_msg}"


def check(s: str):
    print("Check:", s)
    a = eval(s)
    b = ast.literal_eval(s)
    assert_equal(a, b)

    c_bin = py_to_pickle(s)
    c = pickle.loads(c_bin)
    assert_equal(b, c)


def tests():
    checks = [
        "0", "1",
        "0.0", "1.23",
        '""', '"abc"', "''", "'abc'",
        "[]", "[1]", "[1,2,3]",
        "{}", "{'a': 'b', 1: 2}",
        "{1}", "{1,2,3}",
    ]
    for s in checks:
        check(s)


def main():
    cpp_compile()
    tests()
    print("All passed!")


if __name__ == "__main__":
    better_exchook.install()
    main()

