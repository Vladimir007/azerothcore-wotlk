#ifndef NCORE_AUTH_DEFINES_H
#define NCORE_AUTH_DEFINES_H

#include "Define.h"
#include <array>

constexpr std::size_t SESSION_KEY_LENGTH = 40;
using SessionKey = std::array<uint8, SESSION_KEY_LENGTH>;

#endif
