#include "morse_code.hpp"

#include <cstring>

namespace morse {
namespace {

struct Entry {
    char symbol;
    const char* pattern;
};

constexpr Entry kTable[] = {
    // Letters
    {'A', ".-"},     {'B', "-..."},   {'C', "-.-."},   {'D', "-.."},
    {'E', "."},      {'F', "..-."},   {'G', "--."},    {'H', "...."},
    {'I', ".."},     {'J', ".---"},   {'K', "-.-"},    {'L', ".-.."},
    {'M', "--"},     {'N', "-."},     {'O', "---"},    {'P', ".--."},
    {'Q', "--.-"},   {'R', ".-."},    {'S', "..."},    {'T', "-"},
    {'U', "..-"},    {'V', "...-"},   {'W', ".--"},    {'X', "-..-"},
    {'Y', "-.--"},   {'Z', "--.."},
    // Digits
    {'0', "-----"},  {'1', ".----"},  {'2', "..---"},  {'3', "...--"},
    {'4', "....-"},  {'5', "....."},  {'6', "-...."},  {'7', "--..."},
    {'8', "---.."},  {'9', "----."},
    // Punctuation
    {'.', ".-.-.-"}, {',', "--..--"}, {'?', "..--.."}, {'\'', ".----."},
    {'!', "-.-.--"}, {'/', "-..-."},  {'(', "-.--."},  {')', "-.--.-"},
    {'&', ".-..."},  {':', "---..."}, {';', "-.-.-."}, {'=', "-...-"},
    {'+', ".-.-."},  {'-', "-....-"}, {'_', "..--.-"}, {'"', ".-..-."},
    {'$', "...-..-"}, {'@', ".--.-."},
};

char toUpper(char c) { return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c; }

}  // namespace

const char* encode(char c) {
    const char upper = toUpper(c);
    for (const Entry& e : kTable) {
        if (e.symbol == upper) return e.pattern;
    }
    return nullptr;
}

uint32_t messageUnits(const char* text) {
    uint32_t units = 0;
    bool sentAny = false;
    bool wordGap = false;
    for (const char* t = text; t != nullptr && *t != '\0'; ++t) {
        const char* pattern = encode(*t);
        if (pattern == nullptr) {
            if (*t == ' ') wordGap = true;
            continue;
        }
        if (sentAny) units += wordGap ? kWordGapUnits : kCharGapUnits;
        for (const char* p = pattern; *p != '\0'; ++p) {
            if (p != pattern) units += kElementGapUnits;
            units += (*p == '-') ? kDashUnits : kDotUnits;
        }
        sentAny = true;
        wordGap = false;
    }
    return units;
}

char decode(const char* pattern) {
    if (pattern == nullptr || pattern[0] == '\0') return '\0';
    for (const Entry& e : kTable) {
        if (std::strcmp(e.pattern, pattern) == 0) return e.symbol;
    }
    return '\0';
}

}  // namespace morse
