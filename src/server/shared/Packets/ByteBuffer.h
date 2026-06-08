#ifndef BYTE_BUFFER_H
#define BYTE_BUFFER_H

#include <array>
#include <cstring>
#include <string>
#include <vector>

#include "Define.h"

class MessageBuffer;

// Root of ByteBuffer exception hierarchy
class ByteBufferException : public std::exception
{
public:
    ~ByteBufferException() noexcept override = default;

    [[nodiscard]] char const* what() const noexcept override { return msg_.c_str(); }

protected:
    std::string & message() noexcept { return msg_; }

private:
    std::string msg_;
};

class ByteBufferPositionException : public ByteBufferException
{
public:
    ByteBufferPositionException(bool add, std::size_t pos, std::size_t size, std::size_t valueSize);

    ~ByteBufferPositionException() noexcept override = default;
};

class ByteBufferSourceException : public ByteBufferException
{
public:
    ByteBufferSourceException(std::size_t pos, std::size_t size, std::size_t valueSize);

    ~ByteBufferSourceException() noexcept override = default;
};

class ByteBufferInvalidValueException : public ByteBufferException
{
public:
    ByteBufferInvalidValueException(char const* type, char const* value);

    ~ByteBufferInvalidValueException() noexcept override = default;
};

class ByteBuffer
{
public:
    constexpr static std::size_t DEFAULT_SIZE = 0x1000;

    ByteBuffer()
    {
        _storage.reserve(DEFAULT_SIZE);
    }

    explicit ByteBuffer(const std::size_t reserve)
    {
        _storage.reserve(reserve);
    }

    ByteBuffer(ByteBuffer&& buf) noexcept : _rpos(buf._rpos), _wpos(buf._wpos), _storage(std::move(buf._storage))
    {
        buf._rpos = 0;
        buf._wpos = 0;
    }

    ByteBuffer(ByteBuffer const& right) = default;
    explicit ByteBuffer(MessageBuffer&& buffer);
    virtual ~ByteBuffer() = default;

    ByteBuffer& operator=(ByteBuffer const& right)
    {
        if (this != &right)
        {
            _rpos = right._rpos;
            _wpos = right._wpos;
            _storage = right._storage;
        }

        return *this;
    }

    ByteBuffer& operator=(ByteBuffer&& right) noexcept
    {
        if (this != &right)
        {
            _rpos = right._rpos;
            right._rpos = 0;
            _wpos = right._wpos;
            right._wpos = 0;
            _storage = std::move(right._storage);
        }

        return *this;
    }

    void clear()
    {
        _storage.clear();
        _rpos = _wpos = 0;
    }

    template <typename T>
    void append(T value)
    {
        static_assert(std::is_fundamental_v<T>, "append(compound)");
        append(reinterpret_cast<uint8*>(&value), sizeof(value));
    }

    template <typename T>
    void put(const std::size_t pos, T value)
    {
        static_assert(std::is_fundamental_v<T>, "append(compound)");
        put(pos, reinterpret_cast<uint8*>(&value), sizeof(value));
    }

    ByteBuffer& operator<<(const bool value)
    {
        append<uint8>(value ? 1 : 0);
        return *this;
    }

    ByteBuffer& operator<<(const uint8 value)
    {
        append<uint8>(value);
        return *this;
    }

    ByteBuffer& operator<<(const uint16 value)
    {
        append<uint16>(value);
        return *this;
    }

    ByteBuffer& operator<<(const uint32 value)
    {
        append<uint32>(value);
        return *this;
    }

    ByteBuffer& operator<<(const uint64 value)
    {
        append<uint64>(value);
        return *this;
    }

    // signed as in 2e complement
    ByteBuffer& operator<<(const int8 value)
    {
        append<int8>(value);
        return *this;
    }

    ByteBuffer& operator<<(const int16 value)
    {
        append<int16>(value);
        return *this;
    }

    ByteBuffer& operator<<(const int32 value)
    {
        append<int32>(value);
        return *this;
    }

    ByteBuffer& operator<<(const int64 value)
    {
        append<int64>(value);
        return *this;
    }

    // floating points
    ByteBuffer& operator<<(const float value)
    {
        append<float>(value);
        return *this;
    }

    ByteBuffer& operator<<(const double value)
    {
        append<double>(value);
        return *this;
    }

    ByteBuffer& operator<<(const std::string_view value)
    {
        if (const std::size_t len = value.length())
            append(reinterpret_cast<uint8 const*>(value.data()), len);

        append(static_cast<uint8>(0));
        return *this;
    }

    ByteBuffer& operator<<(const std::string& str)
    {
        return operator<<(std::string_view(str));
    }

    ByteBuffer& operator<<(const char* str)
    {
        return operator<<(std::string_view(str ? str : ""));
    }

    ByteBuffer& operator>>(bool& value)
    {
        value = read<char>() > 0;
        return *this;
    }

    ByteBuffer& operator>>(uint8& value)
    {
        value = read<uint8>();
        return *this;
    }

    ByteBuffer& operator>>(uint16& value)
    {
        value = read<uint16>();
        return *this;
    }

    ByteBuffer& operator>>(uint32& value)
    {
        value = read<uint32>();
        return *this;
    }

    ByteBuffer& operator>>(uint64& value)
    {
        value = read<uint64>();
        return *this;
    }

    //signed as in 2e complement
    ByteBuffer& operator>>(int8& value)
    {
        value = read<int8>();
        return *this;
    }

    ByteBuffer& operator>>(int16& value)
    {
        value = read<int16>();
        return *this;
    }

    ByteBuffer& operator>>(int32& value)
    {
        value = read<int32>();
        return *this;
    }

