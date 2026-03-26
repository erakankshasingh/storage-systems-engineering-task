#define _FILE_OFFSET_BITS 64   // allow correct handling of large files

#include <errno.h>      // errno
#include <fcntl.h>      // open(), O_RDONLY
#include <inttypes.h>   // PRIu64, PRIdMAX
#include <stdint.h>     // uint32_t, uint64_t
#include <stdio.h>      // printf(), fprintf()
#include <stdlib.h>     // malloc(), calloc(), free(), exit()
#include <string.h>     // strerror(), memmove()
#include <sys/stat.h>   // fstat(), struct stat
#include <sys/types.h>
#include <unistd.h>     // read(), close()

// Size of the read buffer: 16 MiB
#ifndef BUFFER_BYTES
#define BUFFER_BYTES (16u * 1024u * 1024u)
#endif

/*
 We need 1 bit for every possible uint32_t value.

 Number of possible uint32_t values = 2^32
 Each uint64_t word has 64 bits = 2^6 bits

 So number of uint64_t words needed:
 2^32 / 2^6 = 2^26
*/
#define BITSET_WORDS ((size_t)1 << 26)

/*
 Allocate one bitset and initialize all bits to 0.

 calloc() is used instead of malloc() because calloc fills memory with zeros.
 At the beginning, no value has been seen yet, so all bits should be 0.
*/
static uint64_t *alloc_bitset(void) {
    uint64_t *p = (uint64_t *)calloc(BITSET_WORDS, sizeof(uint64_t));
    if (!p) {
        fprintf(stderr, "calloc failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return p;
}

/*
 Process one input value x.

 We maintain two bitsets:
 - once[x]  = x has been seen exactly once
 - twice[x] = x has been seen two or more times

 State changes:
 - unseen      -> once
 - once        -> twice
 - twice       -> stays in twice
*/
static inline void process_value(uint64_t *once, uint64_t *twice, uint32_t x) {
    // Find which 64-bit word contains the bit for x.
    // x / 64 is same as x >> 6
    size_t idx = (size_t)(x >> 6);

    // Find which bit inside that 64-bit word belongs to x.
    // x % 64 is same as x & 63
    uint64_t mask = 1ULL << (x & 63);

    // If x is already marked as seen 2+ times, nothing to do.
    if (twice[idx] & mask) {
        return;
    }

    // If x is in 'once', then this is the second occurrence.
    if (once[idx] & mask) {
        once[idx] &= ~mask;   // remove it from 'once'
        twice[idx] |= mask;   // add it to 'twice'
    } else {
        // Otherwise this is the first time we see x.
        once[idx] |= mask;
    }
}

/*
 Count how many bits are set to 1 in the whole bitset.

 __builtin_popcountll(word) counts the number of 1-bits in one uint64_t.
*/
static uint64_t popcount_bitset(const uint64_t *bits) {
    uint64_t total = 0;

    for (size_t i = 0; i < BITSET_WORDS; ++i) {
        total += (uint64_t)__builtin_popcountll(bits[i]);
    }

    return total;
}

int main(int argc, char **argv) {
    int fd = -1;        // file descriptor for input
    int close_fd = 0;   // remember whether we should close it later

    /*
     Input handling:
     - if a filename is provided: read from that file
     - if no filename is provided: read from stdin
    */
    if (argc == 2) {
        fd = open(argv[1], O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "open(%s) failed: %s\n", argv[1], strerror(errno));
            return EXIT_FAILURE;
        }
        close_fd = 1;   // we opened the file, so we should close it later
    } else if (argc == 1) {
        fd = STDIN_FILENO;   // read from stdin
    } else {
        fprintf(stderr, "Usage: %s [input_file]\n", argv[0]);
        return EXIT_FAILURE;
    }

    /*
     If possible, check that the input size is a multiple of 4 bytes.
     Why? Because each uint32_t value uses exactly 4 bytes.
    */
    struct stat st;
    if (fstat(fd, &st) == 0) {
        if (S_ISREG(st.st_mode) && (st.st_size % 4) != 0) {
            fprintf(stderr,
                    "Invalid input size: %" PRIdMAX " bytes (must be multiple of 4)\n",
                    (intmax_t)st.st_size);
            if (close_fd) close(fd);
            return EXIT_FAILURE;
        }
    }

    /*
     Optional performance hint:
     tell the OS that we expect sequential reading.
    */
#ifdef POSIX_FADV_SEQUENTIAL
    (void)posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
#endif

    // Allocate the two bitsets
    uint64_t *once = alloc_bitset();
    uint64_t *twice = alloc_bitset();

    /*
     Allocate read buffer.

     We allocate BUFFER_BYTES + 4 bytes so that we can safely keep
     up to 3 leftover bytes from the previous read.
    */
    unsigned char *buffer = (unsigned char *)malloc(BUFFER_BYTES + 4);
    if (!buffer) {
        fprintf(stderr, "malloc failed: %s\n", strerror(errno));
        free(once);
        free(twice);
        if (close_fd) close(fd);
        return EXIT_FAILURE;
    }

    /*
     carry = how many leftover bytes remain from the previous read

     Sometimes read() may return a number of bytes that is not divisible by 4.
     Example: if we have 10 bytes, that is:
     - 8 bytes = 2 full uint32_t values
     - 2 bytes leftover

     Those leftover bytes must be kept and combined with the next read.
    */
    size_t carry = 0;

    // Read until EOF
    for (;;) {
        // Read new bytes after any leftover bytes already in the buffer
        ssize_t n = read(fd, buffer + carry, BUFFER_BYTES);

        if (n < 0) {
            // If interrupted by a signal, retry
            if (errno == EINTR) {
                continue;
            }

            fprintf(stderr, "read failed: %s\n", strerror(errno));
            free(buffer);
            free(once);
            free(twice);
            if (close_fd) close(fd);
            return EXIT_FAILURE;
        }

        // n == 0 means end of file
        if (n == 0) {
            break;
        }

        // Total bytes currently in the buffer = leftover + new bytes
        size_t total = carry + (size_t)n;

        // Largest multiple of 4 that fits into 'total'
        size_t full_bytes = (total / 4) * 4;

        // Number of complete uint32_t values available
        size_t count = full_bytes / 4;

        // Treat the first 'full_bytes' bytes as uint32_t values
        const uint32_t *vals = (const uint32_t *)buffer;

        // Process each uint32_t value
        for (size_t i = 0; i < count; ++i) {
            process_value(once, twice, vals[i]);
        }

        // Save any leftover bytes that did not form a full uint32_t
        carry = total - full_bytes;

        if (carry > 0) {
            // Move leftover bytes to the start of the buffer
            memmove(buffer, buffer + full_bytes, carry);
        }
    }

    /*
     After EOF, there should be no leftover bytes.
     If carry != 0, the file ended in the middle of a uint32_t value.
    */
    if (carry != 0) {
        fprintf(stderr,
                "Invalid input: trailing %zu byte(s), expected uint32_t stream\n",
                carry);
        free(buffer);
        free(once);
        free(twice);
        if (close_fd) close(fd);
        return EXIT_FAILURE;
    }

    // Close the file only if we opened it ourselves
    if (close_fd) {
        close(fd);
    }

    /*
     Count final answers:
     - seen_once = number of bits set in 'once'
     - seen_twice_or_more = number of bits set in 'twice'
     - unique = all values seen once + all values seen multiple times
    */
    uint64_t seen_once = popcount_bitset(once);
    uint64_t seen_twice_or_more = popcount_bitset(twice);
    uint64_t unique = seen_once + seen_twice_or_more;

    // Print results
    printf("unique=%" PRIu64 "\n", unique);
    printf("seen_only_once=%" PRIu64 "\n", seen_once);

    // Free allocated memory
    free(buffer);
    free(once);
    free(twice);

    return EXIT_SUCCESS;
}