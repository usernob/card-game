#include "game.h"
#include <string>

float getStageScoreMult(int stage)
{
    switch (stage)
    {
        case 1:
            return 1.0f;
        case 2:
            return 1.12f;
        case 3:
            return 1.32f;
        default:
            return 1.0f;
    }
}

Game::Game()
{
    card_manager.resetCards();
    main_menu_page = std::make_unique<MainMenu>(this);
    game_page = std::make_unique<GamePage>(this);
    current_page = main_menu_page.get();
}

Game::~Game()
{
    main_menu_page.reset();
    game_page.reset();
}

void Game::toMainMenu()
{
    current_page = main_menu_page.get();
    state = State::MAIN_MENU;
}

void Game::toGame()
{
    current_page = game_page.get();
    state = State::GAME;

    game_page->play_counter->setText(std::string("Play: " + std::to_string(play_counter)).c_str());
    game_page->play_button->setActive(true);
    game_page->discard_counter->setText(
        std::string("Discard: " + std::to_string(discard_counter)).c_str()
    );
    game_page->discard_button->setActive(true);

    newGame();
    newRound();
}

void Game::render(SDL_Renderer *renderer)
{
    current_page->render(renderer);
}

void Game::update(float dt)
{
    if (state == State::GAME_CALCULATING)
    {
        unsigned int current_tick = SDL_GetTicks();
        if (current_tick - last_tick > 500)
        {
            last_tick = current_tick;
            if (last_card_index < card_manager.m_played_cards.size())
            {
                auto &card = card_manager.m_played_cards[last_card_index];
                m_current_hand_rank.chips += static_cast<int>(card->rank);
                updateComboWidget();
                if (last_card_index != 0)
                {
                    game_page->played_card[last_card_index - 1]->setRenderScore(false);
                }
                game_page->played_card[last_card_index]->setRenderScore(true);
                last_card_index++;
            }
            else
            {
                game_page->played_card[last_card_index - 1]->setRenderScore(false);
                m_score += m_current_hand_rank.chips * m_current_hand_rank.multiplier;
                updateInfoRound();
                state = State::GAME_SCORING;
            }
        }
    }
    if (state == State::GAME_SCORING)
    {
        unsigned int current_tick = SDL_GetTicks();
        if (current_tick - last_tick > 1000)
        {
            last_tick = current_tick;

            if (m_score >= m_target_score)
            {
                nextRound();
            }
            else
            {
                card_manager.m_played_cards.clear();
                m_current_hand_rank = {0, 1};
                card_manager.m_hand_name = "High Card";
                updateComboWidget();
                updateInfoRound();
                game_page->play_button->setActive(true);
                game_page->discard_button->setActive(true);
                card_manager.getNewShowedCards();
                updateHandCardsWidget();
                hidePlayedCardsWidget();
                game_page->play_counter->setText(
                    std::string("Play: " + std::to_string(play_counter)).c_str()
                );
                game_page->discard_counter->setText(
                    std::string("Discard: " + std::to_string(discard_counter)).c_str()
                );

                if (play_counter == 0)
                {
                    game_page->push(game_page->game_over_overlay);
                }
            }
            state = State::GAME;
        }
    }
    current_page->update(dt);
}

void Game::updateHandCardsWidget()
{
    for (int i = 0; i < game_page->hand_card.size(); i++)
    {
        auto hand_card = game_page->hand_card[i];
        if (i < card_manager.m_hand_cards.size())
        {
            hand_card->setCard(card_manager.m_hand_cards[i].get());
            hand_card->resetPosition();
            hand_card->setVisible(true);
            hand_card->setActive(true);
        }
        else
        {
            hand_card->setVisible(false);
        }
    }
}

void Game::registerMouseEvents(SDL_Event *event)
{
    current_page->registerMouseEvents(event);
}

void Game::nextStage()
{
    m_stage_counter++;
    card_manager.resetCards();

    m_round_counter = 1;
    m_target_score += 200;
}

void Game::nextRound()
{
    if (m_round_counter == 3)
    {
        nextStage();
    }
    else
    {
        m_round_counter++;
    }

    int play_reward = play_counter / 2;
    int discard_reward = discard_counter / 2;
    int total = 1 + play_reward + discard_reward; // base + play + discard
    game_page->play_reward->setText(std::string("$" + std::to_string(play_reward)).c_str());
    game_page->discard_reward->setText(std::string("$" + std::to_string(discard_reward)).c_str());
    game_page->total_reward->setText(std::string("$" + std::to_string(total)).c_str());

    m_coin += total;
    game_page->push(game_page->win_round_overlay);
}

