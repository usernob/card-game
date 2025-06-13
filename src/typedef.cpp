#include "typedef.h"
#include <algorithm>
#include <sstream>

std::string utils::toSnakeCase(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    std::replace(result.begin(), result.end(), ' ', '_');
    return result;
}

std::string utils::toTitleCase(const std::string &snake_case)
{
    std::stringstream ss(snake_case);
    std::string word;
    std::string result;

    while (std::getline(ss, word, '_'))
    {
        if (!word.empty())
        {
            word[0] = std::toupper(static_cast<unsigned char>(word[0]));
            for (size_t i = 1; i < word.size(); ++i)
                word[i] = std::tolower(static_cast<unsigned char>(word[i]));

            if (!result.empty()) result += " ";
            result += word;
        }
    }

    return result;
}
