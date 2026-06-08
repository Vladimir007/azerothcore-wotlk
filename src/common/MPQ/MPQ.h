#ifndef NCORE_MPQ_H
#define NCORE_MPQ_H

#include <cstring>
#include <deque>
#include <vector>
#include <libmpq/mpq.h>

#include "loadlib.h"

using namespace std;

class MPQArchive
{
public:
    mpq_archive_s* mpq_a;

    explicit MPQArchive(const char* filename);
    ~MPQArchive() { close(); }
    void close();

    void GetFileListTo(vector<string>& filelist)
    {
        uint32_t fileNum;
        if (libmpq__file_number(mpq_a, "(listfile)", &fileNum)) return;
        libmpq__off_t size, transferred;
        libmpq__file_unpacked_size(mpq_a, fileNum, &size);

        const auto buffer = new char[size + 1];
        buffer[size] = '\0';

        libmpq__file_read(mpq_a, fileNum, reinterpret_cast<unsigned char*>(buffer), size, &transferred);

        constexpr char seps[] = "\n";

        char* token = strtok(buffer, seps);
        uint32 counter = 0;
        while (token != nullptr && counter < size)
        {
            token[strlen(token) - 1] = 0;
            string s = token;
            filelist.push_back(s);
            counter += strlen(token) + 2;
            token = strtok(nullptr, seps);
        }

        delete[] buffer;
    }
};

typedef std::deque<MPQArchive*> ArchiveSet;

extern ArchiveSet gOpenArchives;

void OpenMPQFiles(const std::vector<std::string>& files);
void CloseMPQFiles();
bool ExtractMPQFile(const char* mpqName, const std::string& filename);

class MPQFile
{
    bool eof;
    char* buffer;
    libmpq__off_t pointer, size;

public:
    explicit MPQFile(const char* filename);
    ~MPQFile() { close(); }
    std::size_t read(void* dest, std::size_t bytes);
    std::size_t getSize() { return size; }
    std::size_t getPos() { return pointer; }
    char* getBuffer() { return buffer; }
    char* getPointer() { return buffer + pointer; }
    bool isEof() { return eof; }
    void seek(int offset);
    void seekRelative(int offset);
    void close();
};

inline void flipMagic(char* magic)
{
    char t = magic[0];
    magic[0] = magic[3];
    magic[3] = t;
    t = magic[1];
    magic[1] = magic[2];
    magic[2] = t;
}

#endif
