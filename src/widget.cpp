#include "widget.h"
#include "SDL3/SDL_timer.h"
#include "layout.h"
#include "textrenderer.h"
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>

void Widget::draw(SDL_Renderer *renderer)
{
    if (m_texture && m_texture != nullptr)
    {
        SDL_RenderTexture9Grid(renderer, m_texture, &m_tex_rect, 6, 6, 6, 6, 2.f, &m_rect);
    }
    else
    {
        if (m_color.a)
        {
            SDL_SetRenderDrawColor(renderer, m_color.r, m_color.g, m_color.b, m_color.a);
            SDL_RenderFillRect(renderer, &m_rect);
        }
    }
#if DEBUG_LAYOUT
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderRect(renderer, &m_rect);
#endif // DEBUG_LAYOUT
}

void Widget::setBackgroundTexture(SDL_Texture *texture, SDL_FRect rect)
{
    m_texture = texture;
    m_tex_rect = rect;
}

void Widget::setBackgroundColor(SDL_Color color)
{
    m_color = color;
}

void Widget::setPadding(Float4 padding)
{
    m_padding = padding;
    m_rect.x = m_bounding_rect.x - m_padding.l;
    m_rect.y = m_bounding_rect.y - m_padding.u;
    m_rect.w = m_bounding_rect.w + m_padding.l + m_padding.r;
    m_rect.h = m_bounding_rect.h + m_padding.u + m_padding.d;
}

void Widget::setVisible(bool visible)
{
    m_visible = visible;
    m_parent->recalculateBoundingRect();
}

void Widget::setActive(bool active)
{
    m_active = active;
}

bool Widget::isVisible()
{
    return m_visible;
}

bool Widget::isActive()
{
    return m_active;
}

SDL_FRect Widget::getRect()
{
    return m_rect;
}

void Widget::setRect(SDL_FRect rect)
{
    m_rect = rect;
}

void Widget::setBoundingRect(SDL_FRect rect)
{
    m_bounding_rect = rect;
    m_rect.x = m_bounding_rect.x - m_padding.l;
    m_rect.y = m_bounding_rect.y - m_padding.u;
    m_rect.w = m_bounding_rect.w + m_padding.l + m_padding.r;
    m_rect.h = m_bounding_rect.h + m_padding.u + m_padding.d;
}

// ================================  WidgetClickable  ================================

void WidgetClickable::setClickColor(SDL_Color color)
{
    m_click_color = color;
}

void WidgetClickable::setClickTexture(SDL_Texture *texture, SDL_FRect rect)
{
    m_click_texture = texture;
    m_click_tex_rect = rect;
}

bool WidgetClickable::checkClick(SDL_FPoint mouse)
{
    if (SDL_PointInRectFloat(&mouse, &m_rect))
    {
        m_last_click = SDL_GetTicks();
        m_clicked = !m_clicked;
        m_mouse = mouse;
        return true;
    }
    return false;
}

void WidgetClickable::doClick()
{
    if (m_callback)
    {
        m_callback(m_mouse);
    }
}

void WidgetClickable::clickEnter()
{
    if (m_color.a > 0)
    {
        temp_color = m_color;
        m_color = m_click_color;
        m_change_color = true;
    }

    if (m_click_texture)
    {
        temp_texture = m_texture;
        temp_rect = m_tex_rect;
        m_texture = m_click_texture;
        m_tex_rect = m_click_tex_rect;
        m_change_texture = true;
    }
}

void WidgetClickable::clickLeave()
{
    if (m_change_color)
    {
        m_color = temp_color;
        m_change_color = false;
    }
    if (m_change_texture)
    {
        m_texture = temp_texture;
        m_tex_rect = temp_rect;
        m_change_texture = false;
    }
}

void WidgetClickable::update(float dt)
{
    if (m_clicked && SDL_GetTicks() - m_last_click > m_delay_click)
    {
        m_last_click = 0;
        doClick();
        m_clicked = false;
    }
    if (m_clicked && !m_was_clicked)
    {
        clickEnter();
    }
    else if (!m_clicked && m_was_clicked)
    {
        clickLeave();
    }
    m_was_clicked = m_clicked;
}

void WidgetClickable::onClick(std::function<void(SDL_FPoint)> callback)
{
    m_callback = callback;
}

// ================================  Button  ================================

Button::Button(WidgetLayout *parent, Text &&text_renderer, Float4 padding)
    : m_text_renderer(std::move(text_renderer))
{
    m_parent = parent;
    m_padding = padding;
    setBoundingRect(m_text_renderer.getRect());
}

