/* Tools - string_utils.c
 *
 * Purpose: string manipulation helpers.
 */
#include "utils/utils.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

static bool append_part(char *buffer, size_t buffer_size, size_t *offset, const char *text) {
    if (buffer == NULL || offset == NULL || text == NULL || buffer_size == 0) {
        return false;
    }

    if (*offset >= buffer_size - 1) {
        return false;
    }

    size_t remaining = buffer_size - *offset;
    int written = snprintf(buffer + *offset, remaining, "%s", text);

    if (written < 0) {
        return false;
    }

    size_t appended = (size_t) written;
    if (appended >= remaining) {
        *offset = buffer_size - 1;
        buffer[buffer_size - 1] = '\0';
        return false;
    }

    *offset += appended;
    return true;
}

bool string_join(char *buffer, size_t buffer_size, const char *const *items, size_t count, const char *separator,
                 const char *prefix, const char *suffix) {
    size_t offset = 0;

    if (buffer == NULL || buffer_size == 0 || (count > 0 && items == NULL)) {
        return false;
    }

    buffer[0] = '\0';

    if (!append_part(buffer, buffer_size, &offset, prefix != NULL ? prefix : "")) {
        return false;
    }

    for (size_t index = 0; index < count; ++index) {
        if (index > 0 && !append_part(buffer, buffer_size, &offset, separator != NULL ? separator : "")) {
            return false;
        }

        if (!append_part(buffer, buffer_size, &offset, items[index])) {
            return false;
        }
    }

    if (!append_part(buffer, buffer_size, &offset, suffix != NULL ? suffix : "")) {
        return false;
    }

    return true;
}
