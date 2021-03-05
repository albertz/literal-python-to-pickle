# Convert literal Python to Pickle

## Why

`ast.literal_eval` is very slow.
`json.loads` shows that it can be much faster.
`pickle.loads` is yet faster.

## Usage

Can be compiled as a standalone executable (mostly for testing), or as a library.

Library compilation:

    c++ -std=c++11 -DLIB py-to-pickle.cpp -shared -fPIC -o libpytopickle.so

Then you can use `common.py_to_pickle(s: bytes) -> bytes`.

In your code, if you have `eval(s)`, replace it by `pickle.loads(py_to_pickle(s))`

## Statistics

Run `./time-read.py`:

```
Gunzip + read time: 0.1663219928741455
Size: 22540270
py_to_pickle: 0.539439306
pickle.loads+py_to_pickle: 0.7234611099999999
compile: 3.3440755870000003
parse: 3.6302585899999995
eval: 3.306765757000001
ast.literal_eval: 4.056752016000003
json.loads: 0.3230752619999997
pickle.loads: 0.1351051709999993
marshal.loads: 0.10351717500000035
```

So our implementation is >6x faster than `eval`.

## Properties

We consider objects which can be serialized with `repr`
and reconstructed via `ast.literal_eval`.
I.e. it uses only the basic types: `str`, `bool`, `int`, `float`, `list`, `tuple`, `dict`, `set`.
I.e. we assume for such an object `obj`:

    obj == ast.literal_eval(repr(obj))

We guarantee:

    obj == pickle.loads(py_to_pickle(repr(obj)))

(At least that is the intention. If this is not the case, consider this a bug.)
(At the time of writing, it is slightly incomplete. E.g. `bool` and `tuple` not supported yet.)

This is not intended to 100% cover all valid Python code
(but all code generated by `repr`),
i.e. maybe might not hold for some bytes `s`
(although we are not aware of such cases):

    ast.literal_eval(s) == pickle.loads(py_to_pickle(s))

E.g. we do not support these:

* Joined strings `"abc" "def"`
* String prefixes, e.g. `r"abc"`
* `bytes` currently not supported