void Button::clickEnter()
{
    WidgetClickable::clickEnter();
    m_text_renderer.setPosition(m_rect.x + m_rect.w * 0.5, m_rect.y + 1 + m_rect.h * 0.5);
}

void Button::clickLeave()
{
    WidgetClickable::clickLeave();
    m_text_renderer.setPosition(m_rect.x + m_rect.w * 0.5, m_rect.y + m_rect.h * 0.5);
}

void Button::setRect(SDL_FRect rect)
{
    Widget::setRect(rect);
    m_text_renderer.setPosition(m_rect.x + m_rect.w * 0.5, m_rect.y + m_rect.h * 0.5);
}

void Button::draw(SDL_Renderer *renderer)
{
    Widget::draw(renderer);
    m_text_renderer.renderText(renderer, true);
}

// ================================  Label  ================================
Label::Label(WidgetLayout *parent, Text &&text_renderer, Float4 padding)
    : m_text_renderer(std::move(text_renderer))
{
    m_parent = parent;
    m_padding = padding;
    setBoundingRect(m_text_renderer.getRect());
}

void Label::setText(const char *text)
{
    m_text_renderer.setText(text);
    setBoundingRect(m_text_renderer.getRect());
    m_parent->recalculateBoundingRect();
}

void Label::setRect(SDL_FRect rect)
{
    Widget::setRect(rect);
    m_text_renderer.setPosition(m_rect.x + m_rect.w * 0.5, m_rect.y + m_rect.h * 0.5);
}

void Label::draw(SDL_Renderer *renderer)
{
    Widget::draw(renderer);
    m_text_renderer.renderText(renderer, true);
}

MainButton::MainButton(WidgetLayout *parent, Text &&text_renderer, Float4 padding)
    : Button(parent, std::move(text_renderer), padding)
{
    auto atlas = TextureManager::instance()->getAtlas("ui-atlas");
    setBackgroundTexture(atlas->getAtlas(), atlas->getTextureInfo("button-big").rect);
    setClickTexture(atlas->getAtlas(), atlas->getTextureInfo("button-big-clicked").rect);
}

void MainButton::clickEnter()
{
    WidgetClickable::clickEnter();
    m_text_renderer.setPosition(m_rect.x + m_rect.w * 0.5, m_rect.y + 4 + m_rect.h * 0.5);
}

PrimaryButton::PrimaryButton(
    WidgetLayout *parent,
    Text &&text_renderer,
    const std::string &texture_name,
    Float4 padding
)
    : Button(parent, std::move(text_renderer), padding)
{
    auto atlas = TextureManager::instance()->getAtlas("ui-atlas");
    setBackgroundTexture(atlas->getAtlas(), atlas->getTextureInfo(texture_name).rect);
    setClickTexture(atlas->getAtlas(), atlas->getTextureInfo(texture_name + "-clicked").rect);
}

CardWidget::CardWidget(WidgetLayout *parent, Card *card)
    : m_card(card), m_text_renderer(FontsManager::getFont("font1-w"), "")
{
    m_parent = parent;
    m_delay_click = 100;

    float card_width = 57 * 2;
    float card_height = 79 * 2;
    setRect({0, 0, card_width, card_height});
    setBoundingRect({0, 0, card_width, card_height});
};

void CardWidget::clickLeave()
{
    if (!m_card)
    {
        return;
    }
    if (m_card->selected && !m_was_selected)
    {
        m_rect.y -= 20;
    }
    else if (!m_card->selected && m_was_selected)
    {
        m_rect.y += 20;
    }

    m_was_selected = m_card->selected;
}

void CardWidget::resetPosition()
{
    if (m_was_selected)
    {
        m_rect.y += 20;
        m_was_selected = false;
    }
}

void CardWidget::setCard(Card *card)
{
    m_card = card;
    if (m_card != nullptr)
    {
        m_text_renderer.setText(
            std::string("+" + std::to_string(static_cast<int>(card->rank))).c_str()
        );
    }
}

void CardWidget::setRenderScore(bool render_score)
{
    m_render_score = render_score;
}

void CardWidget::draw(SDL_Renderer *renderer)
{
    if (m_card != nullptr)
    {
        SDL_RenderTexture(renderer, m_card->atlas, &m_card->tex_rect, &m_rect);
    }
    else
    {
        SDL_SetRenderDrawColor(renderer, 55, 55, 55, 160);
        SDL_RenderFillRect(renderer, &m_rect);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &m_rect);
    }

    if (m_render_score)
    {
        m_text_renderer.setPosition(m_rect.x + m_rect.w * 0.5, m_rect.y - 20);
        m_text_renderer.renderText(renderer, true);
    }
};
