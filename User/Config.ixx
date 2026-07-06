module;
#include "Utils/Scan.h"

export module GW2Viewer.User.Config;
import GW2Viewer.Common;
import GW2Viewer.Common.GUID;
import GW2Viewer.Common.JSON;
import GW2Viewer.Common.Time;
import GW2Viewer.Data.Content;
import std;
#include "Macros.h"

export namespace GW2Viewer::User
{

struct Config
{
    bool Load();
    bool Save(bool crash = false);

    template<typename T>
    struct Bookmark
    {
        Time::Point Time;
        T Value;

        NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(Bookmark
            , Time
            , Value
        )

        std::strong_ordering operator<=>(Bookmark const&) const = default;
    };

    std::string GameExePath;
    std::string GameDatPath;
    std::string LocalDatPath;
    std::string DecryptionKeysPath;
    Language Language = Language::English;

    struct UI
    {
        bool ShowVoiceDecryptionStatusInText = false;

        struct Controls
        {
            struct FileButton
            {
                bool TooltipModelGrid = false;
                bool TooltipModelSkeleton = false;

                NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(FileButton
                    , TooltipModelGrid
                    , TooltipModelSkeleton
                )
            } FileButton;

            NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(Controls
                , FileButton
            )
        } Controls;

        struct Viewers
        {
            struct ContentListViewer
            {
                bool AutoExpandSearchResults = true;
                uint32 AutoExpandSearchMaxResults = 5;
                bool AutoOpenSearchResult = false;
                bool AutoOpenSearchResultInBackgroundTab = false;
                bool DrawTreeLines = true;
                bool DrawSeparatorsBetweenReleases = false;
                bool HorizontalScroll = false;
                bool HorizontalScrollAutoIndent = false;
                bool HorizontalScrollAutoContent = false;

                NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(ContentListViewer
                    , AutoExpandSearchResults
                    , AutoExpandSearchMaxResults
                    , AutoOpenSearchResult
                    , AutoOpenSearchResultInBackgroundTab
                    , DrawTreeLines
                    , DrawSeparatorsBetweenReleases
                    , HorizontalScroll
                    , HorizontalScrollAutoIndent
                    , HorizontalScrollAutoContent
                )
            } ContentListViewer;

            struct PackFileViewer
            {
                struct Model
                {
                    bool Grid = true;
                    bool Skeleton = true;

                    NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(Model
                        , Grid
                        , Skeleton
                    )
                } Model;

                NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(PackFileViewer
                    , Model
                )
            } PackFileViewer;

            NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(Viewers
                , ContentListViewer
                , PackFileViewer
            )
        } Viewers;

        struct Windows
        {
            struct ArchiveIndex
            {
                bool Backup = false;

                NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(ArchiveIndex
                    , Backup
                )
            } ArchiveIndex;

            NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(Windows
                , ArchiveIndex
            )
        } Windows;

        NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(UI
            , ShowVoiceDecryptionStatusInText

            , Controls
            , Viewers
            , Windows
        )
    } UI;

    struct Services
    {
        struct Export
        {
            bool ExportRawAlways = false;
            bool ExportRawIfNotConverted = true;
            bool ConvertTexture = true;
            bool ConvertSound = true;
            bool SkipExisting = false;

            NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(Export
                , ExportRawAlways
                , ExportRawIfNotConverted
                , ConvertTexture
                , ConvertSound
                , SkipExisting
            )
        } Export;

        NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(Services
            , Export
        )
    } Services;

    bool ShowImGuiDemo = false;
    bool ShowOriginalNames = false;
    bool ShowValidRawPointers = false;
    bool ShowContentSymbolNameBeforeType = false;
    bool TreeContentStructLayout = false;
    std::string Notes;
    std::wstring BruteforceDictionary;
    std::set<Bookmark<GUID>> BookmarkedContentObjects;
    std::map<uint32, std::map<uint32, std::string>> ConversationScriptedStartSituations;

    std::map<uint32, Data::Content::TypeInfo> TypeInfo;
    std::map<std::string, Data::Content::TypeInfo::StructLayout> SharedTypes;
    std::map<std::string, Data::Content::TypeInfo::Enum, std::less<>> SharedEnums;
    std::map<std::wstring, std::wstring> ContentNamespaceNames;
    std::map<GUID, std::wstring> ContentObjectNames;
    uint32 LastNumContentTypes = 0;

    NLOHMANN_DEFINE_TYPE_ORDERED_INTRUSIVE_WITH_DEFAULT(Config
        , GameExePath
        , GameDatPath
        , LocalDatPath
        , DecryptionKeysPath
        , Language

        , UI

        , ShowImGuiDemo
        , ShowOriginalNames
        , ShowValidRawPointers
        , ShowContentSymbolNameBeforeType
        , TreeContentStructLayout
        , Notes
        , BruteforceDictionary
        , BookmarkedContentObjects
        , ConversationScriptedStartSituations

        , TypeInfo
        , SharedTypes
        , SharedEnums
        , ContentNamespaceNames
        , ContentObjectNames
        , LastNumContentTypes
    )
    void FinishLoading()
    {
    }
};

}

export namespace GW2Viewer::G { User::Config Config; }
