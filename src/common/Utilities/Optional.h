#ifndef NCORE_OPTIONAL_H
#define NCORE_OPTIONAL_H

#include <optional>

//! Optional helper class to wrap optional values within.
template <class T>
using Optional = std::optional<T>;

#endif
