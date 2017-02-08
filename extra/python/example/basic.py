#!/usr/bin/env python

import immer

v0 = immer.Vector().append(13).append(42)
assert v0[0] == 13 and v0[1] == 42 and len(v0) == 2

v1 = v0.set(0, 12)
assert v0[0] == 13 and v0[1] == 42 and len(v0) == 2
assert v1[0] == 12 and v1[1] == 42 and len(v1) == 2
