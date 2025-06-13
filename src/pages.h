#ifndef SRC_PAGES_H
#define SRC_PAGES_H

#include "layout.h"
#include "typedef.h"
#include "widget.h"
#include <SDL3/SDL_log.h>
#include <array>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>

/* class for creating pages.
 * WARN: important! read this before using.
 * This class using builder pattern, so be carreful when using it.
 * When creating layouts you must use the beginLayout method and endLayout for closing.
 * Layout can be nested with other layouts or widget layouts but not with widgets.
 * addWidget for adding widgets only works inside widget layouts.
 * so before adding widgets you must use beginWidgetLayout.
 * after adding widgets you must use endWidgetLayout.
 * The begin* and end* method are important make sure you call them in the right order.
 * rimsvek for claude ai for the inspiration.
 */
class Page
{
public:
    std::unordered_map<std::string, std::unique_ptr<Widget>> widgets;             // Weak references
    std::unordered_map<std::string, std::unique_ptr<Layout>> layouts;             // Weak references
    std::unordered_map<std::string, std::unique_ptr<WidgetLayout>> widgetlayouts; // Weak references
    std::stack<Layout *> layoutstack;
    Layout *currentLayout = nullptr;
    WidgetLayout *currentWidgetLayout = nullptr;

    Page(LayoutProp prop);

    ~Page();

    template <typename T, typename... Args>
    Page &addWidget(const std::string &id, T **widgetptr, Args &&...args);

    template <typename T> T *getWidget(const std::string &id);

    Layout *getLayout(const std::string &id);

    WidgetLayout *getWidgetLayout(const std::string &id);

    Page &beginLayout(const std::string &id, Layout **layoutptr, LayoutProp prop = LayoutProp());

    Page &endLayout();

    Page &beginWidgetLayout(
        const std::string &id,
        WidgetLayout **widgetlayoutptr,
        LayoutProp prop = LayoutProp()
    );

    Page &endWidgetLayout();

    Page &setLayoutBackgroundColor(SDL_Color color);

    Page &setWidgetLayoutBackgroundColor(SDL_Color color);

    Page &setLayoutTexture(SDL_Texture *texture, SDL_FRect rect);

    Page &setWidgetLayoutTexture(SDL_Texture *texture, SDL_FRect rect);

    Layout *getRootLayout();
};

// forward declaration
class Game;

class Pages
{
protected:
    std::unordered_map<std::string, std::unique_ptr<Page>> pages;
    std::vector<Page *> pagestack;
    Game *game_ref;

public:
    Pages(Game *game) : game_ref(game) {};
    Page *get(const std::string &id);

    Page *getCurrent();

    void push(Page *page);

    void pop();

    Page *create(const std::string &id, LayoutProp prop);

    virtual void registerMouseEvents(SDL_Event *event);

    virtual void render(SDL_Renderer *renderer);

    virtual void update(float dt);

    void clear();
};

class MainMenu : public Pages
{
public:
    MainMenu(Game *game);
};

class GamePage : public Pages
{
public:
    std::array<CardWidget *, 6> tarot_active;
    std::array<CardWidget *, 9> hand_card;
    std::array<CardWidget *, 5> played_card;

    Label *round_label_counter;
    Label *stage_label_counter;
    Label *target_score_label;
    Label *score_label;
    Label *coin_label;
    Label *combo_name;
    Label *combo_chips;
    Label *combo_mult;

    PrimaryButton *play_button;
    PrimaryButton *discard_button;

    Label *play_counter;
    Label *discard_counter;

    Page *win_round_overlay;
    Label *play_reward;
    Label *discard_reward;
    Label *total_reward;

    Page *game_over_overlay;
    GamePage(Game *game);
};

#endif // SRC_PAGES_H
