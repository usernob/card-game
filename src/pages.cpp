#include "pages.h"
#include "SDL3/SDL_rect.h"
#include "game.h"
#include "layout.h"
#include "textrenderer.h"
#include "texturemanager.h"
#include "typedef.h"
#include "widget.h"
#include <SDL3/SDL_blendmode.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_pixels.h>
#include <string>

Page::Page(LayoutProp prop)
{
    auto rootlayout = std::make_unique<Layout>(prop);
    Layout *ptr = rootlayout.get(); // important! : must get the pointer before moving
    layouts["root"] = std::move(rootlayout);
    currentLayout = ptr;
    layoutstack.push(ptr);
}

Page::~Page()
{
    layouts.clear();
    widgetlayouts.clear();
    widgets.clear();
}

template <typename T, typename... Args>
Page &Page::addWidget(const std::string &id, T **widgetptr, Args &&...args)
{
    if (currentWidgetLayout == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No current widget layout");
        return *this;
    }
    auto widget = std::make_unique<T>(currentWidgetLayout, std::forward<Args>(args)...);

    widgets[id] = std::move(widget);
    T *ptr = static_cast<T *>(widgets[id].get());
    currentWidgetLayout->addWidget(ptr);

    if (widgetptr != nullptr)
    {
        *widgetptr = ptr;
    }
    return *this;
}

template <typename T> T *Page::getWidget(const std::string &id)
{
    auto it = widgets.find(id);
    if (it != widgets.end())
    {
        return static_cast<T *>(it->second.get());
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Widget \"%s\" not found", id.c_str());
        return nullptr;
    }
}

Layout *Page::getLayout(const std::string &id)
{
    auto it = layouts.find(id);
    if (it != layouts.end())
    {
        return it->second.get();
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Layout \"%s\" not found", id.c_str());
        return nullptr;
    }
}

WidgetLayout *Page::getWidgetLayout(const std::string &id)
{
    auto it = widgetlayouts.find(id);
    if (it != widgetlayouts.end())
    {
        return it->second.get();
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "WidgetLayout \"%s\" not found", id.c_str());
        return nullptr;
    }
}

Page &Page::beginLayout(const std::string &id, Layout **layoutptr, LayoutProp prop)
{
    auto layout = std::make_unique<Layout>(prop);
    Layout *ptr = layout.get();
    currentLayout->addLayout(ptr);
    currentLayout = ptr;
    layoutstack.push(ptr);
    if (layouts.contains(id))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Layout \"%s\" already exists", id.c_str());
        return *this;
    }
    layouts[id] = std::move(layout);
    if (layoutptr != nullptr)
    {
        *layoutptr = ptr;
    }
    return *this;
}

Page &Page::endLayout()
{
    layoutstack.pop();
    currentLayout->recalculateRect();
    currentLayout = layoutstack.top();
    return *this;
}

Page &Page::beginWidgetLayout(
    const std::string &id,
    WidgetLayout **widgetlayoutptr,
    LayoutProp prop
)
{
    auto widgetlayout = std::make_unique<WidgetLayout>(prop);
    WidgetLayout *ptr = widgetlayout.get();
    currentLayout->addLayout(ptr);
    currentWidgetLayout = ptr;
    if (widgetlayouts.contains(id))
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "WidgetLayout \"%s\" already exists", id.c_str());
        return *this;
    }
    widgetlayouts[id] = std::move(widgetlayout);
    if (widgetlayoutptr != nullptr)
    {
        *widgetlayoutptr = ptr;
    }
    return *this;
}

Page &Page::endWidgetLayout()
{
    currentWidgetLayout->recalculateRect();
    currentWidgetLayout = nullptr;
    return *this;
}

Page &Page::setLayoutBackgroundColor(SDL_Color color)
{
    currentLayout->setBackgroundColor(color);
    return *this;
};

Page &Page::setWidgetLayoutBackgroundColor(SDL_Color color)
{
    if (currentWidgetLayout == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No current widget layout");
        return *this;
    }
    currentWidgetLayout->setBackgroundColor(color);
    return *this;
};

Page &Page::setLayoutTexture(SDL_Texture *texture, SDL_FRect rect)
{
    currentLayout->setBackgroundTexture(texture, rect);
    return *this;
}

Page &Page::setWidgetLayoutTexture(SDL_Texture *texture, SDL_FRect rect)
{
    if (currentWidgetLayout == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No current widget layout");
        return *this;
    }
    currentWidgetLayout->setBackgroundTexture(texture, rect);
    return *this;
}

