export module GW2Viewer.Data.Pack.Manager;
import GW2Viewer.Common;
import GW2Viewer.Common.FourCC;
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
        if (auto const layout = file.GetLayout(); layout && !layout->Chunks.empty())
            return Utils::Container::Find(layout->Chunks, chunk.GetDisambiguatedFourCC(file));

        return Utils::Container::Find(m_layout.Chunks, chunk.GetDisambiguatedFourCC(file));
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

    void LoadEmbeddedLayout(PackFile& file, PackFileChunk const& chunk);

private:
    bool m_loaded = false;
    Layout::Container m_layout;
};

}
