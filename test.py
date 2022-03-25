#!/usr/bin/env python3

import better_exchook
from subprocess import check_call, CalledProcessError
import ast
import os
import pickle
import gzip
import argparse
from typing import Union
from common import *


def assert_equal(a, b, _msg=""):
    assert type(a) == type(b)
    if isinstance(a, dict):
        assert len(a) == len(b)
        for key, a_value in a.items():
            b_value = b[key]
            assert_equal(a_value, b_value, _msg + f"[{key}]")
    elif isinstance(a, (list, tuple)):
        assert len(a) == len(b)
        for i, (a_value, b_value) in enumerate(zip(a, b)):
            assert_equal(a_value, b_value, _msg + f"[{i}]")
    else:
        assert a == b, f"{a!r} != {b!r} in {_msg}"


def check(s: Union[str, bytes]):
    print("Check:", repr(s) if len(s) <= 80 else repr(s[:70]) + "...")
    a = eval(s)
    b = ast.literal_eval(s if isinstance(s, str) else s.decode("utf8"))
    assert_equal(a, b)

    c_bin = py_to_pickle(s)
    c_bin2 = py_to_pickle_tmp(s)
    c_bin = c_bin[:len(c_bin2)]
    assert_equal(c_bin, c_bin2)

    c = pickle.loads(c_bin)
    assert_equal(b, c)


def tests(*, fast=True):
    checks = [
        "0", "1",
        "0.0", "1.23",
        '""', '"abc"', "''", "'abc'",
        '"abc\\n\\x00\\x01\\"\'abc"',
        "'hällö'",
        "'k\\u200eý'",
        "[]", "[1]", "[1,2,3]",
        "{}", "{'a': 'b', 1: 2}",
        "{1}", "{1,2,3}",
    ]
    for s in checks:
        check(s)

    if fast:
        return

    txt_fn_gz = "demo.txt.gz"  # use the generate script
    if os.path.exists(txt_fn_gz):
        txt = gzip.open(txt_fn_gz, "rb").read()
        check(txt)
    else:
        print(f"({txt_fn_gz} does not exist)")


def main():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument("--fast", action="store_true")
    args = arg_parser.parse_args()
    cpp_compile()
    tests(fast=args.fast)
    print("All passed!")


if __name__ == "__main__":
    better_exchook.install()
    main()

