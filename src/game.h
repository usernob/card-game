#ifndef SRC_GAME_H
#define SRC_GAME_H

#include "SDL3/SDL_timer.h"
#include "card.h"
#include "pages.h"
#include <memory>



float getStageScoreMult(int round);

class Game
{
private:
    bool m_exit = false;
    int m_stage_counter = 1;
    int m_round_counter = 1;
    int m_target_score = 200;
    int m_score = 0;
    HandRank m_current_hand_rank = {0, 1};
    int m_coin = 0;
    int discard_counter = 4;
    int play_counter = 5;
    std::unique_ptr<MainMenu> main_menu_page;
    std::unique_ptr<GamePage> game_page;
    Pages *current_page;
    unsigned int last_tick = 0;
    int last_card_index = 0;

    enum class State
    {
        MAIN_MENU,
        GAME,
        GAME_CALCULATING,
        GAME_SCORING
    } state;

public:
    CardManager card_manager;

    Game();

    ~Game();

    void toMainMenu();

    void toGame();

    void render(SDL_Renderer *renderer);

    void update(float dt);

    void registerMouseEvents(SDL_Event *event);

    void nextRound();

    void nextStage();

    void requestExit();

    void updateComboWidget();

    void updateHandCardsWidget();

    void hidePlayedCardsWidget();

    void updatePlayedCardsWidget();

    void selectCard(int index);

    void playHandSelectedCards();

    void discardHandSelectedCards();

    void updateInfoRound();

    void newRound();

    void newGame();

    bool exit();
};

#endif // SRC_GAME_H
