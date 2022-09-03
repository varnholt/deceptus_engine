#include "json.hpp"

#include <fstream>
#include <iostream>

using namespace std;
using json = nlohmann::json;

int main()
{
    std::ifstream ifs("animations.json");
    json jf = json::parse(ifs);

    std::cout << jf.dump(3) << std::endl;

    return 0;
}

// if (pretty_print)
// {
//     o->write_character('[');
//
//     // variable to hold indentation for recursive calls
//     const auto new_indent = current_indent + indent_step;
//     if (JSON_HEDLEY_UNLIKELY(indent_string.size() < new_indent))
//     {
//         indent_string.resize(indent_string.size() * 2, ' ');
//     }
//
//     // first n-1 elements
//     for (auto i = val.m_value.array->cbegin();
//          i != val.m_value.array->cend() - 1; ++i)
//     {
//         dump(*i, true, ensure_ascii, indent_step, new_indent);
//         o->write_character(',');
//         o->write_character(' ');
//     }
//
//     // last element
//     JSON_ASSERT(!val.m_value.array->empty());
//     dump(val.m_value.array->back(), true, ensure_ascii, indent_step, new_indent);
//
//     o->write_character(']');
// }