Layout *Page::getRootLayout()
{
    return static_cast<Layout *>(layouts["root"].get());
}

Page *Pages::get(const std::string &id)
{
    auto it = pages.find(id);
    if (it != pages.end())
    {
        return it->second.get();
    }
    else
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Page \"%s\" not found", id.c_str());
        return nullptr;
    }
}

Page *Pages::getCurrent()
{
    return pagestack.back();
}

void Pages::push(Page *page)
{
    pagestack.push_back(page);
}

void Pages::pop()
{
    pagestack.pop_back();
}

Page *Pages::create(const std::string &id, LayoutProp prop)
{
    auto page = std::make_unique<Page>(prop);
    Page *ptr = page.get();
    pages[id] = std::move(page);
    if (pagestack.empty())
    {
        pagestack.push_back(ptr);
    }
    return ptr;
}

void Pages::registerMouseEvents(SDL_Event *event)
{
    if (pagestack.empty()) return;
    pagestack.back()->getRootLayout()->registerMouseEvents(event);
}

void Pages::render(SDL_Renderer *renderer)
{
    if (pagestack.empty()) return;
    bool first = true;
    for (auto page : pagestack)
    {
        if (!first)
        {
            // draw backdrop
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE * 0.4);
            SDL_RenderFillRect(renderer, nullptr);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }
        page->getRootLayout()->render(renderer);
        if (first) first = false;
    }
}

void Pages::update(float dt)
{
    if (pagestack.empty()) return;
    pagestack.back()->getRootLayout()->update(dt);
}

void Pages::clear()
{
    pages.clear();
    pagestack.clear();
}

MainMenu::MainMenu(Game *game) : Pages(game)
{
    Font *font = FontsManager::getFont("font1-w");
    Label *label1;
    MainButton *playGamebtn;
    MainButton *exitGamebtn;
    TextureAtlas *atlas = TextureManager::instance()->getAtlas("ui-atlas");
    Page *page = create(
        "game",
        LayoutProp{
            .layout_type = LayoutType::VERTICAL,
            .width = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT,
            .horizontal_anchor = Anchor::CENTER,
            .vertical_anchor = Anchor::CENTER,
            .gap = 10
        }
    );

    page->beginWidgetLayout(
            "menu",
            nullptr,
            LayoutProp{
                .layout_type = LayoutType::VERTICAL,
                .horizontal_anchor = Anchor::CENTER,
                .vertical_anchor = Anchor::CENTER,
                .gap = 10
            }
    )
        .addWidget<Label>("label1", &label1, Text(font, "Main Menu"))
        .addWidget<MainButton>("play", &playGamebtn, Text(font, "Play"))
        .addWidget<MainButton>("exit", &exitGamebtn, Text(font, "Exit"))
        .endWidgetLayout();

    playGamebtn->onClick([this](SDL_FPoint mouse) { this->game_ref->toGame(); });
    exitGamebtn->onClick([this](SDL_FPoint mouse) { this->game_ref->requestExit(); });
    playGamebtn->setBackgroundTexture(atlas->getAtlas(), atlas->getTextureInfo("button-big").rect);
    exitGamebtn->setBackgroundTexture(atlas->getAtlas(), atlas->getTextureInfo("button-big").rect);
}

