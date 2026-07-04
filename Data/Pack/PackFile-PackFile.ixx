export module GW2Viewer.Data.Pack.PackFile:PackFile;
export import :Layout;
import GW2Viewer.Common;
import GW2Viewer.Common.FourCC;
import std;
import <cstddef>;
import <cstring>;

export namespace GW2Viewer::Data::Pack
{
namespace Layout::Traversal
{
struct QueryChunk;
}

#pragma pack(push, 1)
struct PackFileChunk
{
    struct ChunkHeader
    {
        fcc Magic;
        uint32 NextChunkOffset;
        uint16 Version;
        uint16 HeaderSize;
        uint32 FixupsOffset;
    };
    struct Fixups
    {
        uint32 Count;
        uint32 Offsets[];
    };
    struct ChunkFooter
    {
        uint32 UnkOffset;
        uint32 LayoutOffset;
        uint16 Version;
        uint16 FooterSize;
        fcc Magic;
    };

    ChunkHeader Header;
    byte Data[];

    PackFileChunk() = delete;
    PackFileChunk(PackFileChunk const&) = delete;
    PackFileChunk(PackFileChunk&&) = delete;

    PackFileChunk const* GetNextChunk() const { return (PackFileChunk const*)((byte const*)this + offsetof(PackFileChunk::ChunkHeader, NextChunkOffset) + sizeof(Header.NextChunkOffset) + Header.NextChunkOffset); }
    std::span<uint32 const> GetFixupOffsets() const
    {
        auto const fixups = (Fixups const*)((byte const*)this + offsetof(PackFileChunk::ChunkHeader, FixupsOffset) + sizeof(Header.FixupsOffset) + Header.FixupsOffset);
        return { fixups->Offsets, fixups->Count };
    }
    ChunkFooter const* GetFooter() const
    {
        auto const footer = (ChunkFooter const*)GetNextChunk() - 1;
        return footer->Magic == fcc::FOOT && !footer->Version && footer->FooterSize == sizeof(ChunkFooter) ? footer : nullptr;
    }
};
struct PackFile
{
    struct FileHeader
    {
        char Magic[2]; // signature PACKFILE_SIGNATURE
        uint16 FlagUnk1 : 1;
        uint16 FlagUnk2 : 1;
        uint16 Is64Bit : 1;
        uint16 Zero;
        uint16 HeaderSize;
        fcc ContentType;
    };

    static PackFile* Alloc(size_t size)
    {
        auto* file = (PackFile*)operator new(size + sizeof(PackFileChunk::ChunkHeader));
        memset(file, 0, size + sizeof(PackFileChunk::ChunkHeader));
        return file;
    }

    FileHeader Header; // hdr
    byte Data[];

    PackFile() = delete;
    PackFile(PackFile const&) = delete;
    PackFile(PackFile&&) = delete;
    ~PackFile();

    template<typename T>
    class ChunkIteratorBase
    {
        T* m_pos;

    public:
        ChunkIteratorBase(T* pos) : m_pos(pos) { }
        ChunkIteratorBase(ChunkIteratorBase const&) = default;
        ChunkIteratorBase(ChunkIteratorBase&&) = default;
        ChunkIteratorBase& operator=(ChunkIteratorBase const&) = default;
        ChunkIteratorBase& operator=(ChunkIteratorBase&&) = default;

        bool operator==(ChunkIteratorBase const&) const = default;
        ChunkIteratorBase& operator++() { m_pos = const_cast<T*>(m_pos->GetNextChunk()); return *this; }
        ChunkIteratorBase operator++(int) { auto copy = *this; ++*this; return copy; }

        T& operator*() const { return *m_pos; }
        T* operator->() const { return m_pos; }
    };
    using ChunkIterator = ChunkIteratorBase<PackFileChunk>;
    using ConstChunkIterator = ChunkIteratorBase<PackFileChunk const>;

    [[nodiscard]] bool HasChunk(fcc magic) const
    {
        for (auto const& chunk : *this)
            if (chunk.Header.Magic == magic)
                return true;
        return false;
    }

    [[nodiscard]] PackFileChunk const& GetFirstChunk() const { return *begin(); }
    [[nodiscard]] PackFileChunk const& GetChunk(fcc magic) const
    {
        for (auto itr = begin(); ; ++itr)
            if (itr->Header.Magic == magic)
                return *itr;
    }
    [[nodiscard]] Layout::Traversal::QueryChunk QueryChunk(fcc magic) const;

    [[nodiscard]] ChunkIterator begin() { return (PackFileChunk*)&Data; }
    [[nodiscard]] ConstChunkIterator begin() const { return (PackFileChunk const*)&Data; }
    [[nodiscard]] ChunkIterator end()
    {
        auto itr = begin();
        while (itr->Header.Magic != fcc::Empty)
            ++itr;
        return itr;
    }
    [[nodiscard]] ConstChunkIterator end() const
    {
        auto itr = begin();
        while (itr->Header.Magic != fcc::Empty)
            ++itr;
        return itr;
    }
};
#pragma pack(pop)

}
