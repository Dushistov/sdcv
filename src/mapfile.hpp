#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MMAP
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include <glib.h>

class MapFile
{
public:
    MapFile() {}
    ~MapFile();
    MapFile(const MapFile &) = delete;
    MapFile &operator=(const MapFile &) = delete;
    bool open(const char *file_name, unsigned long file_size);
    gchar *begin() { return data; }

private:
    char *data = nullptr;
    unsigned long size = 0ul;
#ifdef HAVE_MMAP
    int mmap_fd = -1;
#elif defined(_WIN32)
    HANDLE hFile = 0;
    HANDLE hFileMap = 0;
#endif
};

inline bool MapFile::open(const char *file_name, unsigned long file_size)
{
    size = file_size;
#ifdef HAVE_MMAP
    if ((mmap_fd = ::open(file_name, O_RDONLY)) < 0) {
        // g_print("Open file %s failed!\n",fullfilename);
        return false;
    }
    struct stat st;
    if (fstat(mmap_fd, &st) == -1 || st.st_size < 0 || (st.st_size == 0 && S_ISREG(st.st_mode))
        || sizeof(st.st_size) > sizeof(file_size) || static_cast<unsigned long>(st.st_size) != file_size) {
        close(mmap_fd);
        return false;
    }

    data = (gchar *)mmap(nullptr, file_size, PROT_READ, MAP_SHARED, mmap_fd, 0);
    if ((void *)data == (void *)(-1)) {
        // g_print("mmap file %s failed!\n",idxfilename);
        data = nullptr;
        return false;
    }
#elif defined(_WIN32)
    hFile = CreateFile(file_name, GENERIC_READ, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    hFileMap = CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, file_size, nullptr);
    data = (gchar *)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, file_size);
#else
    gsize read_len;
    if (!g_file_get_contents(file_name, &data, &read_len, nullptr))
        return false;

    if (read_len != file_size)
        return false;
#endif

    return true;
}

inline MapFile::~MapFile()
{
    if (!data)
        return;
#ifdef HAVE_MMAP
    munmap(data, size);
    close(mmap_fd);
#else
#ifdef _WIN32
    UnmapViewOfFile(data);
    CloseHandle(hFileMap);
    CloseHandle(hFile);
#else
    g_free(data);
#endif
#endif
}
