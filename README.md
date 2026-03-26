# Core Systems Engineering Assignment

This repository contains solutions and analysis notes for a systems engineering assignment covering low-level data processing, compressed JSON analysis, and reverse engineering of Linux executables.

The work is organized into three tasks and focuses on correctness, resource-aware design, and clear technical reasoning.

## Tasks

### Task 1 — Counting `uint32_t` values in a binary file
Implemented an exact counting solution for a large binary file containing 1 billion `uint32_t` values.

The program computes:
- the number of unique values
- the number of values seen exactly once

The solution uses a fixed-domain bitset approach instead of hashing, which makes memory usage predictable and keeps the algorithm efficient for a full `uint32_t` key space.

### Task 2 — Analysis of a compressed JSON file
Implemented a Python solution to analyze a compressed `.json.bz2` file containing disk information.

The program:
- reads the compressed file directly
- parses the JSON in streaming mode
- counts how many distinct disk models exist
- counts how many times each model appears

The solution avoids loading the full decompressed JSON into memory.

### Task 3 — Reverse engineering notes
Analyzed two provided Linux ELF executables that crash in different ways.

Because the initial inspection environment was macOS, the analysis focused first on:
- platform-related execution errors on macOS
- static inspection using tools such as `file` and `strings -a`
- identifying likely intent and likely failure areas conservatively

The notes separate:
- direct observations
- reasonable inferences
- what would require Linux runtime debugging for exact confirmation

## Repository structure

Each task includes its own implementation and/or task-specific notes.
Suggested structure:

- task1/
- task2/
- task3/
- README.md

## Technologies used

C for low-level binary-data processing
Python for compressed JSON analysis
basic reverse-engineering tooling and static binary inspection

## Notes

This repository is intentionally documentation-friendly:
task-specific explanations are kept close to each task 
the top-level README gives a quick overview
detailed reasoning is included where useful

## Author

Akanksha Singh
