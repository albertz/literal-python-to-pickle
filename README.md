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
