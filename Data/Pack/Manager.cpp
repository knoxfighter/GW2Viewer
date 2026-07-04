module GW2Viewer.Data.Pack.Manager;
import GW2Viewer.Utils.ScanPE;
import <cctype>;

namespace GW2Viewer::Data::Pack
{

void Manager::Load(std::filesystem::path const& path, Utils::Async::ProgressBarContext& progress)
{
    progress.Start(std::format("Parsing PackFile layouts from {}", path.filename().string()));
    using namespace Layout;
    Utils::ScanPE::Scanner scanner { path };

    struct PackFileField
    {
        UnderlyingTypes UnderlyingType;
        RealTypes RealType;
        uint32 Unk;
        char const* Name;
        union
        {
            PackFileField* ElementFields;
            PackFileField** VariantElementFields;
            void(*PostProcessStruct)();
        };
        uint16 Size;
    };
    struct PackFileVersion
    {
        PackFileField* Fields;
        void* PostProcessFunction;
        void* Unk;
    };
    auto collect = [&](this auto&& collect, PackFileField* fields) -> Type const*
    {
        if (!fields)
            return nullptr;
        PackFileField* field = fields;
        while (true)
        {
            if (!scanner.rdata.Valid(field->Name))
                return nullptr;
            if (field->UnderlyingType != UnderlyingTypes::StructDefinition && !scanner.rdata.Valid(field->ElementFields) && !scanner.data.Valid(field->ElementFields))
                return nullptr;
            if (field->UnderlyingType == UnderlyingTypes::StructDefinition)
                return &m_layout.Types.try_emplace((byte const*)fields,
                    field->Name,
                    field->Size,
                    std::vector { std::from_range,
                    std::span { fields, field }
                    | std::views::transform([&](PackFileField const& field) -> Field {
                        return {
                            field.Name,
                            field.UnderlyingType,
                            field.RealType,
                            field.Size,
                            field.UnderlyingType != UnderlyingTypes::Variant ? collect(field.ElementFields) : nullptr,
                            field.UnderlyingType == UnderlyingTypes::Variant ? std::vector { std::from_range, std::span { field.VariantElementFields, field.Size } | std::views::transform([&collect](auto* fields) { return collect(fields); }) } : std::vector<Type const*> { },
                        };
                    })
                }).first->second;
            ++field;
        }
    };

    progress.Start(scanner.rdata.size());
    for (auto p = scanner.rdata.begin(); p < scanner.rdata.end(); p += sizeof(void*))
    {
        if (isalnum(p[0]) && isalnum(p[1]) && isalnum(p[2]) && (!p[3] || isalnum(p[3])))
        {
            uint32 const numVersions = *(uint32 const*)&p[4];
            if (!numVersions || numVersions > 100)
                continue;

            if (auto const versions = *(PackFileVersion**)&p[8]; versions && scanner.rdata.Valid(versions))
            {
                for (uint32 versionNum = 0; versionNum < numVersions; ++versionNum)
                {
                    auto& version = versions[versionNum];
                    if (!version.Fields)
                        continue;
                    if (!scanner.rdata.Valid(version.Fields))
                        goto fail;
                    if (!scanner.text.Valid(version.PostProcessFunction))
                        goto fail;

                    if (auto type = collect(version.Fields))
                        m_layout.Chunks[std::string((char const*)p, p[3] ? 4 : 3)].try_emplace(versionNum, type);
                    else
                        goto fail;
                }
            }

        fail:;
        }

        if (auto const offset = std::distance(scanner.rdata.begin(), p); !(offset % (100 * sizeof(void*))))
            progress = offset;
    }

    if (m_layout.Chunks["AFNT"].empty())
    {
        auto const filename = m_layout.Types.try_emplace(new byte(), Type
        {
            .Name = "<Font File>",
            .DeclaredSize = 6,
            .Fields =
            {
                { .Name = "File", .UnderlyingType = UnderlyingTypes::FileName },
            },
        }).first;
        auto const font = m_layout.Types.try_emplace(new byte(), Type
        {
            .Name = "<Font>",
            .DeclaredSize = 68,
            .Fields =
            {
                { .Name = "Language", .UnderlyingType = UnderlyingTypes::Byte },
                { .Name = "Scale", .UnderlyingType = UnderlyingTypes::Byte },
                { .Name = "Token", .UnderlyingType = UnderlyingTypes::Qword, .RealType = RealTypes::Token },
                { .Name = "Flags", .UnderlyingType = UnderlyingTypes::Dword },
                { .Name = "ExtentX", .UnderlyingType = UnderlyingTypes::Byte },
                { .Name = "ExtentY", .UnderlyingType = UnderlyingTypes::Byte },
                { .Name = "Files", .UnderlyingType = UnderlyingTypes::InlineArray, .ArraySize = 13, .ElementType = &filename->second },
            },
        }).first;
        auto const fonts = m_layout.Types.try_emplace(new byte(), Type
        {
            .Name = "<Fonts>",
            .DeclaredSize = 8,
            .Fields =
            {
                { .Name = "Fonts", .UnderlyingType = UnderlyingTypes::DwordArray, .ElementType = &font->second },
            },
        }).first;
        m_layout.Chunks["AFNT"].try_emplace(0, &fonts->second);
    }

    if (m_layout.Chunks["FOOT"].empty())
    {
        auto const field = m_layout.Types.try_emplace(new byte(), Type
        {
            .Name = "<Field>",
            .DeclaredSize = 24,
        }).first;
        auto const type = m_layout.Types.try_emplace(new byte(), Type
        {
            .Name = "<Type>",
            .DeclaredSize = 24,
            .Fields =
            {
                { .Name = "Fields", .UnderlyingType = UnderlyingTypes::InlineArray, .ArraySize = 9999, .ElementType = &field->second },
            },
        }).first;
        field->second.Fields =
        {
            { .Name = "UnderlyingType", .UnderlyingType = UnderlyingTypes::Word },
            { .Name = "RealType", .UnderlyingType = UnderlyingTypes::Word },
            { .Name = "Name", .UnderlyingType = UnderlyingTypes::String },
            { .Name = "Type", .UnderlyingType = UnderlyingTypes::Ptr, .ElementType = &type->second },
            { .Name = "Size", .UnderlyingType = UnderlyingTypes::Dword },
        };
        m_layout.Chunks["FOOT"].try_emplace(0, &type->second);
    }

    if (m_layout.Chunks["STAR"].empty())
    {
        auto const star = m_layout.Types.try_emplace(new byte(), Type
        {
            .Name = "<Star>",
            .DeclaredSize = 24,
            .Fields =
            {
                { .Name = "Position", .UnderlyingType = UnderlyingTypes::Float2 },
                { .Name = "TexCoordMin", .UnderlyingType = UnderlyingTypes::Float2 },
                { .Name = "TexCoordMax", .UnderlyingType = UnderlyingTypes::Float2 },
            },
        }).first;
        auto const stars = m_layout.Types.try_emplace(new byte(), Type
        {
            .Name = "<Stars>",
            .DeclaredSize = 16,
            .Fields =
            {
                { .Name = "Scale", .UnderlyingType = UnderlyingTypes::Float },
                { .Name = "Stars", .UnderlyingType = UnderlyingTypes::DwordArray, .ElementType = &star->second },
                { .Name = "TextureFile", .UnderlyingType = UnderlyingTypes::FileName },
            },
        }).first;
        m_layout.Chunks["STAR"].try_emplace(0, &stars->second);
    }

    m_loaded = true;
}

void Manager::LoadEmbeddedLayout(LayoutContainer& container, PackFile const& file, PackFileChunk const& chunk)
{
    using namespace Layout;
    auto const footer = chunk.GetFooter();
    if (!footer || !footer->LayoutOffset)
        return;

    auto const p = &chunk.Data[footer->LayoutOffset + 4];
    if (p >= (byte const*)chunk.GetNextChunk() || *(UnderlyingTypes const*)p == UnderlyingTypes::StructDefinition)
        return;

    auto addType = [&container](this auto&& addType, Traversal::FieldIterator fields) -> Type const*
    {
        if (!fields)
            return nullptr;
        std::vector<Field> typeFields;
        for (auto const field : fields)
        {
            auto const underlyingType = (UnderlyingTypes)(uint16)field["UnderlyingType"];
            if (underlyingType == UnderlyingTypes::StructDefinition)
                return typeFields.empty() ? nullptr : &container.Types.try_emplace(field.GetPointer(), field["Name"], field["Size"], std::move(typeFields)).first->second;
            if (underlyingType == UnderlyingTypes::Variant)
                throw new std::exception("Variant field in embedded PackFile layout is not supported");
            typeFields.emplace_back(
                field["Name"],
                underlyingType,
                (RealTypes)(uint16)field["RealType"],
                field["Size"],
                underlyingType != UnderlyingTypes::Variant ? addType(field["Type"]["Fields"]) : nullptr);
        }
        return nullptr;
    };

    if (auto const type = addType(*Traversal::QueryFields(file, p, *m_layout.Chunks["FOOT"].begin()->second, "Fields").begin()))
        container.Chunks[std::string((char const*)&chunk.Header.Magic, strnlen((char const*)&chunk.Header.Magic, 4))].try_emplace(chunk.Header.Version, type);
}

}
