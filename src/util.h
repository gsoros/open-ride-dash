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
};

#endif  // UTIL_H