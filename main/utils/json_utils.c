/* Tools - json_utils.c
 *
 * Purpose: parse small JSON payloads used by REST handlers.
 */
#include "utils/utils.h"

#include <stdio.h>
#include <string.h>

static const char *find_value_start(const char *json, const char *key) {
    /* 3 = opening quote + closing quote + NUL terminator for "key". */
    char pattern[strlen(key) + 3];

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);

    const char *key_pos = strstr(json, pattern);

    if (key_pos == NULL) {
        return NULL;
    }

    const char *colon = strchr(key_pos, ':');

    if (colon == NULL) {
        return NULL;
    }

    colon++;

    while (*colon == ' ' || *colon == '\t' || *colon == '\n' || *colon == '\r') {
        colon++;
    }

    return colon;
}

JsonStringResult json_get_string(const char *json, const char *key) {
    JsonStringResult result = {
        .ok = false,
        .value = {0}
    };

    const char *value = find_value_start(json, key);

    if (value == NULL || *value != '"') {
        return result;
    }

    value++;

    int index = 0;

    while (*value != '\0' && *value != '"' && index < JSON_STRING_RESULT_MAX_LENGTH - 1) {
        result.value[index] = *value;
        index++;
        value++;
    }

    result.value[index] = '\0';
    result.ok = *value == '"';

    return result;
}

JsonFloatResult json_get_float(const char *json, const char *key) {
    JsonFloatResult result = {
        .ok = false,
        .value = 0.0f
    };

    const char *value = find_value_start(json, key);

    if (value == NULL) {
        return result;
    }

    result.ok = sscanf(value, "%f", &result.value) == 1;
    return result;
}

JsonIntResult json_get_int(const char *json, const char *key) {
    JsonIntResult result = {
        .ok = false,
        .value = 0
    };

    const char *value = find_value_start(json, key);

    if (value == NULL) {
        return result;
    }

    result.ok = sscanf(value, "%d", &result.value) == 1;
    return result;
}
