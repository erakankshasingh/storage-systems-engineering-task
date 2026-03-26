# Task 3 — Reverse Engineering Notes

## Overview

The archive contains two executable files:

- `a`
- `b`

The task is to explain:

1. why they crash
2. what they appear to be trying to do

This README documents a **macOS-based analysis**. Since the binaries are not native macOS executables, the analysis here focuses on:

- the errors encountered on macOS
- the commands used to inspect the files
- the exact evidence lines found through static inspection
- the conclusions that can be made conservatively from that evidence

---

## Initial errors encountered on macOS

Trying to execute the binaries directly on macOS produced:

```text
zsh: exec format error: ./a
```

and similarly for `b`.

Trying to start GDB produced:

```text
zsh: command not found: gdb
```

### Interpretation

The first error indicates that the binaries are **not native macOS executables**.

To confirm that, I used:

```bash
file a
file b
```

This showed that both files are **Linux ELF x86-64 executables**, not Mach-O binaries.

So the `exec format error` on macOS is a **platform mismatch**, not yet the internal bug of the program itself.

The second error means `gdb` was not installed locally on macOS. Even if installed, macOS still would not execute these Linux ELF binaries natively.

---

## Static analysis approach used on macOS

Since the binaries could not be run directly on macOS, I used static inspection.

### Commands used

```bash
file a
file b
strings -a a | less
strings -a b | less
```

### Why `strings -a`

A plain `strings` run mostly returned generic runtime / libc / loader text, which was noisy and not very informative.

Using `strings -a` was more useful because it scans the full binary and exposes application-specific strings more reliably.


## Analysis of binary `a`

### Commands used

```bash
file a
strings -a a | less
strings -a a | grep -n '^\./'
strings -a a | grep -n 'pesho'
```

### Relevant evidence

The key application-specific line found was:

```text
./pesho
```

### Interpretation

`./pesho` is a relative path in the current working directory. Unlike generic runtime strings such as `/dev/null` or `/usr/share/locale`, it looks application-specific.

This strongly suggests that binary `a` expects a local file named `pesho`.

### Conservative conclusion for `a`

From macOS static analysis alone, the safest conclusion is:

- binary `a` appears to depend on a local file `./pesho`
- the crash is likely related to broken file-handling logic around that file
- a missing or unreadable `./pesho` is therefore a plausible trigger


## Analysis of binary `b`

### Commands used

```bash
file b
strings -a b | less
strings -a b | grep -n 'TMPDIR\|Init done %d \.\|Enter password: \|WRONG\|OK\.'
```

### Relevant evidence

The important strings found were:

```text
TMPDIR
Init done %d .
Enter password:
WRONG
OK.
```

### Interpretation

These strings strongly suggest the following structure:

1. an initialization step involving the `TMPDIR` environment variable
2. a password prompt
3. a password validation path with success / failure messages

### Conservative conclusion for `b`

From macOS static analysis alone, the safest conclusion is:

- binary `b` appears to use the `TMPDIR` environment variable during initialization
- binary `b` then appears to prompt the user for a password
- the crash likely happens early, during initialization or environment handling, before or around the password-check path


## Summary

### Errors directly encountered on macOS

1. `exec format error` when trying to run `./a` or `./b`
2. `gdb: command not found`

### What these errors mean

- the files are Linux ELF executables and cannot run natively on macOS
- therefore macOS allowed only static inspection, not direct runtime debugging

### Most useful commands

```bash
file a
file b
strings -a a | grep -n '^\./'
strings -a a | grep -n 'pesho'
strings -a b | grep -n 'TMPDIR\|Init done %d \.\|Enter password: \|WRONG\|OK\.'
```

### Most useful evidence lines

- for `a`: `./pesho`
- for `b`: `TMPDIR`, `Enter password:`, `WRONG`, `OK.`


## Final note

This macOS-based analysis is enough to identify likely intent and likely failure areas, but it is not enough to prove the exact crashing instruction.

Exact root-cause confirmation would require running the binaries in a **Linux x86-64 environment** and then using tools such as:

- `gdb`
- `strace`
- `objdump`
- `readelf`

