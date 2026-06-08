#include "MPQ.h"

#include <cstdio>
#include <deque>
#include <format>
#include <iostream>

ArchiveSet gOpenArchives;

MPQArchive::MPQArchive(const char* filename)
{
    const int result = libmpq__archive_open(&mpq_a, filename, -1);
    std::cout << "Opening " << filename << std::endl;
    if (result)
    {
        switch (result)
        {
        case LIBMPQ_ERROR_OPEN:
            std::cout << std::format("Error opening archive '{}': Does file really exist?", filename);
            break;
        case LIBMPQ_ERROR_FORMAT:
            std::cout << std::format("Error opening archive '{}': Bad file format", filename);
            break;
        case LIBMPQ_ERROR_SEEK:
            std::cout << std::format("Error opening archive '{}': Seeking in file failed", filename);
            break;
        case LIBMPQ_ERROR_READ:
            std::cout << std::format("Error opening archive '{}': Read error in archive", filename);
            break;
        case LIBMPQ_ERROR_MALLOC:
            std::cout << std::format("Error opening archive '{}': Maybe not enough memory", filename);
            break;
        default:
            std::cout << std::format("Error opening archive '{}': Unknown error", filename);
            break;
        }
        std::cout << std::endl;
        return;
    }
    gOpenArchives.push_front(this);
}

void MPQArchive::close()
{
    libmpq__archive_close(mpq_a);
}

MPQFile::MPQFile(const char* filename): eof(false), buffer(nullptr), pointer(0), size(0)
{
    for (const auto& gOpenArchive : gOpenArchives)
    {
        mpq_archive* mpq_a = gOpenArchive->mpq_a;

        uint32_t fileNum;
        if (libmpq__file_number(mpq_a, filename, &fileNum)) continue;
        libmpq__off_t transferred;
        libmpq__file_unpacked_size(mpq_a, fileNum, &size);

        // HACK: in patch.mpq some files don't want to open and give 1 for filesize
        if (size <= 1)
        {
            eof = true;
            buffer = nullptr;
            return;
        }
        buffer = new char[size];

        libmpq__file_read(mpq_a, fileNum, reinterpret_cast<unsigned char*>(buffer), size, &transferred);
        return;
    }
    eof = true;
    buffer = nullptr;
}

std::size_t MPQFile::read(void* dest, std::size_t bytes)
{
    if (eof) return 0;

    const std::size_t rpos = pointer + bytes;
    if (rpos > static_cast<std::size_t>(size))
    {
        bytes = size - pointer;
        eof = true;
    }

    memcpy(dest, &buffer[pointer], bytes);
    pointer = rpos;
    return bytes;
}

void MPQFile::seek(const int offset)
{
    pointer = offset;
    eof = pointer >= size;
}

void MPQFile::seekRelative(const int offset)
{
    pointer += offset;
    eof = pointer >= size;
}

void MPQFile::close()
{
    delete[] buffer;
    buffer = nullptr;
    eof = true;
}

void OpenMPQFiles(const std::vector<std::string>& files)
{
    for (const auto& filename : files)
    {
        new MPQArchive(filename.c_str());
        if (const auto archive = new MPQArchive(filename.c_str()); gOpenArchives.empty() || gOpenArchives.front() != archive)
            delete archive;
    }
}

void CloseMPQFiles()
{
    for (const auto& gOpenArchive : gOpenArchives)
        gOpenArchive->close();
    gOpenArchives.clear();
}

bool ExtractMPQFile(const char* mpqName, const std::string& filename)
{
    FILE* output = fopen(filename.c_str(), "wb");
    if (!output)
        return false;

    if (MPQFile m(mpqName); !m.isEof())
        fwrite(m.getPointer(), 1, m.getSize(), output);

    fclose(output);
    return true;
}