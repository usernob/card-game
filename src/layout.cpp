#include "layout.h"
#include "typedef.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <algorithm>

SDL_FRect BaseLayout::getRect()
{
    return m_rect;
}

SDL_FRect BaseLayout::getBoundingRect()
{
    return m_bounding_rect;
}

SDL_FRect BaseLayout::getMaxmimumBoundingRect()
{
    return {
        m_rect.x + m_prop.padding.l,
        m_rect.y + m_prop.padding.u,
        m_rect.w - m_prop.padding.l - m_prop.padding.r,
        m_rect.h - m_prop.padding.u - m_prop.padding.d
    };
}

void BaseLayout::setRect(SDL_FRect rect)
{
    m_rect = rect;
}

void BaseLayout::recalculateRect()
{
    setRect(m_rect);
}

float BaseLayout::getInitWidth()
{
    return m_prop.width;
}

float BaseLayout::getInitHeight()
{
    return m_prop.height;
}

void BaseLayout::setBackgroundColor(SDL_Color color)
{
    m_background_color = color;
}

void BaseLayout::setBackgroundTexture(SDL_Texture *texture, SDL_FRect rect)
{
    m_background_texture = texture;
    m_background_texture_rect = rect;
}

void BaseLayout::renderBackground(SDL_Renderer *renderer)
{
    if (m_background_color.a == 0) return;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(
        renderer,
        m_background_color.r,
        m_background_color.g,
        m_background_color.b,
        m_background_color.a
    );
    SDL_RenderFillRect(renderer, &m_rect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void BaseLayout::renderTexture(SDL_Renderer *renderer)
{
    if (m_background_texture == nullptr) return;
    SDL_RenderTexture9Grid(
        renderer,
        m_background_texture,
        &m_background_texture_rect,
        6,
        6,
        6,
        6,
        2.f,
        &m_rect
    );
}

void BaseLayout::updateCenterPoint()
{
    m_center_point = {
        static_cast<float>(m_rect.x + m_rect.w * 0.5),
        static_cast<float>(m_rect.y + m_rect.h * 0.5)
    };
}

void BaseLayout::updatePosHorizontalBoundingRect()
{
    if (m_prop.horizontal_anchor == Anchor::START)
    {
        m_bounding_rect.x = m_rect.x + m_prop.padding.l;
    }
    else if (m_prop.horizontal_anchor == Anchor::CENTER)
    {
        m_bounding_rect.x = m_center_point.x - m_bounding_rect.w * 0.5;
    }
    else if (m_prop.horizontal_anchor == Anchor::END)
    {
        m_bounding_rect.x = m_rect.x + m_rect.w - m_bounding_rect.w - m_prop.padding.r;
    }
}

void BaseLayout::updatePosVerticalBoundingRect()
{
    if (m_prop.vertical_anchor == Anchor::START)
    {
        m_bounding_rect.y = m_rect.y + m_prop.padding.u;
    }
    else if (m_prop.vertical_anchor == Anchor::CENTER)
    {
        m_bounding_rect.y = m_center_point.y - m_bounding_rect.h * 0.5;
    }
    else if (m_prop.vertical_anchor == Anchor::END)
    {
        m_bounding_rect.y = m_rect.y + m_rect.h - m_bounding_rect.h - m_prop.padding.d;
    }
}

void BaseLayout::registerMouseEvents(const SDL_Event *event)
{
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        mouseClickEvent(&event->button);
    }
};

WidgetLayout::WidgetLayout(LayoutProp prop)
{
    m_rect = {0, 0, prop.width, prop.height};
    m_prop = prop;
    m_widget_childs.reserve(4);
    updateCenterPoint();
    updatePosHorizontalBoundingRect();
    updatePosVerticalBoundingRect();
}

void WidgetLayout::setRect(SDL_FRect rect)
{
    m_rect = rect;

    if (m_prop.fit_content)
    {
        SDL_FRect max_rect;
        max_rect.w = m_bounding_rect.w + m_prop.padding.l + m_prop.padding.r;
        max_rect.h = m_bounding_rect.h + m_prop.padding.u + m_prop.padding.d;
        m_prop.height = max_rect.h;
        m_prop.width = max_rect.w;
        m_rect.w = max_rect.h;
        m_rect.h = max_rect.w;
    }
    updateCenterPoint();
    updatePosHorizontalBoundingRect();
    updatePosVerticalBoundingRect();
    updateChildPos();
}

void WidgetLayout::recalculateBoundingRect()
{
    m_bounding_rect = {0, 0, 0, 0};
    float wide = 0;
    for (auto &child_widget : m_widget_childs)
    {
        if (!child_widget->isVisible())
        {
            continue;
        }
        SDL_FRect child_widget_rect = child_widget->getRect();

        if (m_prop.layout_type == LayoutType::HORIZONTAL)
        {
            m_bounding_rect.h = std::max(m_bounding_rect.h, child_widget_rect.h);
            wide += child_widget_rect.w;
            m_bounding_rect.w = wide;
            wide += m_prop.gap;
            // we must calculate gap manually because there is childs that are not visible
        }
        else if (m_prop.layout_type == LayoutType::VERTICAL)
        {
            m_bounding_rect.w = std::max(m_bounding_rect.w, child_widget_rect.w);
            wide += child_widget_rect.h;
            m_bounding_rect.h = wide;
            wide += m_prop.gap;
        }
    }
    recalculateRect();
}

void WidgetLayout::updateChildPos()
{
    float offset = 0;
    for (auto &child_widget : m_widget_childs)
    {
        if (!child_widget->isVisible())
        {
            return;
        }
        SDL_FRect child_widget_rect = child_widget->getRect();
        if (m_prop.layout_type == LayoutType::HORIZONTAL)
        {
            child_widget_rect.x = m_bounding_rect.x + offset;
            if (m_prop.vertical_anchor == Anchor::START)
            {
                child_widget_rect.y = m_rect.y + m_prop.padding.u;
            }
            else if (m_prop.vertical_anchor == Anchor::CENTER)
            {
                child_widget_rect.y = m_center_point.y - child_widget_rect.h * 0.5;
            }
            else if (m_prop.vertical_anchor == Anchor::END)
            {
                child_widget_rect.y = m_rect.y + m_rect.h - child_widget_rect.h - m_prop.padding.d;
            }
            offset += child_widget_rect.w + m_prop.gap;
        }
        else if (m_prop.layout_type == LayoutType::VERTICAL)
        {
            child_widget_rect.y = m_bounding_rect.y + offset;
            if (m_prop.horizontal_anchor == Anchor::START)
            {
                child_widget_rect.x = m_rect.x + m_prop.padding.l;
            }
            else if (m_prop.horizontal_anchor == Anchor::CENTER)
            {
                child_widget_rect.x = m_center_point.x - child_widget_rect.w * 0.5;
            }
            else if (m_prop.horizontal_anchor == Anchor::END)
            {
                child_widget_rect.x = m_rect.x + m_rect.w - child_widget_rect.w - m_prop.padding.r;
            }
            offset += child_widget_rect.h + m_prop.gap;
        }
        child_widget->setRect(child_widget_rect);
    }
}

void WidgetLayout::addWidgetHorizontal(Widget *widget)
{
    SDL_FRect widget_rect = widget->getRect();
    m_bounding_rect.w += widget_rect.w;

    if (m_widget_childs.size() > 0)
    {
        m_bounding_rect.w += m_prop.gap;
    }

    m_bounding_rect.h = std::max(m_bounding_rect.h, widget_rect.h);

    updatePosVerticalBoundingRect();
    updatePosHorizontalBoundingRect();

    widget->setRect(widget_rect);
}

void WidgetLayout::addWidgetVertical(Widget *widget)
{
    SDL_FRect widget_rect = widget->getRect();
    m_bounding_rect.h += widget_rect.h;

    if (m_widget_childs.size() > 0)
    {
        m_bounding_rect.h += m_prop.gap;
    }

    m_bounding_rect.w = std::max(m_bounding_rect.w, widget_rect.w);

    updatePosHorizontalBoundingRect();
    updatePosVerticalBoundingRect();

    widget->setRect(widget_rect);
}

void WidgetLayout::addWidget(Widget *widget)
{

    if (m_prop.layout_type == LayoutType::HORIZONTAL)
    {
        addWidgetHorizontal(widget);
    }
    else if (m_prop.layout_type == LayoutType::VERTICAL)
    {
        addWidgetVertical(widget);
    }
    m_widget_childs.push_back(widget);
    updateChildPos();
}

void WidgetLayout::render(SDL_Renderer *renderer)
{
    renderTexture(renderer);
    renderBackground(renderer);
    for (auto &widget : m_widget_childs)
    {
        if (!widget->isVisible())
        {
            continue;
        }
        widget->draw(renderer);
    }

#if DEBUG_LAYOUT
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderPoint(renderer, m_center_point.x, m_center_point.y);
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderRect(renderer, &m_bounding_rect);
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderRect(renderer, &m_rect);
#endif // DEBUG_LAYOUT
}

void WidgetLayout::update(float dt)
{
    for (auto &widget : m_widget_childs)
    {
        if (!widget->isVisible())
        {
            continue;
        }
        widget->update(dt);
    }
}

void WidgetLayout::mouseClickEvent(const SDL_MouseButtonEvent *event)
{
    SDL_FPoint mouse_pos = {static_cast<float>(event->x), static_cast<float>(event->y)};
    if (SDL_PointInRectFloat(&mouse_pos, &m_bounding_rect))
    {
        for (auto &child_layout : m_widget_childs)
        {
            if (!child_layout->isVisible() || !child_layout->isActive())
            {
                continue;
            }
            child_layout->checkClick(mouse_pos);
        }
    }
}

Layout::Layout(LayoutProp prop)
{
    m_rect = {0, 0, prop.width, prop.height};
    m_prop = prop;
    m_layout_childs.reserve(4);
    updateCenterPoint();
    updatePosHorizontalBoundingRect();
    updatePosVerticalBoundingRect();
}

void Layout::setRect(SDL_FRect rect)
{
    m_rect = rect;

    recalculateBoundingRect();
    updateCenterPoint();
    updatePosHorizontalBoundingRect();
    updatePosVerticalBoundingRect();
    updateChildPos();
}

void Layout::recalculateBoundingRect()
{
    SDL_FRect maximum_bounding_rect = getMaxmimumBoundingRect();
    m_bounding_rect = {0, 0, 0, 0};
    bool full_bounding_width = false;
    bool full_bounding_height = false;
    for (auto layout : m_layout_childs)
    {
        SDL_FRect layout_rect = layout->getRect();
        if (layout->getInitWidth() <= 0)
        {
            full_bounding_width = true;
            m_bounding_rect.w = maximum_bounding_rect.w;
        }
        if (layout->getInitHeight() <= 0)
        {
            full_bounding_height = true;
            m_bounding_rect.h = maximum_bounding_rect.h;
        }
        if (m_prop.layout_type == LayoutType::HORIZONTAL)
        {
            if (!full_bounding_width)
            {
                m_bounding_rect.w += layout_rect.w;
            }
            m_bounding_rect.h = std::max(m_bounding_rect.h, layout_rect.h);
        }
        else if (m_prop.layout_type == LayoutType::VERTICAL)
        {
            if (!full_bounding_height)
            {
                m_bounding_rect.h += layout_rect.h;
            }
            m_bounding_rect.w = std::max(m_bounding_rect.w, layout_rect.w);
        }
    }
    if (!full_bounding_width)
    {
        m_bounding_rect.w += (m_layout_childs.size() - 1) * m_prop.gap;
    }
    if (!full_bounding_height)
    {
        m_bounding_rect.h += (m_layout_childs.size() - 1) * m_prop.gap;
    }
}

void Layout::updateChildPos()
{
    std::vector<BaseLayout *> flexible_layouts;
    float fixed_space = 0;
    for (auto &child_layout : m_layout_childs)
    {
        float wide_side = 0;
        if (m_prop.layout_type == LayoutType::HORIZONTAL)
        {
            wide_side = child_layout->getInitWidth();
        }
        else if (m_prop.layout_type == LayoutType::VERTICAL)
        {
            wide_side = child_layout->getInitHeight();
        }

        if (wide_side > 0)
        {
            fixed_space += wide_side;
        }
        else
        {
            flexible_layouts.push_back(child_layout);
        }
    }

    float width = 0;
    if (m_prop.layout_type == LayoutType::HORIZONTAL)
    {
        width = m_rect.w - m_prop.padding.l - m_prop.padding.r;
    }
    else if (m_prop.layout_type == LayoutType::VERTICAL)
    {
        width = m_rect.h - m_prop.padding.u - m_prop.padding.d;
    }

    float flexible_space = ((width - (m_layout_childs.size() - 1) * m_prop.gap) - fixed_space) /
                           flexible_layouts.size();

    float offset = 0;
    for (auto &child_layout : m_layout_childs)
    {
        SDL_FRect child_rect = child_layout->getRect();

        float wide_side = 0;
        if (m_prop.layout_type == LayoutType::HORIZONTAL)
        {
            wide_side = child_layout->getInitWidth();
            child_rect.x = m_bounding_rect.x + offset;
            if (wide_side <= 0)
            {
                child_rect.w = flexible_space;
            }
            if (child_layout->getInitHeight() > 0)
            {
                if (m_prop.vertical_anchor == Anchor::START)
                {
                    child_rect.y = m_rect.y + m_prop.padding.u;
                }
                else if (m_prop.vertical_anchor == Anchor::CENTER)
                {
                    child_rect.y = m_center_point.y - child_rect.h * 0.5;
                }
                else if (m_prop.vertical_anchor == Anchor::END)
                {
                    child_rect.y = m_rect.y + m_rect.h - child_rect.h - m_prop.padding.d;
                }
            }
            else
            {
                child_rect.y = m_bounding_rect.y;
                child_rect.h = m_rect.h - m_prop.padding.u - m_prop.padding.d;
            }
        }
        else if (m_prop.layout_type == LayoutType::VERTICAL)
        {
            wide_side = child_layout->getInitHeight();
            child_rect.y = m_bounding_rect.y + offset;
            if (wide_side <= 0)
            {
                child_rect.h = flexible_space;
            }
            if (child_layout->getInitWidth() > 0)
            {
                if (m_prop.horizontal_anchor == Anchor::START)
                {
                    child_rect.x = m_rect.x + m_prop.padding.l;
                }
                else if (m_prop.horizontal_anchor == Anchor::CENTER)
                {
                    child_rect.x = m_center_point.x - child_rect.w * 0.5;
                }
                else if (m_prop.horizontal_anchor == Anchor::END)
                {
                    child_rect.x = m_rect.x + m_rect.w - child_rect.w - m_prop.padding.r;
                }
            }
            else
            {
                child_rect.x = m_bounding_rect.x;
                child_rect.w = m_rect.w - m_prop.padding.l - m_prop.padding.r;
            }
        }

        if (wide_side > 0)
        {
            offset += wide_side + m_prop.gap;
        }
        else
        {
            offset += flexible_space + m_prop.gap;
        }

        child_layout->setRect(child_rect);
    }
}

void Layout::addLayout(BaseLayout *layout)
{
    m_layout_childs.push_back(layout);
    recalculateRect();
}

void Layout::render(SDL_Renderer *renderer)
{
    renderTexture(renderer);
    renderBackground(renderer);
    for (auto &layout : m_layout_childs)
    {
        layout->render(renderer);
    }

#if DEBUG_LAYOUT
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    SDL_RenderPoint(renderer, m_center_point.x, m_center_point.y);
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
    SDL_RenderRect(renderer, &m_bounding_rect);
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    SDL_RenderRect(renderer, &m_rect);
#endif // DEBUG_LAYOUT
}

void Layout::update(float dt)
{
    for (auto &layout : m_layout_childs)
    {
        layout->update(dt);
    }
}

void Layout::mouseClickEvent(const SDL_MouseButtonEvent *event)
{
    SDL_FPoint mouse_pos = {static_cast<float>(event->x), static_cast<float>(event->y)};
    if (SDL_PointInRectFloat(&mouse_pos, &m_bounding_rect))
    {
        for (auto &child_layout : m_layout_childs)
        {
            child_layout->mouseClickEvent(event);
        }
    }
}
