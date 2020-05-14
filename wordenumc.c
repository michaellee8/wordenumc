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
 * ## References
 * - https://stackoverflow.com/questions/26259421/use-mmap-in-c-to-write-into-memory
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
#include <stdnoreturn.h>
#include <string.h>

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

/**
 * @brief Calculate factorial of n.
 * Calculate factorial of n. If n <= 20 then use the lookup table, otherwise
 * calculate it using recursion.
 * @param n The number to calculate factorial for. Should be n <= 20.
 * @return Factorial of n.
 */
static inline uint64_t fact(const uint64_t n) {
  if (n < 21lu) {
    return FACT_N[n];
  }
  uint64_t result = 1;
  for (int i = 21; i <= n; i++) {
    result *= i;
  }
  return result * FACT_N[20];
}

/**
 * @brief Calculate nCr.
 * Calculate number of combinations of size r can be found in a set of total
 * size n.
 * @param n Total number of elements.
 * @param r Number of elements per combination.
 * @return Number of combinations.
 */
static inline uint64_t ncr(const uint64_t n, const uint64_t r) {
  return fact(n) / fact(n - r) / fact(r);
}

/**
 * @brief Shorthand for handling fatal errors.
 * Shorthand for handling fatal errors. Output error message and then exit with
 * an error exit value.
 * @param msg Error message to be displayed.
 */
