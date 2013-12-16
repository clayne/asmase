/*
 * Copyright (C) 2013 Omar Sandoval
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "input.h"

/**
 * Limit on the depth of the redirection stack to prevent infinite recursion.
 */
#define MAX_INPUT_STACK_SIZE 128

/** Stack of files from which we are reading. */
static FILE **input_stack = NULL;

/** Number of files in the stack. */
static size_t input_stack_size = 0;

/** Number of slots in the stack. */
static size_t input_stack_capacity = 0;

/** See input.h. */
void init_input()
{
    using_history();

    input_stack_capacity = 4;
    input_stack = malloc(input_stack_capacity * sizeof(*input_stack));
}

/** See input.h. */
void shutdown_input()
{
    free(input_stack);
    clear_history();
}

/** See input.h. */
char *read_input_line(const char *prompt)
{
    char *line = NULL;
    size_t n = 0;
retry:
    if (input_stack_size == 0) {
        line = readline(prompt);
        if (line && line[0]) /* If not empty, add to history */
            add_history(line);
    } else {
        FILE *top_file = input_stack[input_stack_size - 1];
        char *new_line;
        if (getline(&line, &n, top_file) == -1) {
            fclose(top_file);
            --input_stack_size;
            goto retry;
        }

        if ((new_line = strchr(line, '\n')))
            *new_line = '\0';
    }

    return line;
}

/** See input.h. */
int redirect_input(const char *path)
{
    FILE *file;

    if (input_stack_size == MAX_INPUT_STACK_SIZE) {
        fprintf(stderr, "Input redirection stack too deep\n");
        return 1;
    }

    if (!(file = fopen(path, "r"))) {
        fprintf(stderr, "Could not open file `%s'\n", path);
        return 1;
    }

    while (input_stack_size >= input_stack_capacity) {
        input_stack_capacity *= 2;
        input_stack = realloc(input_stack,
            input_stack_capacity * sizeof(*input_stack));
        if (!input_stack) {
            perror("realloc");
            exit(1);
        }
    }

    input_stack[input_stack_size++] = file;

    return 0;
}