GamePage::GamePage(Game *game) : Pages(game)
{
    Page *page = create(
        "game",
        LayoutProp{
            .layout_type = LayoutType::HORIZONTAL,
            .width = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT,
            .horizontal_anchor = Anchor::CENTER,
            .vertical_anchor = Anchor::CENTER,
            .gap = 10,
            .padding = {20, 30, 20, 30}
        }
    );

    TextureAtlas *atlas = TextureManager::instance()->getAtlas("ui-atlas");

    PrimaryButton *combo_info_button;
    PrimaryButton *exit_button;
    page->beginLayout(
            "sidebar_container",
            nullptr,
            LayoutProp{
                .layout_type = LayoutType::VERTICAL,
                .width = 320,
                .horizontal_anchor = Anchor::CENTER,
                .gap = 15,
                .padding = {10, 10, 10, 10}
            }
    )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-2").rect)
        .beginLayout(
            "stage_info",
            nullptr,
            LayoutProp{
                .layout_type = LayoutType::HORIZONTAL,
                .height = 120,
                .horizontal_anchor = Anchor::CENTER,
                .vertical_anchor = Anchor::CENTER,
                .gap = 10,
            }
        )

        .beginLayout(
            "stage_label_group",
            nullptr,
            LayoutProp{
                .layout_type = LayoutType::VERTICAL,
                .horizontal_anchor = Anchor::CENTER,
                .padding = {10, 10, 10, 10}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-4").rect)
        .beginWidgetLayout(
            "stage_label_text_container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::CENTER, .fit_content = true}
        )
        .addWidget<Label>(
            "stage_label_text",
            nullptr,
            Text(FontsManager::getFont("font2-w"), "stage")
        )
        .endWidgetLayout()

        .beginWidgetLayout(
            "stage_label_counter_container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::CENTER}
        )
        .setWidgetLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .addWidget<Label>(
            "stage_label_counter",
            &stage_label_counter,
            Text(FontsManager::getFont("font1-w"), "1"),
            Float4{5, 15, 5, 15}
        )
        .endWidgetLayout()
        .endLayout()

        .beginLayout(
            "round_label_group",
            nullptr,
            LayoutProp{
                .layout_type = LayoutType::VERTICAL,
                .horizontal_anchor = Anchor::CENTER,
                .padding = {10, 10, 10, 10}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-4").rect)
        .beginWidgetLayout(
            "round_label_text_container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::CENTER, .fit_content = true}
        )
        .addWidget<Label>(
            "round_label_text",
            nullptr,
            Text(FontsManager::getFont("font2-w"), "Round")
        )
        .endWidgetLayout()

        .beginWidgetLayout(
            "round_label_counter_container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::CENTER}
        )
        .setWidgetLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .addWidget<Label>(
            "round_label_counter",
            &round_label_counter,
            Text(FontsManager::getFont("font1-w"), "1"),
            Float4{5, 15, 5, 15}
        )
        .endWidgetLayout()
        .endLayout()

        .endLayout()


        .beginLayout(
            "target_score_label_group",
            nullptr,
            LayoutProp{
                .layout_type = LayoutType::VERTICAL,
                .height = 120,
                .gap = 10,
                .padding = {10, 10, 10, 10},
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-4").rect)
        .beginWidgetLayout("target_score_label_container", nullptr, LayoutProp{.fit_content = true})
        .addWidget<Label>(
            "target_score_label_text",
            nullptr,
            Text(FontsManager::getFont("font2-w"), "Target Score")
        )
        .endWidgetLayout()
        .beginWidgetLayout(
            "target_score_label_counter_container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END}
        )
        .setWidgetLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .addWidget<Label>(
            "target_score_label_counter",
            &target_score_label,
            Text(FontsManager::getFont("font1-w"), "0"),
            Float4{5, 15, 5, 15}
        )
        .endWidgetLayout()
        .endLayout()


        .beginLayout(
            "current_score_label_group",
            nullptr,
            LayoutProp{
                .layout_type = LayoutType::VERTICAL,
                .height = 120,
                .gap = 10,
                .padding = {10, 10, 10, 10},
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-4").rect)
        .beginWidgetLayout(
            "current_score_label_container",
            nullptr,
            LayoutProp{.fit_content = true}
        )
        .addWidget<Label>(
            "current_score_label_text",
            nullptr,
            Text(FontsManager::getFont("font2-w"), "Score")
        )
        .endWidgetLayout()
        .beginWidgetLayout(
            "current_score_label_counter_container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END}
        )
        .setWidgetLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .addWidget<Label>(
            "current_score_label_counter",
            &score_label,
            Text(FontsManager::getFont("font1-w"), "0"),
            Float4{5, 15, 5, 15}
        )
        .endWidgetLayout()
        .endLayout()


        .beginLayout(
            "combo_container",
            nullptr,
            LayoutProp{
                .layout_type = LayoutType::VERTICAL,
                .height = 120,
                .gap = 10,
                .padding = {10, 10, 10, 10},
            }
        )
        .beginWidgetLayout("combo_label_container", nullptr, LayoutProp{.fit_content = true})
        .addWidget<Label>(
            "combo_label",
            &combo_name,
            Text(FontsManager::getFont("font2-w"), "Royal Flush")
        )
        .endWidgetLayout()

        .beginLayout(
            "combo_mult_container",
            nullptr,
            LayoutProp{.vertical_anchor = Anchor::CENTER, .gap = 5}
        )
        .beginWidgetLayout(
            "combo_value_container",
            nullptr,
            {.horizontal_anchor = Anchor::END, .padding = {5, 5, 5, 5}}
        )
        .setWidgetLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .addWidget<Label>("combo_value", &combo_chips, Text(FontsManager::getFont("font1-w"), "1"))
        .endWidgetLayout()
        .beginWidgetLayout("x_label_container", nullptr, {.fit_content = true})
        .addWidget<Label>("x_label", nullptr, Text(FontsManager::getFont("font2-w"), "x"))
        .endWidgetLayout()
        .beginWidgetLayout(
            "combo_mult_container",
            nullptr,
            {.horizontal_anchor = Anchor::START, .padding = {5, 5, 5, 5}}
        )
        .setWidgetLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .addWidget<Label>("combo_mult", &combo_mult, Text(FontsManager::getFont("font1-w"), "1"))
        .endWidgetLayout()
        .endLayout()

        .endLayout()

        .beginLayout("padding", nullptr)
        .endLayout()

        .beginLayout(
            "coin_group",
            nullptr,
            LayoutProp{
                .layout_type = LayoutType::VERTICAL,
                .height = 120,
                .gap = 10,
                .padding = {10, 10, 10, 10},
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-4").rect)
        .beginWidgetLayout("coin_label_container", nullptr, LayoutProp{.fit_content = true})
        .addWidget<Label>(
            "coin_label_text",
            nullptr,
            Text(FontsManager::getFont("font2-w"), "Coin")
        )
        .endWidgetLayout()
        .beginWidgetLayout(
            "coin_label_counter_container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END}
        )
        .setWidgetLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .addWidget<Label>(
            "coin_label_counter",
            &coin_label,
            Text(FontsManager::getFont("font1-w"), "0"),
            Float4{5, 15, 5, 15}
        )
        .endWidgetLayout()
        .endLayout()


        .beginWidgetLayout(
            "button_group",
            nullptr,
            {.layout_type = LayoutType::VERTICAL, .height = 120, .gap = 10}
        )
        .addWidget<PrimaryButton>(
            "combo_info_button",
            &combo_info_button,
            Text(FontsManager::getFont("font2-w"), "Combo Info"),
            "button-1",
            Float4{10, 35, 10, 35}
        )
        .addWidget<PrimaryButton>(
            "exit_button",
            &exit_button,
            Text(FontsManager::getFont("font2-w"), "End Game"),
            "button-1",
            Float4{10, 35, 10, 35}
        )
        .endWidgetLayout()

        .endLayout();

    exit_button->onClick([=](SDL_FPoint pos) { game->toMainMenu(); });



    Layout *tarort_container;
    WidgetLayout *tarot_layout;

    page->beginLayout(
            "content_container",
            nullptr,
            LayoutProp{.layout_type = LayoutType::VERTICAL, .gap = 15}
    )
        // TODO: add tarot cards
        // .beginLayout("tarot_container", &tarort_container, LayoutProp{.height = 200})
        // .beginWidgetLayout(
        //     "tarot_layout",
        //     &tarot_layout,
        //     LayoutProp{
        //         .layout_type = LayoutType::HORIZONTAL,
        //         .horizontal_anchor = Anchor::CENTER,
        //         .vertical_anchor = Anchor::CENTER,
        //         .gap = 10
        //     }
        // )
        // .addWidget<CardWidget>("tarot-1", &tarot_active[0], nullptr)
        // .addWidget<CardWidget>("tarot-2", &tarot_active[1], nullptr)
        // .addWidget<CardWidget>("tarot-3", &tarot_active[2], nullptr)
        // .addWidget<CardWidget>("tarot-4", &tarot_active[3], nullptr)
        // .addWidget<CardWidget>("tarot-5", &tarot_active[4], nullptr)
        // .addWidget<CardWidget>("tarot-6", &tarot_active[5], nullptr)
        // .endWidgetLayout()
        // .beginWidgetLayout(
        //     "tarot_action",
        //     nullptr,
        //     LayoutProp{
        //         .layout_type = LayoutType::VERTICAL,
        //         .width = 150,
        //         .horizontal_anchor = Anchor::CENTER,
        //         .vertical_anchor = Anchor::CENTER,
        //         .gap = 10
        //     }
        // )
        // .addWidget<PrimaryButton>(
        //     "button_tarot_sell",
        //     nullptr,
        //     Text(FontsManager::getFont("font2-w"), "Sell"),
        //     "button-3"
        // )
        // .endWidgetLayout()
        // .endLayout()

        .beginLayout(
            "cards_container",
            nullptr,
            LayoutProp{.layout_type = LayoutType::VERTICAL, .gap = 20}
        )
        .beginWidgetLayout(
            "played_hand",
            nullptr,
            LayoutProp{
                .horizontal_anchor = Anchor::CENTER,
                .vertical_anchor = Anchor::END,
                .gap = 10
            }
        )
        .addWidget<CardWidget>("played_card-1", &played_card[0], nullptr)
        .addWidget<CardWidget>("played_card-2", &played_card[1], nullptr)
        .addWidget<CardWidget>("played_card-3", &played_card[2], nullptr)
        .addWidget<CardWidget>("played_card-4", &played_card[3], nullptr)
        .addWidget<CardWidget>("played_card-5", &played_card[4], nullptr)
        .endWidgetLayout()

        .beginWidgetLayout(
            "hand",
            nullptr,
            LayoutProp{
                .horizontal_anchor = Anchor::CENTER,
                .vertical_anchor = Anchor::START,
                .gap = 10
            }
        )
        .addWidget<CardWidget>("hand_card-1", &hand_card[0], nullptr)
        .addWidget<CardWidget>("hand_card-2", &hand_card[1], nullptr)
        .addWidget<CardWidget>("hand_card-3", &hand_card[2], nullptr)
        .addWidget<CardWidget>("hand_card-4", &hand_card[3], nullptr)
        .addWidget<CardWidget>("hand_card-5", &hand_card[4], nullptr)
        .addWidget<CardWidget>("hand_card-6", &hand_card[5], nullptr)
        .addWidget<CardWidget>("hand_card-7", &hand_card[6], nullptr)
        .addWidget<CardWidget>("hand_card-8", &hand_card[7], nullptr)
        .addWidget<CardWidget>("hand_card-9", &hand_card[8], nullptr)
        .endWidgetLayout()
        .endLayout()

        .beginLayout(
            "action_container",
            nullptr,
            {.layout_type = LayoutType::HORIZONTAL, .height = 100, .gap = 10}
        )
        .beginWidgetLayout(
            "hand_counter_action",
            nullptr,
            LayoutProp{
                .width = 300,
                .horizontal_anchor = Anchor::CENTER,
                .vertical_anchor = Anchor::CENTER,
                .gap = 20
            }
        )
        .addWidget<Label>(
            "play_counter",
            &play_counter,
            Text(FontsManager::getFont("font2-w"), "Play: 0")
        )
        .addWidget<Label>(
            "discard_counter",
            &discard_counter,
            Text(FontsManager::getFont("font2-w"), "Discard: 0")
        )
        .endWidgetLayout()
        .beginWidgetLayout(
            "hand_action",
            nullptr,
            LayoutProp{
                .horizontal_anchor = Anchor::CENTER,
                .vertical_anchor = Anchor::CENTER,
                .gap = 20
            }
        )
        .addWidget<PrimaryButton>(
            "button_hand_play",
            &play_button,
            Text(FontsManager::getFont("font2-w"), "Play"),
            "button-4"
        )
        .addWidget<PrimaryButton>(
            "button_hand_discard",
            &discard_button,
            Text(FontsManager::getFont("font2-w"), "Discard"),
            "button-5"
        )
        .endWidgetLayout()
        .endLayout()

        .endLayout();


    for (int i = 0; i < hand_card.size(); i++)
    {
        hand_card[i]->onClick([=, this](SDL_FPoint pos) { game_ref->selectCard(i); });
    }

    // hide first
    for (int i = 0; i < played_card.size(); i++)
    {
        played_card[i]->setVisible(false);
        played_card[i]->setActive(false);
    }

    play_button->onClick([=, this](SDL_FPoint pos) { game_ref->playHandSelectedCards(); });
    discard_button->onClick([=, this](SDL_FPoint pos) { game_ref->discardHandSelectedCards(); });


    win_round_overlay = create(
        "win_round_overlay",
        {
            .layout_type = LayoutType::HORIZONTAL,
            .width = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT,
            .horizontal_anchor = Anchor::CENTER,
            .vertical_anchor = Anchor::CENTER,
            .gap = 10,
        }
    );

    PrimaryButton *next_round_button;
    win_round_overlay
        ->beginLayout(
            "container",
            nullptr,
            LayoutProp{
                .layout_type = LayoutType::VERTICAL,
                .width = 600,
                .height = 400,
                .horizontal_anchor = Anchor::CENTER,
                .vertical_anchor = Anchor::CENTER,
                .gap = 10,
                .padding = {20, 20, 20, 20}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container").rect)
        .beginWidgetLayout(
            "label-container",
            nullptr,
            LayoutProp{
                .height = 80,
                .horizontal_anchor = Anchor::CENTER,
                .vertical_anchor = Anchor::CENTER,
            }
        )
        .addWidget<Label>("label", nullptr, Text(FontsManager::getFont("font1-w"), "Round won!"))
        .endWidgetLayout()
        .beginLayout("stats_container", nullptr)
        .beginWidgetLayout(
            "stats_label",
            nullptr,
            LayoutProp{.layout_type = LayoutType::VERTICAL, .gap = 10}
        )
        .addWidget<Label>(
            "base-reward-label",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "Base reward ")
        )
        .addWidget<Label>(
            "play-reward-label",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "Remaining Play ")
        )
        .addWidget<Label>(
            "discard-reward-label",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "Remaining Discard ")
        )
        .addWidget<Label>(
            "total-reward-label",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "Total ")
        )
        .endWidgetLayout()
        .beginLayout("stats_pad", nullptr)
        .endLayout()
        .beginWidgetLayout(
            "stats_text",
            nullptr,
            LayoutProp{
                .layout_type = LayoutType::VERTICAL,
                .horizontal_anchor = Anchor::END,
                .gap = 10
            }
        )
        .addWidget<Label>("base-reward", nullptr, Text(FontsManager::getFont("font3-w"), "$1"))
        .addWidget<Label>("play-reward", &play_reward, Text(FontsManager::getFont("font3-w"), "$1"))
        .addWidget<Label>(
            "discard-reward",
            &discard_reward,
            Text(FontsManager::getFont("font3-w"), "$1")
        )
        .addWidget<Label>(
            "total-reward",
            &total_reward,
            Text(FontsManager::getFont("font3-w"), "$1")
        )
        .endWidgetLayout()
        .endLayout()
        .beginWidgetLayout(
            "next-button-container",
            nullptr,
            LayoutProp{
                .height = 80,
                .horizontal_anchor = Anchor::CENTER,
                .vertical_anchor = Anchor::CENTER
            }
        )
        .addWidget<PrimaryButton>(
            "button",
            &next_round_button,
            Text(FontsManager::getFont("font2-w"), "Next"),
            "button-3"
        )
        .endWidgetLayout()
        .endLayout();

    next_round_button->onClick([=, this](SDL_FPoint pos) {
        this->pop();
        game_ref->newRound();
    });

    Page *combo_info = create(
        "combo_info",
        LayoutProp{
            .layout_type = LayoutType::VERTICAL,
            .width = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT,
            .horizontal_anchor = Anchor::CENTER,
            .vertical_anchor = Anchor::CENTER,
        }
    );

    PrimaryButton *combo_info_close_btn;

    combo_info
        ->beginLayout(
            "combo_info_container",
            nullptr,
            LayoutProp{
                .layout_type = LayoutType::VERTICAL,
                .width = 600,
                .height = 780,
                .gap = 10,
                .padding = {20, 20, 20, 20}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container").rect)

        .beginLayout(
            "combo-container",
            nullptr,
            LayoutProp{
                .height = 55,
                .vertical_anchor = Anchor::CENTER,
                .gap = 20,
                .padding = {10, 20, 10, 20}
            }
        )
        .beginWidgetLayout("combo-label-container", nullptr)
        .addWidget<Label>("combo-label", nullptr, Text(FontsManager::getFont("font3-w"), "combo"))
        .endWidgetLayout()
        .beginWidgetLayout(
            "combo-text-container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END, .gap = 20}
        )
        .addWidget<Label>("combo-text", nullptr, Text(FontsManager::getFont("font3-w"), "value"))
        .addWidget<Label>("combo-used", nullptr, Text(FontsManager::getFont("font3-w"), "used"))
        .endWidgetLayout()
        .endLayout()

        .beginLayout(
            "Straight Flush-container",
            nullptr,
            LayoutProp{
                .height = 55,
                .vertical_anchor = Anchor::CENTER,
                .gap = 20,
                .padding = {10, 20, 10, 20}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .beginWidgetLayout("Straight Flush-label-container", nullptr)
        .addWidget<Label>(
            "Straight Flush-label",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "Straight Flush")
        )
        .endWidgetLayout()
        .beginWidgetLayout(
            "Straight Flush-text-container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END, .gap = 20}
        )
        .addWidget<Label>(
            "Straight Flush-text",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "1 x 2")
        )
        .addWidget<Label>(
            "Straight Flush-used",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "1")
        )
        .endWidgetLayout()
        .endLayout()

        .beginLayout(
            "Four of a Kind-container",
            nullptr,
            LayoutProp{
                .height = 55,
                .vertical_anchor = Anchor::CENTER,
                .gap = 20,
                .padding = {10, 20, 10, 20}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .beginWidgetLayout("Four of a Kind-label-container", nullptr)
        .addWidget<Label>(
            "Four of a Kind-label",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "Four of a Kind")
        )
        .endWidgetLayout()
        .beginWidgetLayout(
            "Four of a Kind-text-container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END, .gap = 20}
        )
        .addWidget<Label>(
            "Four of a Kind-text",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "1 x 2")
        )
        .addWidget<Label>(
            "Four of a Kind-used",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "1")
        )
        .endWidgetLayout()
        .endLayout()


        .beginLayout(
            "Full House-container",
            nullptr,
            LayoutProp{
                .height = 55,
                .vertical_anchor = Anchor::CENTER,
                .gap = 20,
                .padding = {10, 20, 10, 20}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .beginWidgetLayout("Full House-label-container", nullptr)
        .addWidget<Label>(
            "Full House-label",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "Full House")
        )
        .endWidgetLayout()
        .beginWidgetLayout(
            "Full House-text-container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END, .gap = 20}
        )
        .addWidget<Label>(
            "Full House-text",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "1 x 2")
        )
        .addWidget<Label>("Full House-used", nullptr, Text(FontsManager::getFont("font3-w"), "1"))
        .endWidgetLayout()
        .endLayout()


        .beginLayout(
            "Flush-container",
            nullptr,
            LayoutProp{
                .height = 55,
                .vertical_anchor = Anchor::CENTER,
                .gap = 20,
                .padding = {10, 20, 10, 20}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .beginWidgetLayout("Flush-label-container", nullptr)
        .addWidget<Label>("Flush-label", nullptr, Text(FontsManager::getFont("font3-w"), "Flush"))
        .endWidgetLayout()
        .beginWidgetLayout(
            "Flush-text-container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END, .gap = 20}
        )
        .addWidget<Label>("Flush-text", nullptr, Text(FontsManager::getFont("font3-w"), "1 x 2"))
        .addWidget<Label>("Flush-used", nullptr, Text(FontsManager::getFont("font3-w"), "1"))
        .endWidgetLayout()
        .endLayout()


        .beginLayout(
            "Straight-container",
            nullptr,
            LayoutProp{
                .height = 55,
                .vertical_anchor = Anchor::CENTER,
                .gap = 20,
                .padding = {10, 20, 10, 20}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .beginWidgetLayout("Straight-label-container", nullptr)
        .addWidget<Label>(
            "Straight-label",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "Straight")
        )
        .endWidgetLayout()
        .beginWidgetLayout(
            "Straight-text-container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END, .gap = 20}
        )
        .addWidget<Label>("Straight-text", nullptr, Text(FontsManager::getFont("font3-w"), "1 x 2"))
        .addWidget<Label>("Straight-used", nullptr, Text(FontsManager::getFont("font3-w"), "1"))
        .endWidgetLayout()
        .endLayout()

        .beginLayout(
            "Three of a Kind-container",
            nullptr,
            LayoutProp{
                .height = 55,
                .vertical_anchor = Anchor::CENTER,
                .gap = 20,
                .padding = {10, 20, 10, 20}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .beginWidgetLayout("Three of a Kind-label-container", nullptr)
        .addWidget<Label>(
            "Three of a Kind-label",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "Three of a Kind")
        )
        .endWidgetLayout()
        .beginWidgetLayout(
            "Three of a Kind-text-container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END, .gap = 20}
        )
        .addWidget<Label>(
            "Three of a Kind-text",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "1 x 2")
        )
        .addWidget<Label>(
            "Three of a Kind-used",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "1")
        )
        .endWidgetLayout()
        .endLayout()

        .beginLayout(
            "Two Pair-container",
            nullptr,
            LayoutProp{
                .height = 55,
                .vertical_anchor = Anchor::CENTER,
                .gap = 20,
                .padding = {10, 20, 10, 20}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .beginWidgetLayout("Two Pair-label-container", nullptr)
        .addWidget<Label>(
            "Two Pair-label",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "Two Pair")
        )
        .endWidgetLayout()
        .beginWidgetLayout(
            "Two Pair-text-container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END, .gap = 20}
        )
        .addWidget<Label>("Two Pair-text", nullptr, Text(FontsManager::getFont("font3-w"), "1 x 2"))
        .addWidget<Label>("Two Pair-used", nullptr, Text(FontsManager::getFont("font3-w"), "1"))
        .endWidgetLayout()
        .endLayout()

        .beginLayout(
            "Pair-container",
            nullptr,
            LayoutProp{
                .height = 55,
                .vertical_anchor = Anchor::CENTER,
                .gap = 20,
                .padding = {10, 20, 10, 20}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .beginWidgetLayout("Pair-label-container", nullptr)
        .addWidget<Label>("Pair-label", nullptr, Text(FontsManager::getFont("font3-w"), "Pair"))
        .endWidgetLayout()
        .beginWidgetLayout(
            "Pair-text-container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END, .gap = 20}
        )
        .addWidget<Label>("Pair-text", nullptr, Text(FontsManager::getFont("font3-w"), "1 x 2"))
        .addWidget<Label>("Pair-used", nullptr, Text(FontsManager::getFont("font3-w"), "1"))
        .endWidgetLayout()
        .endLayout()

        .beginLayout(
            "High Card-container",
            nullptr,
            LayoutProp{
                .height = 55,
                .vertical_anchor = Anchor::CENTER,
                .gap = 20,
                .padding = {10, 20, 10, 20}
            }
        )
        .setLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container-3").rect)
        .beginWidgetLayout("High Card-label-container", nullptr)
        .addWidget<Label>(
            "High Card-label",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "High Card")
        )
        .endWidgetLayout()
        .beginWidgetLayout(
            "High Card-text-container",
            nullptr,
            LayoutProp{.horizontal_anchor = Anchor::END, .gap = 20}
        )
        .addWidget<Label>(
            "High Card-text",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "1 x 2")
        )
        .addWidget<Label>("High Card-used", nullptr, Text(FontsManager::getFont("font3-w"), "1"))
        .endWidgetLayout()
        .endLayout()

        .beginLayout("padding", nullptr)
        .endLayout()
        .beginWidgetLayout(
            "combo-close-container",
            nullptr,
            {.height = 80, .horizontal_anchor = Anchor::CENTER, .vertical_anchor = Anchor::CENTER}
        )
        .addWidget<PrimaryButton>(
            "combo-close",
            &combo_info_close_btn,
            Text(FontsManager::getFont("font2-w"), "Close"),
            "button-3"
        )
        .endWidgetLayout()
        .endLayout();

    combo_info_button->onClick([=, this](SDL_FPoint pos) {
        Page *combo_info_overlay = this->get("combo_info");
        for (auto rank : game_ref->card_manager.hand_ranks)
        {
            combo_info_overlay->getWidget<Label>(rank.first + "-label")
                ->setText(rank.first.c_str());

            std::string text =
                std::to_string(rank.second.chips) + " x " + std::to_string(rank.second.multiplier);
            combo_info_overlay->getWidget<Label>(rank.first + "-text")->setText(text.c_str());

            combo_info_overlay->getWidget<Label>(rank.first + "-used")
                ->setText(std::to_string(rank.second.used).c_str());
        }
        this->push(combo_info_overlay);
    });
    combo_info_close_btn->onClick([=, this](SDL_FPoint pos) { this->pop(); });


    game_over_overlay = create(
        "game_over",
        LayoutProp{
            .layout_type = LayoutType::VERTICAL,
            .width = WINDOW_WIDTH,
            .height = WINDOW_HEIGHT,
            .horizontal_anchor = Anchor::CENTER,
            .vertical_anchor = Anchor::CENTER,
            .gap = 10
        }
    );

    PrimaryButton *play_again_button;
    PrimaryButton *no_button;
    game_over_overlay
        ->beginWidgetLayout(
            "container",
            nullptr,
            LayoutProp{.layout_type = LayoutType::VERTICAL, .horizontal_anchor = Anchor::CENTER}
        )
        .setWidgetLayoutTexture(atlas->getAtlas(), atlas->getTextureInfo("container").rect)
        .addWidget<Label>("title", nullptr, Text(FontsManager::getFont("font1-w"), "Game Over"))
        .addWidget<Label>(
            "text",
            nullptr,
            Text(FontsManager::getFont("font3-w"), "Do you want to play again?")
        )
        .addWidget<PrimaryButton>(
            "yes",
            &play_again_button,
            Text(FontsManager::getFont("font2-w"), "Yes"),
            "button-3"
        )
        .addWidget<PrimaryButton>(
            "no",
            &no_button,
            Text(FontsManager::getFont("font2-w"), "No"),
            "button-3"
        )
        .endWidgetLayout();

    play_again_button->onClick([=, this](SDL_FPoint pos) {
        this->pop();
        game_ref->toGame();
    });
    no_button->onClick([=, this](SDL_FPoint pos) {
        this->pop();
        game_ref->toMainMenu();
    });
}
