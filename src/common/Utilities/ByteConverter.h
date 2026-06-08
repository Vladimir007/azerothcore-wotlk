#ifndef ACORE_BYTE_CONVERTER_H
#define ACORE_BYTE_CONVERTER_H

namespace ByteConverter
{
    template<std::size_t T>
    void convert(char* val)
    {
        std::swap(*val, *(val + T - 1));
        convert<T - 2>(val + 1);
    }

    template<> inline void convert<0>(char*) { }
    template<> inline void convert<1>(char*) { }            // ignore central byte

    template<typename T>
    void apply(T* val)
    {
        convert<sizeof(T)>(reinterpret_cast<char*>(val));
    }
}

#endif
