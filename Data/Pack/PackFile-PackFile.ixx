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

struct PackFile;
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

    static bool IsAmbiguousFourCC(fcc fcc)
    {
        switch (fcc)
        {
            case fcc::Main:
                return true;
            default:
                return false;
        }
    }
    fcc GetFourCC() const { return Header.Magic; }
    fcc GetDisambiguatedFourCC(PackFile const& file) const;
    std::string_view GetFourCCStringView() const { return { (char const*)&Header.Magic, strnlen((char const*)&Header.Magic, 4) }; }
    std::string GetFourCCString() const { return std::string { GetFourCCStringView() }; }

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

    PackFile(uint32 size) : m_ownsData(true), m_data(new byte const[size + sizeof(PackFileChunk::ChunkHeader)] { }, size) { }
    PackFile(std::span<byte const> data) : m_ownsData(false), m_data(data) { }
    PackFile(PackFile const&) = delete;
    PackFile(PackFile&&) = delete;
    ~PackFile() { if (m_ownsData) delete m_data.data(); }

    void FinishLoading();

    FileHeader const& GetHeader() const { return *(FileHeader const*)m_data.data(); }
    std::span<byte const> GetData() const { return { m_data.data() + sizeof(FileHeader), m_data.size() - sizeof(FileHeader) }; }
    std::span<byte const> GetRawData() const { return m_data; }
    std::span<byte> GetRawWritableData() { if (!m_ownsData) std::terminate(); return { (byte*)m_data.data(), m_data.size() }; }

    fcc GetFourCC() const { return GetHeader().ContentType; }
    std::string_view GetFourCCStringView() const { return { (char const*)&GetHeader().ContentType, strnlen((char const*)&GetHeader().ContentType, 4) }; }
    std::string GetFourCCString() const { return std::string { GetFourCCStringView() }; }

    bool Is64Bit() const { return GetHeader().Is64Bit; }

    auto& CreateLayout() { return *(m_layout = std::make_unique<Layout::Container>()); }
    auto GetLayout() const { return m_layout.get(); }

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

    [[nodiscard]] bool HasChunk(fcc fcc) const
    {
        for (auto const& chunk : *this)
            if (chunk.GetFourCC() == fcc)
                return true;
        return false;
    }

    [[nodiscard]] PackFileChunk const& GetChunk(fcc fcc) const
    {
        for (auto const& chunk : *this)
            if (chunk.GetFourCC() == fcc)
                return chunk;
        std::terminate();
    }
    [[nodiscard]] Layout::Traversal::QueryChunk QueryChunk(fcc magic) const;

    [[nodiscard]] ChunkIterator begin() { return (PackFileChunk*)GetData().data(); }
    [[nodiscard]] ConstChunkIterator begin() const { return (PackFileChunk const*)GetData().data(); }
    [[nodiscard]] ChunkIterator end() { return (PackFileChunk*)(GetData().data() + GetData().size()); }
    [[nodiscard]] ConstChunkIterator end() const { return (PackFileChunk const*)(GetData().data() + GetData().size()); }

private:
    bool m_ownsData;
    std::span<byte const> m_data;
    std::unique_ptr<Layout::Container> m_layout;
};

fcc PackFileChunk::GetDisambiguatedFourCC(PackFile const& file) const
{
    auto const fcc = GetFourCC();
    return IsAmbiguousFourCC(fcc) ? file.GetFourCC() : fcc;
}

}
