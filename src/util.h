#ifndef UTIL_H
#define UTIL_H

#include <cstring>
#include <cstdint>

class Util {
   public:
    // Remove leading and trailing whitespace from a string
    static void trimInPlace(char* s) {
        if (s == nullptr) return;
        char* start = s;
        while (*start == ' ' ||
               *start == '\t' ||
               *start == '\r' ||
               *start == '\n')
            ++start;
        if (start != s) {
            size_t len = strlen(start) + 1;
            memmove(s, start, len);
        }
        size_t len = strlen(s);
        while (len > 0 &&
               (s[len - 1] == ' ' ||
                s[len - 1] == '\t' ||
                s[len - 1] == '\r' ||
                s[len - 1] == '\n'))
            s[--len] = '\0';
    }

    // Skip leading whitespace characters and return pointer to first non-whitespace
    static const char* skipWhitespace(const char* s) {
        if (s == nullptr) return "";
        while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
            ++s;
        return s;
    }

    // Skip leading whitespace, extract the next whitespace-delimited token,
    // and advance the input pointer past the token.
    // Returns true if a token was extracted, false if empty or too long.
    static bool nextToken(const char*& input, char* output, size_t outputSize) {
        if (input == nullptr || output == nullptr || outputSize == 0)
            return false;
        input = skipWhitespace(input);
        if (*input == '\0') {
            output[0] = '\0';
            return false;
        }
        size_t length = 0;
        while (input[length] != '\0' &&
               input[length] != ' ' &&
               input[length] != '\t' &&
               input[length] != '\r' &&
               input[length] != '\n') {
            if (length >= outputSize - 1) {
                output[0] = '\0';
                return false;
            }
            output[length] = input[length];
            ++length;
        }
        output[length] = '\0';
        input += length;
        return true;
    }

    // Safely copy a string with null termination (strncpy + null-terminate wrapper)
    static void copyString(char* dst, size_t dstSize, const char* src) {
        if (dst == nullptr || dstSize == 0) return;
        if (src == nullptr) {
            dst[0] = '\0';
            return;
        }
        strncpy(dst, src, dstSize - 1);
        dst[dstSize - 1] = '\0';
    }

    // Convert a hex buffer to a string
    static void hexToStr(char* buf, size_t bufSize,
                         const uint8_t* data, size_t dataSize) {
        if (bufSize == 0) return;
        if (dataSize > 0 && bufSize < dataSize * 3) return;

        static const char hex[] = "0123456789ABCDEF";

        char* p = buf;

        for (size_t i = 0; i < dataSize; ++i) {
            *p++ = hex[data[i] >> 4];
            *p++ = hex[data[i] & 0x0F];
            if (i + 1 < dataSize) *p++ = ' ';
        }

        *p = '\0';
    }

    // Parse a boolean value from the command line.
    // - returns true if a token was extracted
    // - returns false if empty, too long, or not a boolean value
    // - valid values are "on" and "off"
    static bool parseBoolValue(const char* args, bool* value) {
        args = skipWhitespace(args);

        char token[6] = {};
        if (!nextToken(args, token, sizeof(token))) return false;

        const char* rest = skipWhitespace(args);
        if (*rest != '\0') return false;

        if (isStrBoolOn(token)) {
            *value = true;
            return true;
        }
        if (isStrBoolOff(token)) {
            *value = false;
            return true;
        }
        return false;
    }

    static bool isStrBoolOn(const char* str) {
        return strcmp(str, "on") == 0;
    }

    static bool isStrBoolOff(const char* str) {
        return strcmp(str, "off") == 0;
    }

    static const char* boolToString(bool value) {
        return value ? "on" : "off";
    }
};

#endif  // UTIL_H