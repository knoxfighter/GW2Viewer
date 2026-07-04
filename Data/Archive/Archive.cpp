module GW2Viewer.Data.Archive;
import GW2Viewer.Data.Game;

namespace GW2Viewer::Data::Archive
{

std::unique_ptr<Pack::PackFile> Archive::GetPackFile(uint32 fileID)
{
    std::unique_ptr<Pack::PackFile> result;
    if (auto size = GetFileSize(fileID))
    {
        result.reset(Pack::PackFile::Alloc(size));
        GetFile(fileID, { (byte*)result.get(), size });
        for (auto const& chunk : *result)
            G::Game.Pack.LoadEmbeddedLayout(*result, chunk);
    }
    return result;
}

}
