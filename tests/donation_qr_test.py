#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location('donation_qr', ROOT / 'tools/generate-donation-qr.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

assert module.URL == 'https://buymeacoffee.com/depius'
matrix = module.qr_matrix()
assert len(matrix) == 29 and all(len(row) == 29 for row in matrix)
asset = (ROOT / 'assets/bmc-qr.bmp').read_bytes()
assert asset == module.bmp_bytes(module.URL)
assert asset[:2] == b'BM'
assert struct.unpack_from('<ii', asset, 18) == (407, 407)
assert asset[54:54 + 44 * 3] == b'\xff' * (44 * 3)
print('PASS: committed QR bitmap is reproducibly generated for the confirmed donation URL with a four-module quiet zone')
