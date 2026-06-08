#ifndef NCORE_DEFINE_H
#define NCORE_DEFINE_H

#include <cinttypes>

#define AC_API_EXPORT
#define AC_COMMON_API
#define AC_DATABASE_API
#define AC_GAME_API

#define UI64LIT(N) UINT64_C(N)
#define SI64LIT(N) INT64_C(N)

typedef std::int64_t int64;
typedef std::int32_t int32;
typedef std::int16_t int16;
typedef std::int8_t int8;
typedef std::uint64_t uint64;
typedef std::uint32_t uint32;
typedef std::uint16_t uint16;
typedef std::uint8_t uint8;

#endif
