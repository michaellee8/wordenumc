// Copyright (c) 2020, Michael Lee (michaellee8)
// SPDX-License-Identifier: MIT

#include <stdio.h>

//This program:
//- only accept ascii input file (input.txt)
//- runs on linux x86_64 system only
//- assumes that the program source is placed together with the input.txt
//  file in the same directory
//- can be ran in such directory with `gcc -c program.S && ld program.o && ./a.out && rm program.o a.out
//- accepts an ascii number [0-9]+ in the first line
//- accepts an ascii string, including space, on each line
//- assukes that each line is ended by a \n character (no \r\n on windows)
//- assumes that there is an empty line in the end of file (i.e. the last byte of input.txt must be '\n')

/**
 * wordenumc.c
 * Given a set of strings separated in lines, enumerate all subsets of it.
 * Input:
 * - An input.txt file containing ASCII (char) encoded strings placed at the
 *   current directory (pwd).
 * - The first line of such file contains an integer n, in which n is the
 *   number of lines of string in the file.
 * - The next n lines each line contains an ASCII string, with an '\n' character
 *   separating each line.
 * - The last character of file must be an '\n' character.
 * - Example: "3\ncar\ntoy\nhappy\n"
 * Output:
 * - An output.txt file containing ASCII (char) encoded strings placed at the
 *   current directory (pwd). Any existing output.txt will be overwritten.
 * - Each line contains a possible subset of the set of strings as specified
 *   in input.txt, including the empty set.
 * - Each line starts with '{' and ends with '}'. Each element in such set is
 *   seperated by ", ".
 * - End of each line includes a '\n' character.
 * - The last character of file must be an '\n' character.
 * - The order of output is deterministic, which means for any identical
 *   input.txt, an identical output.txt will be provided.
 * - Example: "{car}\n{toy}\n{happy}"
 * Limits:
 * - It is assumed that n <= 20, such that n! <= UINT64_MAX
 * @return 0 if the program success, other integers if failed.
 */
int main() {
    printf("Hello, World!\n");
    return 0;
}
