export module GW2Viewer.Data.Pack.Manager;
import GW2Viewer.Common;
import GW2Viewer.Data.Pack.PackFile;
import GW2Viewer.Utils.Async.ProgressBarContext;
import GW2Viewer.Utils.Container;
import std;
import <string.h>;

export namespace GW2Viewer::Data::Pack
{

class Manager
{
public:
    auto GetChunkVersions(PackFile const& file, PackFileChunk const& chunk)
    {
        std::string_view const fcc { (char const*)&chunk.Header.Magic, strnlen((char const*)&chunk.Header.Magic, 4) };
        LayoutContainer* container;
        {
            std::scoped_lock _(m_embeddedLayoutsLock);
            container = Utils::Container::Find(m_embeddedLayouts, &file);
        }
        if (container && !container->Chunks.empty())
            return Utils::Container::Find(container->Chunks, fcc);

        return Utils::Container::Find(m_layout.Chunks, fcc);
    }
    Layout::Type const* GetChunkType(PackFile const& file, PackFileChunk const& chunk)
    {
        if (auto const versions = GetChunkVersions(file, chunk))
            if (auto const itr = versions->find(chunk.Header.Version); itr != versions->end())
                return itr->second;
        return nullptr;
    }

    void Load(std::filesystem::path const& path, Utils::Async::ProgressBarContext& progress);
    bool IsLoaded() const { return m_loaded; }

    void LoadEmbeddedLayout(PackFile const& file, PackFileChunk const& chunk)
    {
        LayoutContainer* container;
        bool load;
        {
            std::scoped_lock _(m_embeddedLayoutsLock);
            auto&& [itr, added] = m_embeddedLayouts.try_emplace(&file);
            container = &itr->second;
            load = added;
        }
        if (load)
            LoadEmbeddedLayout(*container, file, chunk);
    }
    void DeleteEmbeddedLayout(PackFile const& file)
    {
        std::scoped_lock _(m_embeddedLayoutsLock);
        m_embeddedLayouts.erase(&file);
    }

private:
    bool m_loaded = false;
    struct LayoutContainer
    {
        std::unordered_map<byte const*, Layout::Type> Types;
        std::map<std::string, std::map<uint32, Layout::Type const*>, std::less<>> Chunks;
    };
    LayoutContainer m_layout;
    std::unordered_map<PackFile const*, LayoutContainer> m_embeddedLayouts;
    std::mutex m_embeddedLayoutsLock;
    void LoadEmbeddedLayout(LayoutContainer& container, PackFile const& file, PackFileChunk const& chunk);
};

}