    ByteBuffer& operator>>(int64& value)
    {
        value = read<int64>();
        return *this;
    }

    ByteBuffer& operator>>(float& value);
    ByteBuffer& operator>>(double& value);

    ByteBuffer& operator>>(std::string& value)
    {
        value = ReadCString(true);
        return *this;
    }

    uint8& operator[](std::size_t const pos)
    {
        if (pos >= size())
            throw ByteBufferPositionException(false, pos, 1, size());

        return _storage[pos];
    }

    uint8 const& operator[](std::size_t const pos) const
    {
        if (pos >= size())
            throw ByteBufferPositionException(false, pos, 1, size());

        return _storage[pos];
    }

    [[nodiscard]] std::size_t rpos() const { return _rpos; }

    std::size_t rpos(std::size_t rpos_)
    {
        _rpos = rpos_;
        return _rpos;
    }

    void rFinish()
    {
        _rpos = wpos();
    }

    [[nodiscard]] std::size_t wpos() const { return _wpos; }

    std::size_t wpos(const std::size_t wpos_)
    {
        _wpos = wpos_;
        return _wpos;
    }

    template<typename T>
    void read_skip() { read_skip(sizeof(T)); }

    void read_skip(const std::size_t skip)
    {
        if (_rpos + skip > size())
            throw ByteBufferPositionException(false, _rpos, skip, size());

        _rpos += skip;
    }

    template <typename T> T read()
    {
        T r = read<T>(_rpos);
        _rpos += sizeof(T);
        return r;
    }

    template <typename T>
    [[nodiscard]] T read(const std::size_t pos) const
    {
        if (pos + sizeof(T) > size())
            throw ByteBufferPositionException(false, pos, sizeof(T), size());
        return *reinterpret_cast<T const*>(&_storage[pos]);
    }

    void read(uint8* dest, const std::size_t len)
    {
        if (_rpos  + len > size())
            throw ByteBufferPositionException(false, _rpos, len, size());

        std::memcpy(dest, &_storage[_rpos], len);
        _rpos += len;
    }

    template <std::size_t Size>
    void read(std::array<uint8, Size>& arr)
    {
        read(arr.data(), Size);
    }

    void readPackGUID(uint64& guid)
    {
        if (rpos() + 1 > size())
            throw ByteBufferPositionException(false, _rpos, 1, size());

        guid = 0;

        uint8 guidMark = 0;
        *this >> guidMark;

        for (int i = 0; i < 8; ++i)
        {
            if (guidMark & (static_cast<uint8>(1) << i))
            {
                if (rpos() + 1 > size())
                    throw ByteBufferPositionException(false, _rpos, 1, size());

                uint8 bit;
                *this >> bit;
                guid |= (static_cast<uint64>(bit) << (i * 8));
            }
        }
    }

    std::string ReadCString(bool requireValidUtf8 = true);
    uint32 ReadPackedTime();

    ByteBuffer& ReadPackedTime(uint32& time)
    {
        time = ReadPackedTime();
        return *this;
    }

    uint8* contents()
    {
        if (_storage.empty())
            throw ByteBufferException();

        return _storage.data();
    }

    [[nodiscard]] uint8 const* contents() const
    {
        if (_storage.empty())
            throw ByteBufferException();

        return _storage.data();
    }

    [[nodiscard]] std::size_t size() const { return _storage.size(); }
    [[nodiscard]] bool empty() const { return _storage.empty(); }

    void resize(const std::size_t newSize)
    {
        _storage.resize(newSize, 0);
        _rpos = 0;
        _wpos = size();
    }

    void reserve(const std::size_t resSize)
    {
        if (resSize > size())
            _storage.reserve(resSize);
    }

    void shrink_to_fit()
    {
        _storage.shrink_to_fit();
    }

    void append(char const* src, const std::size_t cnt)
    {
        return append(reinterpret_cast<const uint8*>(src), cnt);
    }

    template<class T> void append(const T* src, const std::size_t cnt)
    {
        return append(reinterpret_cast<const uint8*>(src), cnt * sizeof(T));
    }

    void append(uint8 const* src, std::size_t cnt);

    void append(ByteBuffer const& buffer)
    {
        if (buffer.wpos())
        {
            append(buffer.contents(), buffer.wpos());
        }
    }

    template <std::size_t Size>
    void append(std::array<uint8, Size> const& arr)
    {
        append(arr.data(), Size);
    }

    // Can be used in SMSG_MONSTER_MOVE opcode
    void appendPackXYZ(const float x, const float y, const float z)
    {
        uint32 packed = 0;
        packed |= static_cast<int>(x / 0.25f) & 0x7FF;
        packed |= (static_cast<int>(y / 0.25f) & 0x7FF) << 11;
        packed |= (static_cast<int>(z / 0.25f) & 0x3FF) << 22;
        *this << packed;
    }

    void appendPackGUID(uint64 guid)
    {
        uint8 packGUID[8 + 1];
        packGUID[0] = 0;
        std::size_t size = 1;

        for (uint8 i = 0; guid != 0;++i)
        {
            if (guid & 0xFF)
            {
                packGUID[0] |= static_cast<uint8>(1 << i);
                packGUID[size] =  static_cast<uint8>(guid & 0xFF);
                ++size;
            }

            guid >>= 8;
        }

        append(packGUID, size);
    }

    void AppendPackedTime(time_t time);
    void put(std::size_t pos, uint8 const* src, std::size_t cnt);
    void print_storage() const;
    void textLike() const;
    void hexLike() const;

protected:
    std::size_t _rpos{0}, _wpos{0};
    std::vector<uint8> _storage;
};
#endif
