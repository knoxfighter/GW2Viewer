module;
#include "dep/fmod/fmod.hpp"

module GW2Viewer.Data.Audio.Manager;
import GW2Viewer.Common.FourCC;
import GW2Viewer.Data.Game;
import GW2Viewer.Data.Encryption.Asset;
import GW2Viewer.Data.Encryption.RC4;
import GW2Viewer.Data.Pack.PackFile;
import GW2Viewer.Services.Export;

namespace GW2Viewer::Data::Audio
{

bool Manager::PlayVoice(uint32 voiceID, PlayOptions const& options)
{
    auto data = G::Game.Voice.Get(voiceID, options.Language);
    if (data.empty())
        return false;

    auto const system = GetOrCreateSystem();
    if (!system)
        return false;

    try
    {
        auto isPlaying = false;
        auto play = true;
        if (options.StopIsAlreadyPlaying && m_channel && m_channel->isPlaying(&isPlaying) && m_lastPlayedVoiceID == voiceID)
            play = false;
        Stop();

        FMOD_CREATESOUNDEXINFO info
        {
            .cbsize = sizeof(FMOD_CREATESOUNDEXINFO),
            .length = (uint32)data.size(),
        };
        FMOD::Sound* sound = nullptr;
        std::vector<byte> encrypted;
        if (system->createSound((char const*)data.data(), FMOD_OPENMEMORY, &info, &sound) != FMOD_OK)
        {
            auto const key = G::Game.Encryption.GetAssetKey(Data::Encryption::AssetType::Voice, voiceID);
            if (!key)
                return false;

            encrypted.assign_range(data);
            Encryption::RC4(Encryption::RC4::MakeKey(*key)).Crypt(encrypted);
            if (system->createSound((char const*)encrypted.data(), FMOD_OPENMEMORY, &info, &sound) != FMOD_OK)
                return false;

            data = encrypted;
        }

        auto result = false;
        if (options.Export)
        {
            std::filesystem::path const path = std::format(R"(Export\Voice\{}\{}.mp3)", options.Language, voiceID);
            if (!options.ExportSkipExisting || !((result = exists(path))))
                result = G::Services::Export.Data(data, path);
        }

        if (options.Play && play)
        {
            if (system->playSound(sound, nullptr, false, &m_channel) == FMOD_OK)
            {
                m_lastPlayedVoiceID = voiceID;
                result = true;
            }
        }
        return result;
    }
    catch (...) { return false; }
}

bool Manager::PlayFile(uint32 fileID, PlayOptions const& options)
{
    std::unique_ptr<Pack::PackFile> file;
    if (!options.DataSource.empty())
        file = std::make_unique<Pack::PackFile>(options.DataSource);
    else
        file = G::Game.Archive.GetPackFile(fileID);
    if (!file || !file->IsValid() || !file->HasChunk(fcc::ASND))
        return false;

    auto const chunk = file->QueryChunk(fcc::ASND);
    byte format = chunk["format"];
    auto const audioData = chunk["audioData[]"];
    std::span data { audioData.GetPointer(), audioData.GetArraySize() };
    if (data.empty())
        return false;

    auto const system = GetOrCreateSystem();
    if (!system)
        return false;

    try
    {
        auto isPlaying = false;
        auto play = true;
        if (options.StopIsAlreadyPlaying && m_channel && m_channel->isPlaying(&isPlaying) && m_lastPlayedFileID == fileID)
            play = false;
        Stop();

        FMOD_CREATESOUNDEXINFO info
        {
            .cbsize = sizeof(FMOD_CREATESOUNDEXINFO),
            .length = (uint32)data.size(),
        };
        FMOD::Sound* sound = nullptr;
        if (system->createSound((char const*)data.data(), FMOD_OPENMEMORY, &info, &sound) != FMOD_OK)
            return false;

        auto result = false;
        if (options.Export)
        {
            std::filesystem::path const path = std::format(R"(Export\{}.{})", fileID, format == 0 ? "wav" : format == 1 ? "mp3" : format == 2 ? "ogg" : "unk");
            if (!options.ExportSkipExisting || !((result = exists(path))))
                result = G::Services::Export.Data(data, path);
        }

        if (options.Play && play)
        {
            if (system->playSound(sound, nullptr, false, &m_channel) == FMOD_OK)
            {
                m_lastPlayedFileID = fileID;
                result = true;
            }
        }
        return result;
    }
    catch (...) { return false; }
}

void Manager::Stop()
{
    if (m_channel)
        std::exchange(m_channel, nullptr)->stop();
}

FMOD::System* Manager::GetOrCreateSystem()
{
    FMOD::System* system = nullptr;
    if (System_Create(&system) != FMOD_OK)
        return nullptr;
    m_system.reset(system);
    if (m_system->init(32, FMOD_INIT_NORMAL, nullptr) != FMOD_OK)
        return nullptr;
    return m_system.get();
}

}
