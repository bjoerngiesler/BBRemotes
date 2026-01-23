#if !defined(BBRUTILS_H)
#define BBRUTILS_H

#include <Arduino.h>
#include <stdarg.h>
#include "BBRTypes.h"

namespace bb {
namespace rmt {

// Sigh. That we need to do this every. single. time.
static int printfFinal(const char* str) {
    Serial.print(str);
    return strlen(str);
}

static int vprintf(const char* format, va_list args) {
    int len = vsnprintf(NULL, 0, format, args) + 1;
    char *buf = new char[len];
    vsnprintf(buf, len, format, args);
    printfFinal(buf);
    free(buf);
    return len;
}

static int printf(const char* format, ...) {
    int retval;
    va_list args;
    va_start(args, format);
    retval = vprintf(format, args);
    va_end(args);
    return retval;
}

//! Enum for unit conversion.
enum Unit {
    UNIT_DEGREES          = 0, // [0..360)
    UNIT_DEGREES_CENTERED = 1, // (-180..180)
    UNIT_UNITY            = 2, // [0..1]
    UNIT_UNITY_CENTERED   = 3, // [-1..1]
    UNIT_RAW              = 4  // dependent on wire interface
};

//! normed goes from 0 to 1, and gets transformed into [0..360], [-180..180], [-1..1] depending on Unit value.
static float normedToUnit(float normed, Unit unit) {
    normed = constrain(normed, 0.0f, 1.0f);
    switch(unit) {
    case UNIT_DEGREES:
        return normed * 360.0f;
    case UNIT_DEGREES_CENTERED:
        return (normed-0.5)*360.0f;
    case UNIT_UNITY_CENTERED:
        return (normed-0.5)*2.0f;
    default:
        break;
    }
    return normed;
}

//! val goes from [0..360], [-180..180], [-1..1] depending on Unit value. Returns value in range [0..1].
static float unitToNormed(float val, Unit unit) {
    switch(unit) {
    case UNIT_DEGREES:
        val = constrain(val, 0.0f, 360.0f);
        return val / 360.0f;
    case UNIT_DEGREES_CENTERED:
        val = constrain(val, -180.0f, 180.0f);
        return (val / 360.0f) + 0.5;
    case UNIT_UNITY_CENTERED:
        val = constrain(val, -1.0f, 1.0f);
        return (val / 2.0f) + 0.5;
    case UNIT_UNITY:
        return constrain(val, 0.0f, 1.0f);
        break;
    default:
        break;
    }
    return val;
}

};
};

#endif