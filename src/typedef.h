#ifndef SRC_TYPEDEF_H
#define SRC_TYPEDEF_H

#include <string>
#ifndef DEUG_LAYOUT

#define DEBUG_LAYOUT 0

#endif // !DEUG_LAYOUT

#define WINDOW_WIDTH 1600
#define WINDOW_HEIGHT 900

struct Float4
{
    float u, l, d, r;
};

enum class Anchor
{
    START,
    CENTER,
    END
};

enum class LayoutType
{
    HORIZONTAL,
    VERTICAL
};

struct LayoutProp
{
    LayoutType layout_type = LayoutType::HORIZONTAL;
    float width = 0;
    float height = 0;
    Anchor horizontal_anchor = Anchor::START;
    Anchor vertical_anchor = Anchor::START;
    float gap = 0;
    Float4 padding = {0, 0, 0, 0};
    bool fit_content = false;
};

namespace utils
{
std::string toSnakeCase(const std::string &str);
std::string toTitleCase(const std::string &snake_case);
}
#endif // SRC_TYPEDEF_H
