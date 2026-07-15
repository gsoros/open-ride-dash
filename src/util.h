#ifndef UTIL_H
#define UTIL_H

#include <cstring>

class Util {
   public:
    static void trimInPlace(char* text) {
        if (text == nullptr) return;
        char* start = text;
        while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') ++start;
        if (start != text) {
            size_t len = strlen(start) + 1;
            memmove(text, start, len);
        }
        size_t len = strlen(text);
        while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t' || text[len - 1] == '\r' || text[len - 1] == '\n')) {
            text[--len] = '\0';
        }
    }

    static void hexToStr(char* buf, size_t bufSize,
                         const uint8_t* data, size_t dataSize) {
        if (bufSize == 0) return;
        if (dataSize > 0 && bufSize < dataSize * 3) return;

        static const char hex[] = "0123456789ABCDEF";

        char* p = buf;

        for (size_t i = 0; i < dataSize; ++i) {
            *p++ = hex[data[i] >> 4];
            *p++ = hex[data[i] & 0x0F];

            if (i + 1 < dataSize)
                *p++ = ' ';
        }

        *p = '\0';
    }
};

#endif  // UTIL_H