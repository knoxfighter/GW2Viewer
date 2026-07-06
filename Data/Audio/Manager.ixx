module;
#include "dep/fmod/fmod.hpp"

export module GW2Viewer.Data.Audio.Manager;
import GW2Viewer.Common;
import GW2Viewer.User.Config;
import std;

export namespace GW2Viewer::Data::Audio
{

struct Manager
{
    struct PlayOptions
    {
        std::span<byte const> DataSource;
        bool Play = true;
        bool StopIsAlreadyPlaying = true;
        bool Export = false;
        bool ExportSkipExisting = false;
        Language Language = G::Config.Language;
    };
    bool PlayVoice(uint32 voiceID, PlayOptions const& options = { });
    bool PlayFile(uint32 fileID, PlayOptions const& options = { });

    void Stop();

private:
    std::unique_ptr<FMOD::System, decltype([](FMOD::System* system) { system->release(); })> m_system;
    FMOD::Channel* m_channel = nullptr;
    uint32 m_lastPlayedVoiceID = 0;
    uint32 m_lastPlayedFileID = 0;
    FMOD::System* GetOrCreateSystem();
};

}
