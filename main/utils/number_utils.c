/* Tools - number_utils.c
 *
 * Purpose: numeric conversion helpers.
 */
#include "utils/utils.h"

#include <string.h>

uint32_t float_to_uint32(float value) {
    uint32_t result;
    memcpy(&result, &value, sizeof(float));
    return result;
}

float uint32_to_float(uint32_t value) {
    float result;
    memcpy(&result, &value, sizeof(float));
    return result;
}
