#include "textrenderer.h"
#include "typedef.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_image/SDL_image.h>
#include <fstream>
#include <sstream>
#include <string>

std::unordered_map<std::string, Font> FontsManager::fonts;

Font::Font(SDL_Renderer *renderer, const std::string &fntPath)
{
    initialize(renderer, fntPath);
}

Font::~Font()
{
    if (texture)
    {
        SDL_DestroyTexture(texture);
    }
}

bool Font::initialize(SDL_Renderer *renderer, const std::string &fntPath)
{
    std::ifstream file(fntPath);
    if (!file.is_open())
    {
        SDL_SetError("cant open %s", fntPath.c_str());
        return false;
    }

    std::string line;
    std::string textureFile;

    while (std::getline(file, line))
    {
        std::istringstream ss(line);
        std::string tag;
        ss >> tag;

        if (tag == "common")
        {
            std::string key;
            while (ss >> key)
            {
                if (key.rfind("lineHeight=", 0) == 0) line_height = std::stof(key.substr(11));
            }
        }
        else if (tag == "page")
        {
            std::string id, fileField;
            ss >> id >> fileField;
            size_t start = fileField.find("\"") + 1;
            size_t end = fileField.find_last_of("\"");
            textureFile = fileField.substr(start, end - start);
        }
        else if (tag == "char")
        {
            int id = -1;
            GlyphInfo g{};
            std::string kv;

            while (ss >> kv)
            {
                auto pos = kv.find('=');
                std::string key = kv.substr(0, pos);
                std::string val = kv.substr(pos + 1);
                if (key == "id") id = std::stoi(val);
                else if (key == "x") g.rect.x = std::stof(val);
                else if (key == "y") g.rect.y = std::stof(val);
                else if (key == "width") g.rect.w = std::stof(val);
                else if (key == "height") g.rect.h = std::stof(val);
                else if (key == "xoffset") g.xoffset = std::stof(val);
                else if (key == "yoffset") g.yoffset = std::stof(val);
                else if (key == "xadvance") g.advance = std::stof(val);
            }
            if (id >= 0) glyphs[id] = g;
        }
    }

    // Load texture (assume in same folder)
    std::string texturePath = fntPath.substr(0, fntPath.find_last_of("/\\") + 1) + textureFile;
    texture = IMG_LoadTexture(renderer, texturePath.c_str());
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART);
    return true;
}

Font *FontsManager::getFont(const std::string &id)
{
    auto it = fonts.find(id);
    return it != fonts.end() ? &it->second : nullptr;
}

bool FontsManager::addFont(
    SDL_Renderer *renderer,
    const std::string &id,
    const std::string &fntPath
)
{
    fonts.insert({id, Font()});
    return fonts[id].initialize(renderer, fntPath);
}

void FontsManager::clear()
{
    fonts.clear();
}

Text::Text(Font *font, const char *text, float max_width, float scale, SDL_Color color)
    : m_font(font), m_max_width(max_width), m_scale(scale), m_color(color)
{
    setText(text); // must callculate the bounding rect
};

Text::~Text()
{
    if (m_cached_texture)
    {
        SDL_DestroyTexture(m_cached_texture);
    }
}

SDL_FRect Text::getRect()
{
    return m_text_rect;
}

void Text::recalculateBoundingRect()
{
    int x = 4, y = 0;
    int maxlineWidth = 0;
    int lineHeight = m_font->line_height;
    std::stringstream ss(m_text);
    std::string word;
    auto it = m_font->glyphs.find(' ');
    GlyphInfo &space = it->second;
    while (ss >> word)
    {
        int wordWidth = 0;
        for (char c : word)
        {
            auto it = m_font->glyphs.find(c);
            if (it == m_font->glyphs.end()) continue;
            GlyphInfo &glyph = it->second;
            wordWidth += glyph.advance + glyph.xoffset + 2;
        }

        // plus space after word
        wordWidth += space.advance + space.xoffset;

        if (m_max_width > 0 && x + wordWidth > m_max_width)
        {
            // wrap
            y += lineHeight;
            x = 4;
        }

        x += wordWidth;
        maxlineWidth = std::max(maxlineWidth, x);
    }
    // remove last space
    maxlineWidth -= space.advance + space.xoffset;
    m_text_rect.w = maxlineWidth + 4;
    m_text_rect.h = y + lineHeight;
    m_text_rect.w *= m_scale;
    m_text_rect.h *= m_scale;
    m_dirty = true;
}

