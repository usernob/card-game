#ifndef SRC_LAYOUT_H
#define SRC_LAYOUT_H

#include "typedef.h"
#include "widget.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <vector>

// WARNING: this class is very buggy absolutely buggy, but it seems work fine sometimes.
// first rule:
// dont activate the debug layout, bcs you will see the bug if you do
class BaseLayout
{
protected:
    SDL_FRect m_rect;
    SDL_FRect m_bounding_rect = {0, 0, 0, 0};
    LayoutProp m_prop;
    SDL_FPoint m_center_point;
    SDL_Color m_background_color = {0, 0, 0, 0};
    SDL_Texture *m_background_texture = nullptr;
    SDL_FRect m_background_texture_rect = {0, 0, 0, 0};

    void updateCenterPoint();

    void updatePosHorizontalBoundingRect();

    void updatePosVerticalBoundingRect();

    virtual void updateChildPos() = 0;

public:
    void registerMouseEvents(const SDL_Event *event);


    virtual void mouseClickEvent(const SDL_MouseButtonEvent *event) = 0;

    virtual SDL_FRect getRect();

    virtual SDL_FRect getBoundingRect();

    virtual SDL_FRect getMaxmimumBoundingRect();

    virtual void recalculateRect();

    virtual void setRect(SDL_FRect rect);

    float getInitWidth();

    float getInitHeight();

    void setBackgroundColor(SDL_Color color);

    void setBackgroundTexture(SDL_Texture *texture, SDL_FRect rect);

    void renderBackground(SDL_Renderer *renderer);

    void renderTexture(SDL_Renderer *renderer);

    virtual void render(SDL_Renderer *renderer) = 0;

    virtual void update(float dt) = 0;
};

class WidgetLayout : public BaseLayout
{
private:
    std::vector<Widget *> m_widget_childs;

    void updateChildPos() override;

    void addWidgetHorizontal(Widget *widget);

    void addWidgetVertical(Widget *widget);

public:
    WidgetLayout(LayoutProp prop);

    void mouseClickEvent(const SDL_MouseButtonEvent *event) override;

    void setRect(SDL_FRect rect) override;

    void recalculateBoundingRect();

    void addWidget(Widget *widget);

    void render(SDL_Renderer *renderer) override;

    void update(float dt) override;
};

class Layout : public BaseLayout
{
private:
    std::vector<BaseLayout *> m_layout_childs;
    bool full_bounding_rect_width = false;
    bool full_bounding_rect_height = false;

    void updateChildPos() override;

public:
    Layout(LayoutProp prop);

    void mouseClickEvent(const SDL_MouseButtonEvent *event) override;

    void setRect(SDL_FRect rect) override;

    void recalculateBoundingRect();

    void addLayout(BaseLayout *layout);

    void render(SDL_Renderer *renderer) override;

    void update(float dt) override;
};

#endif // SRC_LAYOUT_H
