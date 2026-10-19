#!/usr/bin/env python3
"""Generate the offline Buy Me a Coffee QR image without third-party packages."""
import argparse
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
URL = 'https://buymeacoffee.com/depius'
OUTPUT = ROOT / 'assets' / 'bmc-qr.bmp'
VERSION = 3
SIZE = 17 + 4 * VERSION
DATA_CODEWORDS = 55
ECC_CODEWORDS = 15
MODULE_SCALE = 11
QUIET_ZONE = 4


def append_bits(bits, value, count):
    bits.extend((value >> i) & 1 for i in range(count - 1, -1, -1))


def gf_multiply(x, y):
    z = 0
    for _ in range(8):
        if y & 1:
            z ^= x
        y >>= 1
        x <<= 1
        if x & 0x100:
            x ^= 0x11D
    return z


def rs_divisor(degree):
    result = [0] * degree
    result[-1] = 1
    root = 1
    for _ in range(degree):
        for j in range(degree):
            result[j] = gf_multiply(result[j], root)
            if j + 1 < degree:
                result[j] ^= result[j + 1]
        root = gf_multiply(root, 2)
    return result


def rs_remainder(data, divisor):
    result = [0] * len(divisor)
    for value in data:
        factor = value ^ result[0]
        result = result[1:] + [0]
        for i, coefficient in enumerate(divisor):
            result[i] ^= gf_multiply(coefficient, factor)
    return result


def codewords(text):
    payload = text.encode('utf-8')
    if len(payload) > 53:
        raise ValueError('URL exceeds the selected QR version capacity')
    bits = []
    append_bits(bits, 0b0100, 4)  # Byte mode
    append_bits(bits, len(payload), 8)
    for value in payload:
        append_bits(bits, value, 8)
    capacity = DATA_CODEWORDS * 8
    bits.extend([0] * min(4, capacity - len(bits)))
    bits.extend([0] * ((-len(bits)) % 8))
    data = [sum(bits[i + j] << (7 - j) for j in range(8)) for i in range(0, len(bits), 8)]
    pad_index = 0
    while len(data) < DATA_CODEWORDS:
        data.append((0xEC, 0x11)[pad_index % 2])
        pad_index += 1
    return data + rs_remainder(data, rs_divisor(ECC_CODEWORDS))


