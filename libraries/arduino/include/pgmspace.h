#pragma once

// ORIScore ESPsim compatibility layer for AVR/ESP PROGMEM helpers.
// In the desktop simulator program-memory data live in normal process memory.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>

#ifndef PROGMEM
#define PROGMEM
#endif

#ifndef PGM_P
using PGM_P = const char*;
#endif

#ifndef PGM_VOID_P
using PGM_VOID_P = const void*;
#endif

#ifndef PSTR
#define PSTR(string_literal) (string_literal)
#endif

#ifndef pgm_read_byte
#define pgm_read_byte(address_short) (*reinterpret_cast<const uint8_t*>(address_short))
#endif
#ifndef pgm_read_byte_near
#define pgm_read_byte_near(address_short) pgm_read_byte(address_short)
#endif
#ifndef pgm_read_byte_far
#define pgm_read_byte_far(address_long) pgm_read_byte(address_long)
#endif
#ifndef pgm_read_word
#define pgm_read_word(address_short) (*reinterpret_cast<const uint16_t*>(address_short))
#endif
#ifndef pgm_read_word_near
#define pgm_read_word_near(address_short) pgm_read_word(address_short)
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(address_short) (*reinterpret_cast<const uint32_t*>(address_short))
#endif
#ifndef pgm_read_float
#define pgm_read_float(address_short) (*reinterpret_cast<const float*>(address_short))
#endif
#ifndef pgm_read_ptr
#define pgm_read_ptr(address_short) (*reinterpret_cast<void* const*>(address_short))
#endif

#ifndef strlen_P
#define strlen_P(source) std::strlen(source)
#endif
#ifndef strcpy_P
#define strcpy_P(destination, source) std::strcpy((destination), (source))
#endif
#ifndef strncpy_P
#define strncpy_P(destination, source, count) std::strncpy((destination), (source), (count))
#endif
#ifndef strcat_P
#define strcat_P(destination, source) std::strcat((destination), (source))
#endif
#ifndef strcmp_P
#define strcmp_P(left, right) std::strcmp((left), (right))
#endif
#ifndef strncmp_P
#define strncmp_P(left, right, count) std::strncmp((left), (right), (count))
#endif
#ifndef strstr_P
#define strstr_P(haystack, needle) std::strstr((haystack), (needle))
#endif
#ifndef memcpy_P
#define memcpy_P(destination, source, count) std::memcpy((destination), (source), (count))
#endif
#ifndef memcmp_P
#define memcmp_P(left, right, count) std::memcmp((left), (right), (count))
#endif
#ifndef sprintf_P
#define sprintf_P std::sprintf
#endif
#ifndef snprintf_P
#define snprintf_P std::snprintf
#endif
