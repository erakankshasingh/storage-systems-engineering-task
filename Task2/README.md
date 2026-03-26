# Task 2 — Count Disk Models from a Compressed JSON File

## Overview

The input is a bzip2-compressed JSON file:

```text
bigf.json.bz2
```

The file contains disk descriptions, including at least:

- `model`
- `serial`

The program computes:

1. how many distinct disk models are present
2. how many times each model appears

## Approach

I implemented the solution in Python using:

- `bz2` to read the compressed `.bz2` file directly
- `ijson` to parse the JSON incrementally in streaming mode
- `Counter` to aggregate model frequencies

The key design choice is to avoid fully decompressing and loading the entire JSON into memory.

Instead, the program:

1. opens the `.bz2` file directly
2. parses the JSON token by token
3. tracks JSON objects while they are being read
4. for each object containing both `model` and `serial`, increments the count for that model

At the end:

- `len(counts)` gives the number of distinct models
- `counts[model]` gives how many times that model appears

## Why this approach

This solution is a good fit because it is:

- streaming
- memory-efficient
- simple and robust for large inputs
- well suited for compressed structured data

A full `json.load()` solution would first require fully decompressing and loading the JSON, which is less attractive for large files.

## Dependencies

Python 3 and:

```bash
pip install ijson
```

The `bz2` module is part of the Python standard library.

## Run

```bash
python3 Storpool_Task2.py
```

The script expects the input file to be named:

```text
bigf.json.bz2
```

and to be present in the current working directory.

## Output format

The program prints:

- the number of distinct models
- then each model and its count

Example:

```text
number_of_models=2
Samsung    2
Seagate    1
```

## Testing

Before running on the full input, I verified correctness using a small handcrafted JSON test file with known expected results.

### Test workflow

1. I created a tiny JSON file containing a few disk objects with `model` and `serial`.
2. I compressed it to `.bz2`.
3. I renamed it to:

```text
bigf.json.bz2
```

so that the program could be tested without changing the code.
4. I ran the script and checked that the output matched the expected counts.

This allowed me to validate the logic on a controlled dataset before running it on the large input.

### Example tiny test data

```json
[
  {"model": "Samsung", "serial": "A1"},
  {"model": "Samsung", "serial": "A2"},
  {"model": "Seagate", "serial": "B1"}
]
```

After compressing this file to `.bz2` and naming it `bigf.json.bz2`, the expected output is:

```text
number_of_models=2
Samsung    2
Seagate    1
```

## Notes

The implementation counts objects that contain both:

- `model`
- `serial`

This is used as the criterion for identifying disk entries while parsing the JSON stream.

The output is printed in sorted model order to make results deterministic and easy to read.
