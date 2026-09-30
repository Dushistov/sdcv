#pragma once

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MMAP
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include <glib.h>

#include <utility>

class MapFile
{
public:
    MapFile() {}
    ~MapFile();
    MapFile(const MapFile &) = delete;
    MapFile &operator=(const MapFile &) = delete;
    // Movable so a mapping can be validated in a local and then handed to a
    // long-lived owner without being unmapped and remapped in between.
    MapFile(MapFile &&o) noexcept { swap(o); }
    MapFile &operator=(MapFile &&o) noexcept
    {
        if (this != &o) {
            MapFile tmp;      // takes our current mapping ...
            tmp.swap(*this);  // ... and unmaps it when it goes out of scope
            swap(o);
        }
        return *this;
    }
    bool open(const char *file_name, off_t file_size);
    // Tell the kernel this mapping is read by binary search, so a fault brings
    // in the page that was asked for instead of a whole readahead window.
    void advise_random();
    gchar *begin() { return data; }
    const gchar *begin() const { return data; }

private:
    void swap(MapFile &o) noexcept
    {
        std::swap(data, o.data);
#ifdef HAVE_MMAP
        std::swap(size, o.size);
#endif
    }

    char *data = nullptr;
#ifdef HAVE_MMAP
    size_t size = 0u;
#endif
};

inline bool MapFile::open(const char *file_name, off_t file_size)
{
#ifdef HAVE_MMAP
    const int fd = ::open(file_name, O_RDONLY);
    if (fd < 0) {
        // g_print("Open file %s failed!\n",fullfilename);
        return false;
    }
    struct stat st;
    if (fstat(fd, &st) == -1 || st.st_size < 0 || (st.st_size == 0 && S_ISREG(st.st_mode))
        || st.st_size != file_size) {
        close(fd);
        return false;
    }

    size = static_cast<size_t>(st.st_size);
    data = (gchar *)mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
    // The mapping keeps its own reference to the file; retaining the descriptor
    // would cost one open file per cached offset table and dictionary file.
    close(fd);
    if ((void *)data == (void *)(-1)) {
        // g_print("mmap file %s failed!\n",idxfilename);
        size = 0u;
        data = nullptr;
        return false;
    }
#elif defined(_WIN32)
    HANDLE file = CreateFile(file_name, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE,
                             nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    HANDLE mapping = CreateFileMapping(file, nullptr, PAGE_READONLY, 0, file_size, nullptr);
    CloseHandle(file);
    if (!mapping)
        return false;
    data = (gchar *)MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, file_size);
    CloseHandle(mapping);
    if (!data)
        return false;
#else
    gsize read_len;
    if (!g_file_get_contents(file_name, &data, &read_len, nullptr))
        return false;

    if (read_len != file_size)
        return false;
#endif

    return true;
}

inline void MapFile::advise_random()
{
#if defined(HAVE_MMAP) && defined(MADV_RANDOM)
    if (data && size)
        madvise(data, size, MADV_RANDOM);
#endif
}

inline MapFile::~MapFile()
{
    if (!data)
        return;
#ifdef HAVE_MMAP
    munmap(data, size);
#else
#ifdef _WIN32
    UnmapViewOfFile(data);
#else
    g_free(data);
#endif
#endif
}