static inline noreturn void panic(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

const char *INPUT_FILENAME = "input.txt";
const char *OUTPUT_FILENAME = "output.txt";
const int OUTPUT_FILE_MODE = 0777;

#define OUTPUT_LINE_START_LENGTH 1lu
#define OUTPUT_LINE_END_LENGTH 1lu
#define OUTPUT_LINE_BREAK_LENGTH 1lu
#define OUTPUT_SEPARATOR_LENGTH 2lu

const char OUTPUT_LINE_START[OUTPUT_LINE_START_LENGTH] = {'{'};
const char OUTPUT_LINE_END[OUTPUT_LINE_END_LENGTH] = {'}'};
const char OUTPUT_LINE_BREAK[OUTPUT_LINE_BREAK_LENGTH] = {'\n'};
const char OUTPUT_SEPARATOR[OUTPUT_SEPARATOR_LENGTH] = {',', ' '};

/**
 * @brief Calculate the size of the required output file for this program.
 * @param total_length Total length of ell the elements.
 * @param n Number of elements.
 * @return Size of output file (in bytes).
 */
static inline uint64_t compute_output_size(uint64_t total_length, uint64_t n) {
  uint64_t sum = 0;
  for (uint64_t i = 0; i <= n; i++) {
    if (i == 0) {
      sum += OUTPUT_LINE_START_LENGTH + OUTPUT_LINE_END_LENGTH
          + OUTPUT_LINE_BREAK_LENGTH;
      continue;
    }
    sum += ncr(n, i) * (OUTPUT_LINE_START_LENGTH + OUTPUT_LINE_END_LENGTH
        + (i - 1) * OUTPUT_SEPARATOR_LENGTH + OUTPUT_LINE_BREAK_LENGTH);
    sum += total_length * ncr(n, i) * i / n;
  }
  return sum;
}

/**
 * @brief An int stack
 * A stack data structure for int.
 */
struct int_stack {
  int *arr;
  int max_size;
  int size;
};

/**
 * @brief Make a new int_stack.
 * Create and allocate for a new int_stack.
 * @param max_size Maximum size of the stack.
 * @return The new stack.
 */
static inline struct int_stack *make_int_stack(int max_size) {
  struct int_stack *s = malloc(sizeof(struct int_stack));
  s->max_size = max_size;
  s->size = 0;
  s->arr = calloc(sizeof(int), max_size);
  return s;
}

/**
 * @brief Top of an int_stack.
 * Get the current top of an int_stack if the int_stack is not empty.
 * @param s The int_stack.
 * @return Top of such int_stack.
 */
static inline int top_int_stack(struct int_stack *s) {
  if (s->size == 0) {
    panic("Error top stack: stack is empty");
    return 0;
  }
  return s->arr[s->size - 1];
}

/**
 * @brief Pop an int_stack.
 * Pop an int_stack and get the value of the popped element if the int_stack
 * is not empty.
 * @param s The int_stack.
 * @return The popped element of such int_stack.
 */
static inline int pop_int_stack(struct int_stack *s) {
  if (s->size == 0) {
    panic("Error pop stack: stack is empty");
    return 0;
  }
  s->size--;
  return s->arr[s->size];
}

/**
 * @brief Push an element into int_stack.
 * Push a new element into int_stack if the int_stack is not full.
 * @param s The int_stack.
 * @param num The new element to be pushed into.
 */
static inline void push_int_stack(struct int_stack *s, int num) {
  if (s->size == s->max_size) {
    panic("Error push stack: stack is full");
    return;
  }
  s->arr[s->size] = num;
  s->size++;
}

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
  size_t size_in = stat_in.st_size + 1; // Pad one more byte to add a \n there

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

  // Read the number on the first line into n (I don't like slow scanf here)
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

  // Compute for the total length of strings
  uint64_t total_length = 0lu;
  for (int j = 0; j < n; j++) {
    total_length += in_lengths[j];
  }

  // Create (overwrite if exists) output.txt
  int fd_out =
      open(OUTPUT_FILENAME, O_RDWR | O_CREAT | O_TRUNC, OUTPUT_FILE_MODE);
  if (fd_out < 0) {
    panic("Error opening output file: ");
  }

  uint64_t size_out = compute_output_size(total_length, n);

  // Extend the output file to the calculated size
  if (lseek(fd_out, size_out - 1, SEEK_SET) == -1) {
    panic("Error go to last byte of output file");
  }
  if (write(fd_out, "", 1) != 1) {
    panic("Error appending last byte to output file");
  }

  // Mmap the output file
  char *output = mmap(NULL,
                      size_out,
                      PROT_WRITE | PROT_READ,
                      MAP_SHARED,
                      fd_out,
                      0);

  // Use this as a cursor to currently pointed location
  char *cur_output = output;

  // Let's write to output file

  // Initialize the stack we are using to store the current subset
  // We don't need calloc here since we will manually initialize it
  int *cur_subset = malloc(n * sizeof(int));

  // Loop from empty set (cur_n=0) to full set (cur_n=n)
  for (int cur_n = 0; cur_n <= n; cur_n++) {
    if (cur_n == 0) {
      // Handle empty set case
      memcpy(cur_output, OUTPUT_LINE_START, OUTPUT_LINE_START_LENGTH);
      cur_output += OUTPUT_LINE_START_LENGTH;
      memcpy(cur_output, OUTPUT_LINE_END, OUTPUT_LINE_END_LENGTH);
      cur_output += OUTPUT_LINE_END_LENGTH;
      memcpy(cur_output, OUTPUT_LINE_BREAK, OUTPUT_LINE_BREAK_LENGTH);
      cur_output += OUTPUT_LINE_BREAK_LENGTH;
      continue;
    }

    // Initialize the first subset of each cur_n
    for (int i = 0; i < cur_n; i++) {
      cur_subset[i] = i;
    }

    while (true) {
      // Let's print according to content of cur_subset
      memcpy(cur_output, OUTPUT_LINE_START, OUTPUT_LINE_START_LENGTH);
      cur_output += OUTPUT_LINE_START_LENGTH;
      for (int k = 0; k < cur_n; k++) {
        memcpy(cur_output,
               *(in_heads + cur_subset[k]),
               *(in_lengths + cur_subset[k]));
        cur_output += *(in_lengths + cur_subset[k]);
        if (k != cur_n - 1) {
          memcpy(cur_output, OUTPUT_SEPARATOR, OUTPUT_SEPARATOR_LENGTH);
          cur_output += OUTPUT_SEPARATOR_LENGTH;
        }
      }
      memcpy(cur_output, OUTPUT_LINE_END, OUTPUT_LINE_END_LENGTH);
      cur_output += OUTPUT_LINE_END_LENGTH;
      memcpy(cur_output, OUTPUT_LINE_BREAK, OUTPUT_LINE_BREAK_LENGTH);
      cur_output += OUTPUT_LINE_BREAK_LENGTH;

      // Now let's determine the next cur_subset

      while (true) {

      }

    }
    end:;
  }

  // Clean up files
  if (munmap(input, size_in + 1) < 0) {
    panic("Error unmapping input file: ");
    return 0;
  }
  if (munmap(output, size_out) < 0) {
    panic("Error unmapping output file: ");
    return 0;
  }
  if (close(fd_in) < 0) {
    panic("Error closing input file: ");
    return 0;
  }
  if (close(fd_out) < 0) {
    panic("Error closing output file: ");
    return 0;
  }

  return 0;
}
