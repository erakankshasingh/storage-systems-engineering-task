# Task 1 — Counting `uint32_t` Values in a Binary File

## Overview

The input is a binary file containing **1,000,000,000** values of type `uint32_t`  
(total size: **4,000,000,000 bytes**).

The program computes:

1. the number of unique values
2. the number of values that occur exactly once

### Examples

- `0x100 0x100 0xfff 0xfff` → unique = `2`, seen only once = `0`
- `0x100 0x100 0x100 0x100` → unique = `1`, seen only once = `0`
- `0x100 0x100 0x800 0xfff` → unique = `3`, seen only once = `2`

## Design

Since the input values are `uint32_t`, the key space is fixed: **2^32** possible values.

Instead of using a hash table, the solution keeps a compact state for each possible value using two bitsets:

- `once[x]` — value `x` has been seen exactly once
- `twice[x]` — value `x` has been seen two or more times

This gives a simple 3-state model per value:

- unseen
- seen once
- seen multiple times

### State transitions

For each input value `x`:

- if `x` is in neither bitset: set `once[x]`
- if `x` is in `once`: clear `once[x]`, set `twice[x]`
- if `x` is already in `twice`: do nothing

After the full scan:

- `seen_only_once = popcount(once)`
- `unique = popcount(once) + popcount(twice)`

## Why this approach

This is a good fit for the problem because it is:

- exact
- single-pass
- sequential-I/O friendly
- predictable in memory usage
- free from the overhead and cache-unfriendly behavior of a massive hash table

For this specific domain, the fixed `uint32_t` key space makes a bitset-based solution more appropriate than either hashing or sorting.

## Memory usage

Each bitset stores one bit for every possible `uint32_t` value:

- `2^32` bits = `512 MiB`

Two bitsets:

- `once` = `512 MiB`
- `twice` = `512 MiB`

Total working memory:

- about **1 GiB**
- plus a small read buffer

This is large, but still bounded and straightforward.

## Complexity

### Time

- one pass over the input: `O(n)`
- one pass to count set bits in the two bitsets: `O(2^32 / 64)`

### Space

- **1 GiB + read buffer**

## Input format

The input is a raw binary stream of `uint32_t` values.

Requirements:

- file size must be a multiple of 4 bytes
- values are interpreted as 32-bit unsigned integers
- endianness must match the format used to write the file, or be converted explicitly

## Build

```bash
gcc -O3 -march=native -Wall -Wextra -std=c11 Storpool_Task1.c -o Storpool_Task1
```

## Run

### Using a file path

```bash
./Storpool_Task1 sample.bin
```

### If stdin is supported by the implementation

```bash
./Storpool_Task1 < sample.bin
./Storpool_Task1 < sample2.bin
./Storpool_Task1 < sample3.bin
```

## Local testing

A small binary test file can be generated with Python:

```python
import struct

vals = [0x100, 0x100, 0x800, 0xfff]

with open("sample.bin", "wb") as f:
    for v in vals:
        f.write(struct.pack("<I", v))
```

Run:

```bash
python3 Storpool_Task1_Test1.py     (sample.bin creaed)
python3 Storpool_Task1_Test2.py.    (sample2.bin creaed)
python3 Storpool_Task1_Test3.py.    (sample3.bin creaed)
./Storpool_Task1 sample.bin
./Storpool_Task1 sample2.bin
./Storpool_Task1 sample3.bin
```

Expected output:

```text
unique=3
seen_only_once=2
```

## Additional test cases

### Case 1
```bash
python3 Storpool_Task1_Test1.py     (sample.bin creaed)
./Storpool_Task1 sample.bin
```
Input:

```text
0x100 0x100 0xfff 0xfff
```

Expected:

```text
unique=2
seen_only_once=0
```

### Case 2
```bash
python3 Storpool_Task1_Test2.py.    (sample2.bin creaed)
./Storpool_Task1 sample2.bin
```
Input:

```text
0x100 0x100 0x100 0x100
```

Expected:

```text
unique=1
seen_only_once=0
```

### Case 3
```bash
python3 Storpool_Task1_Test3.py.    (sample3.bin creaed)
./Storpool_Task1 sample3.bin
```
Input:

```text
0x100 0x100 0x800 0xfff
```

Expected:

```text
unique=3
seen_only_once=2
```

## Notes

Other exact approaches are possible, but less attractive here:

- **hash table**  
  higher memory overhead, worse locality, less predictable behavior at this scale

- **sorting**  
  more data movement, more I/O, and unnecessary for a fixed 32-bit key space

Given the problem constraints, the two-bitset solution is simple, deterministic, and efficient.