def mask_bit(mask, x, y):
    if mask == 0: return (x + y) % 2 == 0
    if mask == 1: return y % 2 == 0
    if mask == 2: return x % 3 == 0
    if mask == 3: return (x + y) % 3 == 0
    if mask == 4: return (y // 2 + x // 3) % 2 == 0
    if mask == 5: return (x * y) % 2 + (x * y) % 3 == 0
    if mask == 6: return ((x * y) % 2 + (x * y) % 3) % 2 == 0
    return ((x + y) % 2 + (x * y) % 3) % 2 == 0


def format_bits(mask):
    value = (0b01 << 3) | mask  # Error correction level L
    rem = value << 10
    for i in range(14, 9, -1):
        if (rem >> i) & 1:
            rem ^= 0x537 << (i - 10)
    return ((value << 10) | rem) ^ 0x5412


def make_matrix(text, mask):
    modules = [[False] * SIZE for _ in range(SIZE)]
    function = [[False] * SIZE for _ in range(SIZE)]

    def set_function(x, y, dark):
        if 0 <= x < SIZE and 0 <= y < SIZE:
            modules[y][x] = bool(dark)
            function[y][x] = True

    def finder(cx, cy):
        for dy in range(-1, 8):
            for dx in range(-1, 8):
                x, y = cx + dx, cy + dy
                dark = 0 <= dx <= 6 and 0 <= dy <= 6 and (
                    dx in (0, 6) or dy in (0, 6) or (2 <= dx <= 4 and 2 <= dy <= 4))
                set_function(x, y, dark)

    finder(0, 0)
    finder(SIZE - 7, 0)
    finder(0, SIZE - 7)
    for i in range(8, SIZE - 8):
        set_function(6, i, i % 2 == 0)
        set_function(i, 6, i % 2 == 0)
    for dy in range(-2, 3):
        for dx in range(-2, 3):
            set_function(22 + dx, 22 + dy, max(abs(dx), abs(dy)) != 1)

    bits = format_bits(mask)
    for i in range(6): set_function(8, i, (bits >> i) & 1)
    set_function(8, 7, (bits >> 6) & 1)
    set_function(8, 8, (bits >> 7) & 1)
    set_function(7, 8, (bits >> 8) & 1)
    for i in range(9, 15): set_function(14 - i, 8, (bits >> i) & 1)
    for i in range(8): set_function(SIZE - 1 - i, 8, (bits >> i) & 1)
    for i in range(8, 15): set_function(8, SIZE - 15 + i, (bits >> i) & 1)
    set_function(8, SIZE - 8, True)

    words = codewords(text)
    bit_index = 0
    upward = True
    right = SIZE - 1
    while right >= 1:
        if right == 6:
            right -= 1
        for vert in range(SIZE):
            y = SIZE - 1 - vert if upward else vert
            for offset in range(2):
                x = right - offset
                if not function[y][x]:
                    value = ((words[bit_index >> 3] >> (7 - (bit_index & 7))) & 1) if bit_index < len(words) * 8 else 0
                    modules[y][x] = bool(value ^ mask_bit(mask, x, y))
                    bit_index += 1
        upward = not upward
        right -= 2

    return modules


def penalty(matrix):
    score = 0
    n = len(matrix)
    for lines in (matrix, list(map(list, zip(*matrix)))):
        for row in lines:
            run_color, run_length = row[0], 1
            for cell in row[1:]:
                if cell == run_color:
                    run_length += 1
                    if run_length == 5: score += 3
                    elif run_length > 5: score += 1
                else:
                    run_color, run_length = cell, 1
            for i in range(n - 10):
                pattern = row[i:i + 11]
                if pattern == [0,0,0,0,1,0,1,1,1,0,1] or pattern == [1,0,1,1,1,0,1,0,0,0,0]:
                    score += 40
    for y in range(n - 1):
        for x in range(n - 1):
            value = matrix[y][x]
            if matrix[y][x + 1] == value and matrix[y + 1][x] == value and matrix[y + 1][x + 1] == value:
                score += 3
    dark = sum(sum(row) for row in matrix)
    score += (abs(dark * 20 - n * n * 10) // (n * n)) * 10
    return score


def qr_matrix(text=URL):
    candidates = [make_matrix(text, mask) for mask in range(8)]
    return min(candidates, key=penalty)


def bmp_bytes(text=URL):
    matrix = qr_matrix(text)
    modules = len(matrix) + 2 * QUIET_ZONE
    width = modules * MODULE_SCALE
    stride = (width * 3 + 3) & ~3
    pixels = bytearray()
    for y in range(width - 1, -1, -1):
        my = y // MODULE_SCALE - QUIET_ZONE
        row = bytearray()
        for x in range(width):
            mx = x // MODULE_SCALE - QUIET_ZONE
            dark = 0 <= mx < len(matrix) and 0 <= my < len(matrix) and matrix[my][mx]
            row.extend((0, 0, 0) if dark else (255, 255, 255))
        row.extend(b'\0' * (stride - len(row)))
        pixels.extend(row)
    header = struct.pack('<2sIHHI', b'BM', 54 + len(pixels), 0, 0, 54)
    info = struct.pack('<IiiHHIIiiII', 40, width, width, 1, 24, 0, len(pixels), 2835, 2835, 0, 0)
    return header + info + pixels


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true', help='fail unless the committed QR exactly matches the URL generator')
    args = parser.parse_args()
    expected = bmp_bytes(URL)
    if args.check:
        if not OUTPUT.is_file() or OUTPUT.read_bytes() != expected:
            raise SystemExit('Donation QR asset is missing or does not match the exact configured URL')
        print(f'PASS: QR asset deterministically encodes {URL}')
    else:
        OUTPUT.parent.mkdir(parents=True, exist_ok=True)
        OUTPUT.write_bytes(expected)
        print(f'Generated {OUTPUT} for {URL}')


if __name__ == '__main__':
    main()
