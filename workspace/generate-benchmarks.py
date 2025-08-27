import itertools
import os

modes = ["aes256", "camellia", "chacha20", "twofish"]
dims0 = [134217728, 134217728 * 2, 134217728 * 4, 134217728 * 8, 134217728 * 16] # 1, 2, 4, 8, 16 GiB

output_dir = "configs"

for mode, dim0 in itertools.product(modes, dims0):
    filename = f"{mode}-{dim0:010d}"
    filepath = os.path.join(output_dir, filename)

    with open(filepath, "w") as f:
        f.write(f"mode={mode}\n")
        f.write(f"count={dim0}\n")
