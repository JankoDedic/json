#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

using std::get;
using std::ios_base;
using std::istream;
using std::ostream;

namespace json {

// Types

struct object;
struct array;

struct string : std::string {
    using std::string::string;
};

using number = double;

struct true_ {};
struct false_ {};
struct null {};

using value = std::variant<object, array, string, number, true_, false_, null>;

struct object : std::map<string, value> {};
struct array : std::vector<value> {};

using member = std::pair<string, value>;

// Input

void read_one_character(istream& is, char ch)
{
    char c;
    is >> c;
    assert(c == ch);
}

char escaped_character(istream& is)
{
    char c;
    is >> c;
    switch (c) {
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    default: return c;
    }
}

istream& operator>>(istream& is, string& str)
{
    read_one_character(is, '"');
    is >> std::noskipws;
    for (char c; is >> c; ) {
        if (c == '\\') {
            c = escaped_character(is);
        } else if (c == '"') {
            break;
        }
        str.push_back(c);
    }
    is >> std::skipws;
    return is;
}

istream& operator>>(istream&, value&);

istream& operator>>(istream& is, member& mem)
{
    char c;
    is >> c;
    is.unget();
    if (c == '"') {
        is >> mem.first;
        read_one_character(is, ':');
        is >> mem.second;
        return is;
    } else {
        is.setstate(ios_base::failbit);
        return is;
    }
}

istream& operator>>(istream& is, object& obj)
{
    read_one_character(is, '{');
    for (member m; is >> m; ) {
        obj.insert(std::move(m));
        char c;
        is >> c;
        if (c == ',') {
            continue;
        } else {
            is.unget();
            break;
        }
    }
    is.clear();
    read_one_character(is, '}');
    return is;
}

istream& operator>>(istream& is, array& ar)
{
    read_one_character(is, '[');
    for (value val; is >> val; ) {
        ar.push_back(std::move(val));
        char c;
        is >> c;
        if (c == ',') {
            continue;
        } else {
            is.unget();
            break;
        }
    }
    is.clear();
    read_one_character(is, ']');
    return is;
}

istream& operator>>(istream& is, true_)
{
    std::string s;
    is >> s;
    if (s.back() == ',') {
        is.unget();
        s.pop_back();
    }
    assert(s == "true");
    return is;
}

istream& operator>>(istream& is, false_)
{
    std::string s;
    is >> s;
    if (s.back() == ',') {
        is.unget();
        s.pop_back();
    }
    assert(s == "false");
    return is;
}

istream& operator>>(istream& is, null)
{
    std::string s;
    is >> s;
    if (s.back() == ',') {
        is.unget();
        s.pop_back();
    }
    assert(s == "null");
    return is;
}

istream& operator>>(istream& is, value& val)
{
    char c;
    is >> c;
    is.unget();
    switch (c) {
    case '{': val.emplace<object>(); return is >> get<object>(val);
    case '[': val.emplace<array>(); return is >> get<array>(val);
    case '"': val.emplace<string>(); return is >> get<string>(val);
    case 't': val.emplace<true_>(); return is >> get<true_>(val);
    case 'f': val.emplace<false_>(); return is >> get<false_>(val);
    case 'n': val.emplace<null>(); return is >> get<null>(val);
    default: break;
    }
    if (isdigit(c) || c == '-') {
        val.emplace<number>();
        return is >> std::scientific >> get<number>(val);
    } else {
        is.setstate(ios_base::failbit);
        return is;
    }
}

// Output

template<std::size_t N>
const char* find(const char (&characters)[N], char c)
{
    return std::find(std::begin(characters), std::end(characters), c);
}

ostream& operator<<(ostream& os, const string& str)
{
    constexpr char escapes[] = {
        '"', '\\', '/', '\b', '\f', '\n', '\r', '\t'
    };
    constexpr char esc_chars[] = { '"', '\\', '/', 'b', 'f', 'n', 'r', 't' };
    os << '"';
    for (char c : str) {
        if (auto it = find(escapes, c); it != std::end(escapes)) {
            os << '\\' << *(esc_chars + (it - escapes));
        } else {
            os << c;
        }
    }
    os << '"';
    return os;
}

int indent_index()
{
    static int index = ios_base::xalloc();
    return index;
}

struct indent_t {} indent;
struct increase_indent_t {} increase_indent;
struct decrease_indent_t {} decrease_indent;

ostream& operator<<(ostream& os, indent_t)
{
    return os << std::string(os.iword(indent_index()), ' ');
}

ostream& operator<<(ostream& os, increase_indent_t)
{
    os.iword(indent_index()) += 2;
    return os;
}

ostream& operator<<(ostream& os, decrease_indent_t)
{
    os.iword(indent_index()) -= 2;
    return os;
}

ostream& operator<<(ostream&, const value&);

ostream& operator<<(ostream& os, const object& obj)
{
    os << "{\n" << increase_indent;
    bool first = true;
    for (auto&& [key, value] : obj) {
        if (first) {
            first = false;
        } else {
            os << ",\n";
        }
        os << indent << key << ": " << value;
    }
    os << '\n' << decrease_indent << indent << '}';
    return os;
}

ostream& operator<<(ostream& os, const array& ar)
{
    os << "[\n" << increase_indent;
    bool first = true;
    for (auto&& el : ar) {
        if (first) {
            first = false;
        } else {
            os << ",\n";
        }
        os << indent << el;
    }
    os << '\n' << decrease_indent << indent << ']';
    return os;
}

ostream& operator<<(ostream& os, true_)
{
    return os << "true";
}

ostream& operator<<(ostream& os, false_)
{
    return os << "false";
}

ostream& operator<<(ostream& os, null)
{
    return os << "null";
}

ostream& operator<<(ostream& os, const value& val)
{
    visit([&] (auto&& value) { os << value; }, val);
    return os;
}

} // namespace json

int main()
{
    std::ifstream ifs{"sample.json"};
    json::object object;
    ifs >> object;
    std::cout << object << std::endl;
}
