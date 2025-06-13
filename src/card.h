#ifndef SRC_CARD_H
#define SRC_CARD_H

#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class CardRank
{
    TWO = 2,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    TEN,
    JACK,
    QUEEN,
    KING,
    ACE
};

enum class CardSuits
{
    CLUBS,    // ♣
    SPADES,   // ♠
    DIAMONDS, // ♦
    HEARTS    // ♥
};


std::string getCardName(CardRank value);

std::string getCardSuit(CardSuits suit);

struct Card
{
    std::string name;
    SDL_Texture *atlas;
    SDL_FRect tex_rect;
    CardRank rank;
    CardSuits suit;
    bool selected = false;
};

struct HandRank
{
    int chips;
    int multiplier;
    int used = 0;
};

class CardManager
{
public:
    int max_selected_cards = 5;
    int count_selected_card = 0;
    std::deque<std::unique_ptr<Card>> m_cards;
    std::vector<std::unique_ptr<Card>> m_hand_cards;
    std::vector<std::unique_ptr<Card>> m_played_cards;
    std::unordered_map<std::string, HandRank> hand_ranks; // <hand name, hand rank>
    std::string m_hand_name = "High Card";

    CardManager();

    void resetCards();

    void shuffleCards();

    void getNewShowedCards();

    void selectCard(size_t index);

    void deleteSelectedCards();
    void playSelectedCards();

    bool isStraightFlush();
    bool isFourOfAKind();
    bool isFullHouse();
    bool isFlush();
    bool isStraight();
    bool isThreeOfAKind();
    bool isTwoPair();
    bool isPair();
};

// forward declaration
class Game;

struct TarotAction
{
    int multiplier;
    std::string description;
    std::function<void(Game *, int)> action;
};

struct Tarot
{
    std::string name;
    SDL_Texture *atlas;
    SDL_FRect tex_rect;
    TarotAction action;
    bool selected = false;
};

class TarotManager
{
public:
    std::unordered_map<std::string, TarotAction> tarots_actions;
    std::deque<std::unique_ptr<Tarot>> m_tarots;
    std::vector<std::unique_ptr<Tarot>> m_hand_tarots;
    std::vector<std::unique_ptr<Tarot>> m_showed_tarots;
    size_t m_showed_selected_tarots;
    size_t m_selected_tarots;

    TarotManager();

    void shuffleTarots();

    void resetTarots(int round);

    void getNewShowedTarots();

    void selectShowedTarot(size_t index);

    void moveToHand();

    void selectTarot(size_t index);

    void deleteSelectedTarots();

    void applyTarots(Game *game);
};

#endif // SRC_CARD_H
