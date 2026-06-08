#ifndef SFMT_RAND_H
#define SFMT_RAND_H

#include <new>
#include <SFMT.h>
#include "Define.h"

class SFMTRand
{
public:
    SFMTRand();
    uint32 RandomUInt32(); // Output random bits
    void* operator new(std::size_t size, std::nothrow_t const&);
    void operator delete(void* ptr, std::nothrow_t const&);
    void* operator new(std::size_t size);
    void operator delete(void* ptr);
    void* operator new[](std::size_t size, std::nothrow_t const&);
    void operator delete[](void* ptr, std::nothrow_t const&);
    void* operator new[](std::size_t size);
    void operator delete[](void* ptr);
private:
    sfmt_t _state;
};

#endif
