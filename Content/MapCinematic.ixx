export module GW2Viewer.Content.MapCinematic;
import GW2Viewer.Common;
import GW2Viewer.Common.GUID;
import GW2Viewer.Common.Time;
import GW2Viewer.Data.External.Types;
import GW2Viewer.Data.Game;
import GW2Viewer.UI.ImGui;
import GW2Viewer.Utils.Encoding;
import GW2Viewer.Utils.String;
import std;

export namespace GW2Viewer::Content
{

struct MapCinematic
{
    struct Object
    {
        uint32 TextID;
        ImVec2 ContinentPosition;
        int32 Floor;
        GUID ContentGUID;
    };
    static_assert(sizeof(Object) == 32);
    struct Group
    {
        uint32 TextID;
        uint32 MaleVoiceID;
        uint32 FemaleVoiceID;
        Data::External::DBBlobElementSpan<Object> Objects;
        static_assert(sizeof(Objects) == 4);
    };
    struct Sector
    {
        uint32 DataID;
    };

    uint32 UID;
    uint32 Flags;
    std::vector<Group> Groups;
    std::vector<Object> Objects;
    std::vector<Sector> Sectors;

    mutable Data::External::Encounter Encounter;

    std::string Text() const
    {
        for (auto const& group : Groups)
        {
            if (auto const string = G::Game.Text.Get(group.TextID).first)
                return Utils::String::SingleLined(Utils::Encoding::ToUTF8(*string));

            for (auto const& object : group.Objects.GetSpan({ Objects.data(), Objects.size() })) // Workaround for internal compiler error in msvc, should be GetSpan(Objects) otherwise
                if (auto const string = G::Game.Text.Get(object.TextID).first)
                    return Utils::String::SingleLined(Utils::Encoding::ToUTF8(*string));
        }
        return { };
    }
};
std::map<uint64, MapCinematic> mapCinematics;
std::shared_mutex mapCinematicsLock;

}
