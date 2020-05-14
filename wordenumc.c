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
#include <assert.h>

static uint64_t *ncr_table;

/**
 * @brief Calculate nCr.
 * Calculate number of combinations of size r can be found in a set of total
 * size n.
 * @param n Total number of elements.
 * @param r Number of elements per combination.
 * @return Number of combinations.
 */
static inline uint64_t ncr(const uint64_t n, const uint64_t r) {
  if (n == 0) {
    return 1;
  }
  if (n == 1) {
    return 1;
  }
  const uint64_t prev_count = n * (n - 1) / 2;
  return *(ncr_table + prev_count + r - 1);
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
  for (uint64_t i = 0; i <= n; ++i) {

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
  assert (s->size != 0);
  return s->arr[s->size - 1];
}

/**
 * @brief Change the value of the top of an int_stack.
 * Modify the top value of the int_stack. Does not require a pop/push.
 * @param s The int_stack.
 * @param nv Such new value.
 */
static inline void change_top_int_stack(struct int_stack *s, int nv) {
  assert (s->size != 0);
  s->arr[s->size - 1] = nv;
}

/**
 * @brief Increase the top of an int_stack by 1.
 * Increase the top of an int_stack by 1. Does not involve a top/push.
 * @param s The int_stack.
 */
static inline void inc_top_int_stack(struct int_stack *s) {
  assert (s->size != 0);
  s->arr[s->size - 1]++;
}

/**
 * @brief Pop an int_stack.
 * Pop an int_stack and get the value of the popped element if the int_stack
 * is not empty.
 * @param s The int_stack.
 * @return The popped element of such int_stack.
 */
static inline int pop_int_stack(struct int_stack *s) {
  assert(s->size != 0);
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
  assert(s->size != s->max_size);
  s->arr[s->size] = num;
  ++(s->size);
}

/**
 * @brief Test if an int_stack is empty.
 * Determine whether an int_stack is empty by its size.
 * @param s The int_stack.
 * @return Whether such int_stack is empty.
 */
static inline bool is_empty_int_stack(struct int_stack *s) {
  return s->size == 0;
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
    ++cur_input;
  }

  // Move cursor forward by 1 so that now we have address to head of first element
  ++cur_input;

  // Compute the Pascal Tree for calculating nCr
  ncr_table = malloc(sizeof(uint64_t) * (1 + n) * n / 2);
  uint64_t *cur_ncr_table = ncr_table;
  for (int i = 1; i <= n; ++i) {
    const int prev_count = (i - 1) * (i - 2) / 2;
    for (int j = 1; j <= i; ++j) {
      if (j == 1 || j == i) {
        *cur_ncr_table = 1;
        ++cur_ncr_table;
        continue;
      }
      *cur_ncr_table =
          *(ncr_table + prev_count + j - 2) + *(ncr_table + prev_count + j - 1);
      ++cur_ncr_table;
    }
  }

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
      ++i;
    }
    ++cur_input;
  }

  // Compute for the total length of strings
  uint64_t total_length = 0lu;
  for (int j = 0; j < n; ++j) {
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
  struct int_stack *cur_subset = make_int_stack(n);

  // Loop from empty set (cur_n=0) to full set (cur_n=n)
  for (int cur_n = 0; cur_n <= n; ++cur_n) {
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
    for (int i = 0; i < cur_n; ++i) {
      cur_subset->arr[i] = i;
    }
    cur_subset->size = cur_n;

    while (true) {
      // Let's print according to content of cur_subset
      memcpy(cur_output, OUTPUT_LINE_START, OUTPUT_LINE_START_LENGTH);
      cur_output += OUTPUT_LINE_START_LENGTH;
      for (int k = 0; k < cur_subset->size; ++k) {
        memcpy(cur_output,
               *(in_heads + cur_subset->arr[k]),
               *(in_lengths + cur_subset->arr[k]));
        cur_output += *(in_lengths + cur_subset->arr[k]);
        if (k != cur_subset->size - 1) {
          memcpy(cur_output, OUTPUT_SEPARATOR, OUTPUT_SEPARATOR_LENGTH);
          cur_output += OUTPUT_SEPARATOR_LENGTH;
        }
      }
      memcpy(cur_output, OUTPUT_LINE_END, OUTPUT_LINE_END_LENGTH);
      cur_output += OUTPUT_LINE_END_LENGTH;
      memcpy(cur_output, OUTPUT_LINE_BREAK, OUTPUT_LINE_BREAK_LENGTH);
      cur_output += OUTPUT_LINE_BREAK_LENGTH;

      // Now let's determine the next cur_subset
      do {
        if (top_int_stack(cur_subset) == n - 1) {
          pop_int_stack(cur_subset);
          if (is_empty_int_stack(cur_subset)) {
            goto end;
          }
          change_top_int_stack(cur_subset, top_int_stack(cur_subset) + 1);
        } else if (cur_subset->size < cur_n) {
          push_int_stack(cur_subset, top_int_stack(cur_subset) + 1);
        } else {
          change_top_int_stack(cur_subset, top_int_stack(cur_subset) + 1);
        }
      } while (cur_subset->size < cur_n);

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
