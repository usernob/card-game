#ifndef SRC_WIDGET_H
#define SRC_WIDGET_H

#include "card.h"
#include "textrenderer.h"
#include "texturemanager.h"
#include "typedef.h"
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <functional>

// forward declaration
class WidgetLayout;

// Base class of widget
// basically a rectangle with texture or color
class Widget
{
protected:
    WidgetLayout *m_parent = nullptr;
    SDL_FRect m_bounding_rect;
    SDL_FRect m_rect;
    SDL_Texture *m_texture = nullptr;
    SDL_FRect m_tex_rect;
    SDL_Color m_color = {0, 0, 0, 0};
    Float4 m_padding = {0, 0, 0, 0};
    float m_max_width = 0;
    float m_max_height = 0;
    bool m_visible = true;
    bool m_active = true;

public:
    Widget(WidgetLayout *parent = nullptr) : m_parent(parent) {}

    virtual ~Widget() = default;

    void setBackgroundTexture(SDL_Texture *texture, SDL_FRect rect);
    void setBackgroundColor(SDL_Color color);
    void setPadding(Float4 padding);
    void setVisible(bool visible);
    void setActive(bool active);
    bool isVisible();
    bool isActive();
    virtual SDL_FRect getRect();
    virtual void setRect(SDL_FRect rect);
    virtual void setBoundingRect(SDL_FRect rect);

    virtual bool checkClick(SDL_FPoint mouse)
    {
        return false;
    }

    virtual void draw(SDL_Renderer *renderer);

    virtual void update(float dt)
    {
        return;
    }
};

// indicate that this class can be clicked
class WidgetClickable : public Widget
{
protected:
    SDL_Color temp_color;
    SDL_Texture *temp_texture;
    SDL_FRect temp_rect;
    bool m_change_color = false;
    bool m_change_texture = false;
    SDL_Color m_click_color = {255, 255, 255, 255};
    SDL_Texture *m_click_texture = nullptr;
    SDL_FRect m_click_tex_rect;
    std::function<void(SDL_FPoint)> m_callback;
    unsigned int m_last_click = 0;
    bool m_clicked = false;
    bool m_was_clicked = false;
    int m_delay_click = 150;
    SDL_FPoint m_mouse = {0, 0};

public:
    void setClickColor(SDL_Color color);
    void setClickTexture(SDL_Texture *texture, SDL_FRect rect);
    bool checkClick(SDL_FPoint mouse) override;
    void doClick();
    virtual void clickEnter();
    virtual void clickLeave();
    void update(float dt) override;

    virtual void onClick(std::function<void(SDL_FPoint)> callback);
};

// ================================  UI Implementations  ================================

class Button : public WidgetClickable
{
protected:
    Text m_text_renderer;

public:
    Button(WidgetLayout *parent, Text &&text_renderer, Float4 padding = {8, 30, 8, 30});

    Button(const Button &) = delete;
    Button(Button &&) = delete;
    Button &operator=(const Button &) = delete;
    Button &operator=(Button &&) = delete;

    ~Button() = default;

    void setRect(SDL_FRect rect) override;
    void draw(SDL_Renderer *renderer) override;
    void clickEnter() override;
    void clickLeave() override;
};

class MainButton : public Button
{
public:
    MainButton(WidgetLayout *parent, Text &&text_renderer, Float4 padding = {8, 30, 8, 30});

    void clickEnter() override;
};

class PrimaryButton : public Button
{
public:
    PrimaryButton(
        WidgetLayout *parent,
        Text &&text_renderer,
        const std::string &texture_name,
        Float4 padding = {8, 30, 8, 30}
    );
};

class Label : public Widget
{
private:
    Text m_text_renderer;

public:
    Label(WidgetLayout *parent, Text &&text_renderer, Float4 padding = {0, 0, 0, 0});

    Label(const Label &) = delete;
    Label(Label &&) = delete;
    Label &operator=(const Label &) = delete;
    Label &operator=(Label &&) = delete;

    ~Label() = default;

    void setRect(SDL_FRect rect) override;
    void setText(const char *text);
    void draw(SDL_Renderer *renderer) override;
};

class CardWidget : public WidgetClickable
{
private:
    Card *m_card;
    bool m_was_selected = false;
    bool m_render_score = false;
    Text m_text_renderer;

public:
    CardWidget(WidgetLayout *parent, Card *card);

    void clickEnter() override {}

    void clickLeave() override;

    // helper for clean set position without recalculate bounding rect
    void resetPosition();

    void setCard(Card *card);

    void setRenderScore(bool render_score);

    void draw(SDL_Renderer *renderer) override;
};

#endif // SRC_WIDGET_H
