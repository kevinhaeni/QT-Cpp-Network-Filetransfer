#pragma once

#include <crtdbg.h>
#include <cassert>
#include <stdexcept>

// Verifies a pointer
#define chkptr(p) assert(p)

#define ignore_unused(v) do { (void)v; } while(0);

namespace util {

template<bool> struct StaticAssert;
template<> struct StaticAssert<true> {};

typedef unsigned char T_UI1;
typedef unsigned int T_UI4;

} // namespace util
