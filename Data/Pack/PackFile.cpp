module GW2Viewer.Data.Pack.PackFile;
import :Traversal;
import GW2Viewer.Data.Game;
import std;

namespace GW2Viewer::Data::Pack
{

PackFile::~PackFile()
{
    G::Game.Pack.DeleteEmbeddedLayout(*this);
}

Layout::Traversal::QueryChunk PackFile::QueryChunk(fcc magic) const { return { *this, GetChunk(magic) }; }

}

namespace GW2Viewer::Data::Pack::Layout::Traversal
{

Type const* GetChunkType(PackFile const& file, PackFileChunk const& chunk) { return G::Game.Pack.GetChunkType(file, chunk); }

}
