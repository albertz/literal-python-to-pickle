#!/usr/bin/env python3

import gzip
import timeit
import ast
import time
import better_exchook
import json
import pickle
import marshal
from test import py_to_pickle
import argparse

def main():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument("--only-py-to-pickle", action="store_true")
    args = arg_parser.parse_args()

    txt_fn_gz = "demo.txt.gz"  # use the generate script

    t = time.time()
    txt = gzip.open(txt_fn_gz, "rb").read().decode("utf8")
    print("Gunzip + read time:", time.time() - t)
    print("Size:", len(txt))

    print("py_to_pickle:", timeit.timeit(lambda: py_to_pickle(txt), number=1))
    print("pickle.loads+py_to_pickle:", timeit.timeit(lambda: pickle.loads(py_to_pickle(txt)), number=1))

    if args.only_py_to_pickle:
        return

    print("compile:", timeit.timeit(lambda: compile(txt, "<>", "exec"), number=1))
    print("parse:", timeit.timeit(lambda: compile(txt, "<>", "exec", ast.PyCF_ONLY_AST), number=1))
    print("eval:", timeit.timeit(lambda: eval(txt), number=1))
    print("ast.literal_eval:", timeit.timeit(lambda: ast.literal_eval(txt), number=1))

    content = eval(txt)
    js = json.dumps(content)
    pk = pickle.dumps(content, protocol=3)
    m = marshal.dumps(content)

    print("json.loads:", timeit.timeit(lambda: json.loads(js), number=1))
    print("pickle.loads:", timeit.timeit(lambda: pickle.loads(pk), number=1))
    print("marshal.loads:", timeit.timeit(lambda: marshal.loads(m), number=1))


if __name__ == "__main__":
    better_exchook.install()
    main()
