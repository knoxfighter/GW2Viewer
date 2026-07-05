export module GW2Viewer.Utils.String;
import std;

export namespace GW2Viewer::Utils::String
{

inline void ReplaceAll(std::string& str, std::string_view from, std::string_view to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

inline void ReplaceAll(std::wstring& str, std::wstring_view from, std::wstring_view to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::wstring::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

void SingleLine(std::string& str)
{
    ReplaceAll(str, "\r", R"(<c=#F00>\r</c>)");
    ReplaceAll(str, "\n", R"(<c=#F00>\n</c>)");
}
void SingleLine(std::wstring& str)
{
    ReplaceAll(str, L"\r", LR"(<c=#F00>\r</c>)");
    ReplaceAll(str, L"\n", LR"(<c=#F00>\n</c>)");
}
auto SingleLined(std::string_view str)
{
    std::string result { str };
    SingleLine(result);
    return result;
}
auto SingleLined(std::wstring_view str)
{
    std::wstring result { str };
    SingleLine(result);
    return result;
}

template<typename Ch>
void Uppercase(std::basic_string<Ch>& str)
{
    std::ranges::transform(str, str.begin(), (int(*)(int))std::toupper);
}
auto Uppercased(std::string_view str)
{
    std::string result { str };
    Uppercase(result);
    return result;
}
auto Uppercased(std::wstring_view str)
{
    std::wstring result { str };
    Uppercase(result);
    return result;
}

bool EqualsCS(std:: string_view haystack, std:: string_view needle) { return haystack == needle; }
bool EqualsCS(std::wstring_view haystack, std::wstring_view needle) { return haystack == needle; }
bool EqualsCI(std:: string_view haystack, std:: string_view needle) { return haystack.size() == needle.size() && std::ranges::equal(haystack, needle, { }, (int(*)(int))std::toupper, (int(*)(int))std::toupper); }
bool EqualsCI(std::wstring_view haystack, std::wstring_view needle) { return haystack.size() == needle.size() && std::ranges::equal(haystack, needle, { }, (int(*)(int))std::toupper, (int(*)(int))std::toupper); }
bool ContainsCS(std:: string_view haystack, std:: string_view needle) { return haystack.contains(needle); }
bool ContainsCS(std::wstring_view haystack, std::wstring_view needle) { return haystack.contains(needle); }
bool ContainsCI(std:: string_view haystack, std:: string_view needle) { return haystack.size() >= needle.size() && !std::ranges::search(haystack, needle, { }, (int(*)(int))std::toupper, (int(*)(int))std::toupper).empty(); }
bool ContainsCI(std::wstring_view haystack, std::wstring_view needle) { return haystack.size() >= needle.size() && !std::ranges::search(haystack, needle, { }, (int(*)(int))std::toupper, (int(*)(int))std::toupper).empty(); }
bool StartsWithCS(std:: string_view haystack, std:: string_view needle) { return haystack.starts_with(needle); }
bool StartsWithCS(std::wstring_view haystack, std::wstring_view needle) { return haystack.starts_with(needle); }
bool StartsWithCI(std:: string_view haystack, std:: string_view needle) { return haystack.size() >= needle.size() && std::ranges::starts_with(haystack, needle, { }, (int(*)(int))std::toupper, (int(*)(int))std::toupper); }
bool StartsWithCI(std::wstring_view haystack, std::wstring_view needle) { return haystack.size() >= needle.size() && std::ranges::starts_with(haystack, needle, { }, (int(*)(int))std::toupper, (int(*)(int))std::toupper); }
bool EndsWithCS(std:: string_view haystack, std:: string_view needle) { return haystack.ends_with(needle); }
bool EndsWithCS(std::wstring_view haystack, std::wstring_view needle) { return haystack.ends_with(needle); }
bool EndsWithCI(std:: string_view haystack, std:: string_view needle) { return haystack.size() >= needle.size() && std::ranges::ends_with(haystack, needle, { }, (int(*)(int))std::toupper, (int(*)(int))std::toupper); }
bool EndsWithCI(std::wstring_view haystack, std::wstring_view needle) { return haystack.size() >= needle.size() && std::ranges::ends_with(haystack, needle, { }, (int(*)(int))std::toupper, (int(*)(int))std::toupper); }

inline std::vector<std::string_view> Split(std::string_view s, std::string_view delimiter)
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string_view> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
    {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr(pos_start));
    return res;
}

}
