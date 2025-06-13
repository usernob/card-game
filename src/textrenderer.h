#ifndef SRC_TEXTRENDERER_H
#define SRC_TEXTRENDERER_H

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <string>
#include <unordered_map>

struct GlyphInfo
{
    SDL_Rect rect;
    int xoffset, yoffset;
    int advance;
};

// font should be global so that it can be used by all widgets
// and not have to be initialized every time
class Font
{
public:
    int line_height;
    SDL_Texture *texture;
    std::unordered_map<char, GlyphInfo> glyphs;

    Font(SDL_Renderer *renderer, const std::string &fntPath);
    Font() = default;
    Font(const Font &) = default;
    Font(Font &&) = delete;
    Font &operator=(const Font &) = default;
    Font &operator=(Font &&) = delete;
    ~Font();

    // load the font bitmap information
    bool initialize(SDL_Renderer *renderer, const std::string &fntPath);
};

class FontsManager
{
public:
    static std::unordered_map<std::string, Font> fonts;

    static Font *getFont(const std::string &id);

    static bool addFont(SDL_Renderer *renderer, const std::string &id, const std::string &fntPath);

    static void clear();
};

class Text
{
private:
    Font *m_font;
    std::string m_text;
    SDL_FRect m_text_rect;
    float m_scale;
    SDL_Color m_color;
    SDL_Texture *m_cached_texture = nullptr;
    bool m_dirty = true;
    float m_max_width = 0;
    float m_x = 0;
    float m_y = 0;

public:
    Text() = default;

    Text(
        Font *font,
        const char *text,
        float max_width = 0,
        float scale = 1.0f,
        SDL_Color color = {255, 255, 255, 255}
    );

    ~Text();

    SDL_FRect getRect();

    void recalculateBoundingRect();

    void setScale(float scale);

    void setText(const char *text);

    void setColor(float r, float g, float b);

    void setPosition(float x, float y);

    void renderText(SDL_Renderer *renderer, bool centered = false);
};

#endif // SRC_TEXTRENDERER_H
