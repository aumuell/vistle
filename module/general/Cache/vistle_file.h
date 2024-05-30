#ifndef VISTLE_CACHE_VISTLE_FILE_H
#define VISTLE_CACHE_VISTLE_FILE_H

#include <vistle/util/enum.h>
#include <vistle/core/shmname.h>
#include <vistle/core/message.h>

namespace vistle {

struct SubArchiveDirectoryEntry;

class ArchiveCompressionSettings {
public:
    virtual message::CompressionMode archiveCompression() const = 0;
    virtual int archiveCompressionSpeed() const = 0;
};

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ChunkType, (Invalid)(Directory)(PortObject)(Archive))

struct ChunkHeader {
    char Vistle[7] = "Vistle";
    char type = '\0';
    uint32_t version = 1;
    uint64_t size = 0;
};

struct ChunkFooter {
    uint64_t size = 0;
    char type = '\0';
    char Vistle[7] = "vistle";

    ChunkFooter() = default;
    ChunkFooter(const ChunkHeader &cheader): size(cheader.size), type(cheader.type) {}
};

struct PortObjectHeader {
    uint32_t version = 1;
    int32_t port = 0;
    int32_t timestep = -1;
    int32_t block = -1;
    shm_name_t object;

    PortObjectHeader() = default;
    PortObjectHeader(int port, int timestep, int block, const std::string &object)
    : port(port), timestep(timestep), block(block), object(object)
    {}
};

template<class T, typename FileDes>
bool Read(FileDes fd, T &t);
template<typename FileDes>
bool Read(FileDes fd, ChunkHeader &h);
#if 0
template<typename FileDes>
bool Read(FileDes fd, PortObjectHeader &h);
template<typenaame FileDes>
bool Read(FileDes fd, shm_name_t &name);
#endif

template<class Chunk, typename FileDes>
bool WriteChunk(ArchiveCompressionSettings *mod, FileDes fd, const Chunk &chunk);
template<typename FileDes>
bool WriteChunk(ArchiveCompressionSettings *, FileDes fd, SubArchiveDirectoryEntry const &);

template<class Chunk, typename FileDes>
bool ReadChunk(ArchiveCompressionSettings *mod, FileDes fd, const ChunkHeader &cheader, Chunk &chunk);
template<typename FileDes>
bool ReadChunk(ArchiveCompressionSettings *mod, FileDes fd, const ChunkHeader &cheader,
               SubArchiveDirectoryEntry &chunk);

template<typename FileDes>
bool SkipChunk(ArchiveCompressionSettings *mod, FileDes fd, const ChunkHeader &cheader);

} // namespace vistle
#endif
