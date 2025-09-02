#!/bin/python3

import itertools
import os

libs = ["gcrypt", "nettle"]
# modes = ["aes256", "camellia", "chacha20", "twofish"]
modes = ["aes256", "chacha20"]
dims = [134217728, 134217728 * 2, 134217728 * 4, 134217728 * 8, 134217728 * 16] # 1, 2, 4, 8, 16 GiB

output_dir = "configs"

for lib, mode, dim in itertools.product(libs, modes, dims):
    filename = f"{lib}-{mode}-{dim:010d}"
    filepath = os.path.join(output_dir, filename)

    with open(filepath, "w") as f:
        f.write(f"lib={lib}\n")
        f.write(f"mode={mode}\n")
        f.write(f"count={dim}\n")
