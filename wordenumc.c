// Copyright (c) 2020, Michael Lee (michaellee8)
// SPDX-License-Identifier: MIT

/**
 * @file wordenumc.c
 * @author Michael Lee (michaellee8)
 * @date 2020/05/13
 * @brief Given a set of strings separated in lines, enumerate all subsets of it.
 *
 * # COMP2119 2020 Programming Assignment
 *
 * ## Supported platform
 * - Any Linux system on x86_64 architecture. Tested to work on
 *   Ubuntu 18.04 LTS x86_64.
 * - It is assumed that you have GNU Compiler Suite (`gcc`) installed. Such
 *   compiler must support C11.
 *
 * ## How to run
 * 1. It is assumed that both `input.txt` and `wordenumc.c` (this file) is
 *    placed in the same directory.
 * 2. `cd` to that directory.
 * 3. Compile and link the program with `gcc -O3 wordenumc.c -o wordenumc`.
 * 4. Run the program with `./wordenumc`. If `output.txt` already exists, it
 *    will be rewritten.
 * 5. Now you should be able to see the result at `./wordenumc`.
 *
 * ## Input
 * - An input.txt file containing ASCII (char) encoded strings placed at the
 *   current directory (pwd).
 * - The first line of such file contains an integer n, in which n is the
 *   number of lines of string in the file.
 * - The next n lines each line contains an ASCII string, with an `\n` character
 *   separating each line.
 * - The last character of file must be an `\n` character.
 * - Example: `3\nfoo\nboo\nbar\n`
 *
 * ## Output
 * - An output.txt file containing ASCII (char) encoded strings placed at the
 *   current directory (pwd). Any existing output.txt will be overwritten.
 * - Each line contains a possible subset of the set of strings as specified
 *   in input.txt, including the empty set.
 * - Each line starts with `{` and ends with `}`. Each element in such set is
 *   seperated by `, `.
 * - End of each line includes a `\n` character.
 * - The last character of file must be an `\n` character.
 * - The order of output is deterministic, which means for any identical
 *   input.txt, an identical output.txt will be provided.
 * - Example:
 * `{}\n{foo}\n{boo}\n{bar}\n{foo, boo}\n{foo, bar}\n{boo, bar}\n{foo, boo, bar}\n`
 *
 * ## Limitations
 * - It is assumed that `n <= 20`, such that `fact(n) <= UINT64_MAX`
 *
 * ## License
 * MIT License
 *
 * ## Notes
 * - This program uses the mmap system call to prevent syscall overhead due to
 *   multiple read/write calls.
 * - It should also be okay if input.txt does not have an `\n` in the end of it,
 *   since the program will allocate one more byte larger than the size of text
 *   file and then put an `\n` to that last byte, hence ensuring whatever the
 *   file has an `\n` at the end of it, it's `mmap`ed image in the program must
 *   be ended with an `\n`.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

/**
 * @brief Precomputed fact(n)
 *
 * Precomputed value of fact(n) for 0 <= n <= 20, which is the range of n
 * acceptable in this program.
 */
uint64_t FACT_N[21] =
    {
        1,
        1,
        2,
        6,
        24,
        120,
        720,
        5040,
        40320,
        362880,
        3628800,
        39916800,
        479001600,
        6227020800,
        87178291200,
        1307674368000,
        20922789888000,
        355687428096000,
        6402373705728000,
        121645100408832000,
        2432902008176640000,
    };

inline uint64_t fact(const uint64_t n) {
  if (n < 21lu) {
    return FACT_N[n];
  }
  if (n == 0lu || n == 1lu) {
    return 1lu;
  } else {
    return n * fact(n - 1lu);
  }
}

inline uint64_t ncr(const uint64_t n, const uint64_t r) {
  return fact(n) / fact(n - r) / fact(r);
}

extern inline void panic(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

const char *INPUT_FILENAME = "input.txt";
const char *OUTPUT_FILENAME = "output.txt";
const int FILE_MODE = 0x0777;

const char OUTPUT_LINE_START[1] = {'{'};
const char OUTPUT_LINE_END[1] = {'}'};
const char OUTPUT_LINE_BREAK[1] = {'\n'};
const char OUTPUT_SEPARATOR[2] = {',', ' '};

/**
 * Program entry point.
 * @return 0 if the program succeed, any other integer if program failed.
 */
int main() {

  // Open input.txt
  int fd_in = open(INPUT_FILENAME, O_RDONLY);
  if (fd_in < 0) {
    panic("Error opening input file: ");
    return 0;
  }

  // Get size of input.txt
  struct stat stat_in;
  if (fstat(fd_in, &stat_in) < 0) {
    panic("Error getting size of input file: ");
    return 0;
  }
  __off_t size_in = stat_in.st_size + 1; // Pad one more byte to add a \n there

  // Mmap the input file
  char *input =
      mmap(NULL, size_in + 1, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd_in, 0);
  if (input == MAP_FAILED) {
    panic("Error mapping input file: ");
    return 0;
  }

  // Add the '\n' to the last byte of such mapped file
  *(input + size_in - 1) = '\n';

  // Use this as a cursor to currently pointed location
  char *cur_input = input;

  // Read the number on the first line into n (wd don't like slow scanf here)
  int n = 0;
  while (*cur_input != '\n') {
    n *= 10;
    n += (*cur_input) - '0';
    cur_input++;
  }

  // Move cursor forward by 1 so that now we have address to head of first element
  cur_input++;

  // Allocate for an array storing cursor to head of each element
  char **in_heads = calloc(sizeof(char *), n);
  if (in_heads == NULL) {
    panic("Error allocating for in_heads: ");
    return 0;
  }

  // Allocate for an array storing cursor to length of each element
  long *in_lengths = calloc(sizeof(long), n);
  if (in_lengths == NULL) {
    panic("Error allocating for in_lengths: ");
    return 0;
  }

  // Parse head and length of each element into in_heads and in_lengths
  int i = 0;
  bool next_is_head = true; // whether next char will should be head of element
  while (i < n) {
    if (next_is_head) {
      in_heads[i] = cur_input;
      next_is_head = false;
    }
    if ((*cur_input) == '\n') {
      in_lengths[i] = cur_input - in_heads[i];
      next_is_head = true;
      i++;
    }
    cur_input++;
  }

  for (int i = 0; i < n; i++) {
    write(STDOUT_FILENO, in_heads[i], in_lengths[i]);
    write(STDOUT_FILENO, OUTPUT_LINE_BREAK, 1);
  }



  return 0;
}
