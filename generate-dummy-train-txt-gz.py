#!/usr/bin/env python3

import better_exchook
import gzip
import random

better_exchook.install()
rnd = random.Random(42)
chars = "ABC XYZ abc def ghi jkl '\" 012345.,"

with gzip.open("demo.txt.gz", "w") as f:
  f.write(b"[\n")
  for i in range(100000):
    txt_len = rnd.randint(5, 200)
    d = {
      "seq_index": i, "seq_name": "seq-%i" % i, 'file': "seq-%i.flac.ogg" % i,
      "text": "".join(rnd.choice(chars) for _ in range(txt_len)),
      "duration": rnd.uniform(1, 60)}
    f.write(b"%r,\n" % (d,))
  f.write(b"]\n")