void Game::requestExit()
{
    m_exit = true;
}

bool Game::exit()
{
    return m_exit;
}

void Game::updateComboWidget()
{
    game_page->combo_name->setText(card_manager.m_hand_name.c_str());
    game_page->combo_chips->setText(std::to_string(m_current_hand_rank.chips).c_str());
    game_page->combo_mult->setText(std::to_string(m_current_hand_rank.multiplier).c_str());
}

void Game::hidePlayedCardsWidget()
{
    for (int i = 0; i < game_page->played_card.size(); i++)
    {
        auto played_card = game_page->played_card[i];
        played_card->setVisible(false);
        played_card->setCard(nullptr);
    }
}

void Game::updatePlayedCardsWidget()
{
    for (int i = 0; i < card_manager.m_played_cards.size(); i++)
    {
        auto played_card = game_page->played_card[i];
        played_card->setVisible(true);
        played_card->setCard(card_manager.m_played_cards[i].get());
    }
}

void Game::selectCard(int index)
{
    card_manager.selectCard(index);
    m_current_hand_rank = card_manager.hand_ranks[card_manager.m_hand_name];
    updateComboWidget();
}

void Game::playHandSelectedCards()
{
    if (play_counter <= 0 || card_manager.count_selected_card == 0) return;
    play_counter--;
    card_manager.playSelectedCards();
    updatePlayedCardsWidget();
    game_page->play_counter->setText(std::string("Play: " + std::to_string(play_counter)).c_str());
    if (play_counter == 0)
    {
        game_page->play_counter->setActive(false);
    }
    last_tick = SDL_GetTicks();
    state = State::GAME_CALCULATING;
    last_card_index = 0;
    auto *card_rank = &card_manager.hand_ranks[card_manager.m_hand_name];
    card_rank->used++;
    card_rank->chips += card_rank->used % 3 == 0 ? 10 : 0;
    card_rank->multiplier += card_rank->used % 3 == 0 ? 1 : 0;

    m_current_hand_rank = *card_rank;
    for (int i = 0; i < game_page->hand_card.size(); i++)
    {
        auto hand_card = game_page->hand_card[i];
        if (i < card_manager.m_hand_cards.size())
        {
            hand_card->setActive(false);
        }
    }
    game_page->play_button->setActive(false);
    game_page->discard_button->setActive(false);
    updateHandCardsWidget();
}

void Game::discardHandSelectedCards()
{
    if (discard_counter <= 0 || card_manager.count_selected_card == 0) return;
    discard_counter--;
    card_manager.deleteSelectedCards();
    updateHandCardsWidget();
    game_page->discard_counter->setText(
        std::string("Discard: " + std::to_string(discard_counter)).c_str()
    );
    if (discard_counter == 0)
    {
        game_page->discard_counter->setActive(false);
    }
}

void Game::updateInfoRound()
{
    game_page->round_label_counter->setText(std::to_string(m_round_counter).c_str());
    game_page->stage_label_counter->setText(std::to_string(m_stage_counter).c_str());
    game_page->target_score_label->setText(std::to_string(m_target_score).c_str());
    game_page->score_label->setText(std::to_string(m_score).c_str());
    game_page->coin_label->setText(std::to_string(m_coin).c_str());
}

void Game::newRound()
{
    card_manager.m_played_cards.clear();
    card_manager.m_hand_name = "High Card";
    m_current_hand_rank = card_manager.hand_ranks[card_manager.m_hand_name];
    m_score = 0;
    m_target_score = m_target_score * getStageScoreMult(m_round_counter);
    play_counter = 4;
    discard_counter = 5;
    updateComboWidget();
    updateInfoRound();
    game_page->play_button->setActive(true);
    game_page->discard_button->setActive(true);
    card_manager.getNewShowedCards();
    updateHandCardsWidget();
    hidePlayedCardsWidget();
    game_page->play_counter->setText(std::string("Play: " + std::to_string(play_counter)).c_str());
    game_page->discard_counter->setText(
        std::string("Discard: " + std::to_string(discard_counter)).c_str()
    );
}

void Game::newGame()
{
    card_manager.m_hand_cards.clear();
    m_stage_counter = 1;
    m_round_counter = 1;
    m_target_score = 200;
    m_score = 0;
    m_current_hand_rank = {0, 1};
    m_coin = 0;
    discard_counter = 4;
    play_counter = 5;
}