void Text::setScale(float scale)
{
    m_scale = scale;
    m_dirty = true;
}

void Text::setText(const char *text)
{
    m_text = text;
    recalculateBoundingRect();
}

void Text::setColor(float r, float g, float b)
{
    m_color.r = r;
    m_color.g = g;
    m_color.b = b;
    m_dirty = true;
}

void Text::setPosition(float x, float y)
{
    m_x = x;
    m_y = y;
}

void Text::renderText(SDL_Renderer *renderer, bool centered)
{
    SDL_FRect dst_rect = {
        m_x,
        m_y,
        static_cast<float>(m_text_rect.w) * m_scale,
        static_cast<float>(m_text_rect.h) * m_scale
    };
    if (centered)
    {
        dst_rect.x -= m_text_rect.w * 0.5;
        dst_rect.y -= m_text_rect.h * 0.5;
    }
    if (m_dirty)
    {
        if (m_cached_texture)
        {
            SDL_DestroyTexture(m_cached_texture);
            m_cached_texture = nullptr;
        }
        m_cached_texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            static_cast<int>(m_text_rect.w),
            static_cast<int>(m_text_rect.h)
        );
        SDL_SetTextureScaleMode(m_cached_texture, SDL_SCALEMODE_PIXELART);
        SDL_SetRenderTarget(renderer, m_cached_texture);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0); // Clear
        SDL_RenderClear(renderer);

        float cursorX = 4;
        float cursorY = 0;
        std::istringstream iss(m_text);
        std::string word;

        auto it = m_font->glyphs.find(' ');
        GlyphInfo &space = it->second;

        int lineHeight = m_font->line_height;
        while (iss >> word)
        {
            int wordWidth = 0;
            for (char c : word)
            {
                auto it = m_font->glyphs.find(c);
                if (it == m_font->glyphs.end()) continue;
                GlyphInfo &info = it->second;
                wordWidth += info.advance + info.xoffset + 2;
            }

            wordWidth += space.advance + space.xoffset;

            if (m_max_width > 0 && cursorX + wordWidth > m_max_width)
            {
                cursorX = 4;
                cursorY += lineHeight;
            }

            for (char c : word)
            {
                auto it = m_font->glyphs.find(c);
                if (it == m_font->glyphs.end()) continue;
                GlyphInfo &info = it->second;

                SDL_FRect gyph_src_rect, glyph_dst_rect;
                SDL_RectToFRect(&info.rect, &gyph_src_rect);
                glyph_dst_rect.x = cursorX;
                glyph_dst_rect.y = cursorY + static_cast<float>(info.yoffset);
                glyph_dst_rect.w = static_cast<float>(info.rect.w);
                glyph_dst_rect.h = static_cast<float>(info.rect.h);

                SDL_SetTextureColorModFloat(m_cached_texture, m_color.r, m_color.g, m_color.b);
                SDL_RenderTexture(renderer, m_font->texture, &gyph_src_rect, &glyph_dst_rect);
#if DEBUG_LAYOUT
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderRect(renderer, &glyph_dst_rect);
#endif
                cursorX += info.advance + info.xoffset + 2;
            }
            // add space
            cursorX += space.advance + space.xoffset;
        }
        SDL_SetRenderTarget(renderer, nullptr);
        SDL_RenderTexture(renderer, m_cached_texture, nullptr, &dst_rect);
        m_dirty = false;
    }
    else
    {
        SDL_RenderTexture(renderer, m_cached_texture, nullptr, &dst_rect);
    }
#if DEBUG_LAYOUT
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderRect(renderer, &dst_rect);
#endif
}
