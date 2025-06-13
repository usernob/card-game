#include "card.h"
#include "game.h"
#include "texturemanager.h"
#include "typedef.h"
#include <algorithm>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <utility>
#include <vector>

std::string getCardName(CardRank value)
{
    switch (value)
    {
        case CardRank::TWO:
            return "Two";
        case CardRank::THREE:
            return "Three";
        case CardRank::FOUR:
            return "Four";
        case CardRank::FIVE:
            return "Five";
        case CardRank::SIX:
            return "Six";
        case CardRank::SEVEN:
            return "Seven";
        case CardRank::EIGHT:
            return "Eight";
        case CardRank::NINE:
            return "Nine";
        case CardRank::TEN:
            return "Ten";
        case CardRank::JACK:
            return "Jack";
        case CardRank::QUEEN:
            return "Queen";
        case CardRank::KING:
            return "King";
        case CardRank::ACE:
            return "Ace";
        default:
            return "";
    }
}

std::string getCardSuit(CardSuits suit)
{
    switch (suit)
    {
        case CardSuits::CLUBS:
            return "Clubs";
        case CardSuits::SPADES:
            return "Spades";
        case CardSuits::DIAMONDS:
            return "Diamonds";
        case CardSuits::HEARTS:
            return "Hearts";
        default:
            return "";
    }
}

CardManager::CardManager()
{
    m_cards.resize(52);
    m_hand_cards.reserve(9);
    m_played_cards.reserve(5);
    hand_ranks.reserve(9);

    hand_ranks["Straight Flush"] = {90, 8};
    hand_ranks["Four of a Kind"] = {70, 7};
    hand_ranks["Full House"] = {60, 6};
    hand_ranks["Flush"] = {50, 5};
    hand_ranks["Straight"] = {50, 4};
    hand_ranks["Three of a Kind"] = {30, 3};
    hand_ranks["Two Pair"] = {20, 2};
    hand_ranks["Pair"] = {10, 2};
    hand_ranks["High Card"] = {10, 1};

    resetCards();
}

void CardManager::resetCards()
{
    m_cards.clear();
    auto atlas = TextureManager::instance()->getAtlas("base-card-atlas");

    for (int i = 0; i < 4; i++) // itarete through suits
    {
        for (int j = 2; j < 15; j++) // from 2 to ace
        {
            CardRank rank = static_cast<CardRank>(j);
            CardSuits suit = static_cast<CardSuits>(i);
            auto &card = m_cards.emplace_back(std::make_unique<Card>());
            card->suit = suit;
            card->rank = rank;
            card->atlas = atlas->getAtlas();
            card->name = getCardName(rank) + " of " + getCardSuit(suit);
            card->tex_rect = atlas->getTextureInfo(utils::toSnakeCase(card->name)).rect;
        }
    }

    shuffleCards();
}

void CardManager::shuffleCards()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(m_cards.begin(), m_cards.end(), gen);
}

void CardManager::getNewShowedCards()
{
    int n = m_hand_cards.capacity() - m_hand_cards.size();
    for (int i = 0; i < n; i++)
    {
        if (m_cards.empty()) break;
        m_hand_cards.push_back(std::move(m_cards.back()));
        m_cards.pop_back();
    }
    std::sort(
        m_hand_cards.begin(),
        m_hand_cards.end(),
        [](const std::unique_ptr<Card> &a, const std::unique_ptr<Card> &b) {
            return a->rank > b->rank;
        }
    );
}

void CardManager::selectCard(size_t index)
{
    if (index < 0 || index >= m_hand_cards.size())
    {
        return;
    }

    // toggle
    bool selected = m_hand_cards[index]->selected;
    int current_count = count_selected_card;
    current_count += selected ? -1 : 1;

    if (current_count > max_selected_cards)
    {
        return;
    }

    count_selected_card = current_count;
    m_hand_cards[index]->selected = !selected;

    if (isStraightFlush()) m_hand_name = "Straight Flush";
    else if (isFourOfAKind()) m_hand_name = "Four of a Kind";
    else if (isFullHouse()) m_hand_name = "Full House";
    else if (isFlush()) m_hand_name = "Flush";
    else if (isStraight()) m_hand_name = "Straight";
    else if (isThreeOfAKind()) m_hand_name = "Three of a Kind";
    else if (isTwoPair()) m_hand_name = "Two Pair";
    else if (isPair()) m_hand_name = "Pair";
    else m_hand_name = "High Card";
}

void CardManager::deleteSelectedCards()
{
    std::vector<std::unique_ptr<Card>> temp;
    temp.reserve(m_hand_cards.capacity()); // important to maintain capacity
    for (auto &card : m_hand_cards)
    {
        // skip added selected cards
        if (card->selected)
        {
            continue;
        }
        // only move cards that are not selected
        temp.push_back(std::move(card));
    }

    m_hand_cards.clear();
    m_hand_cards = std::move(temp);
    count_selected_card = 0;
    shuffleCards();
    getNewShowedCards();
}

void CardManager::playSelectedCards()
{
    std::vector<std::unique_ptr<Card>> temp;
    temp.reserve(m_hand_cards.capacity());
    for (auto &card : m_hand_cards)
    {
        if (card->selected)
        {
            m_played_cards.push_back(std::move(card));
        }
        else
        {
            temp.push_back(std::move(card));
        }
    }
    m_hand_cards.clear();
    m_hand_cards = std::move(temp);
    count_selected_card = 0;
}

bool CardManager::isStraightFlush()
{
    return isStraight() && isFlush();
}

bool CardManager::isFourOfAKind()
{
    if (count_selected_card < 4) return false;

    std::map<CardRank, int> ranks;
    for (auto &card : m_hand_cards)
    {
        if (!card->selected) continue;
        ranks[card->rank]++;
    }

    for (auto &rank : ranks)
    {
        if (rank.second >= 4)
        {
            return true;
        }
    }
    return false;
}

bool CardManager::isFullHouse()
{
    if (count_selected_card < 5) return false;

    std::map<CardRank, int> ranks;
    for (auto &card : m_hand_cards)
    {
        if (!card->selected) continue;
        ranks[card->rank]++;
    }

    bool three_of_a_kind = false;
    bool pair = false;
    for (auto &rank : ranks)
    {
        three_of_a_kind |= rank.second == 3;
        pair |= rank.second == 2;
    }

    return three_of_a_kind && pair;
}

bool CardManager::isFlush()
{
    if (count_selected_card < 5) return false;

    std::map<CardSuits, int> suits;
    for (auto &card : m_hand_cards)
    {
        if (!card->selected) continue;
        suits[card->suit]++;
    }

    for (auto &suit : suits)
    {
        if (suit.second >= 5)
        {
            return true;
        }
    }
    return false;
}

bool CardManager::isStraight()
{
    if (count_selected_card < 5) return false;

    int last_rank = 0;
    for (auto &card : m_hand_cards)
    {
        if (!card->selected) continue;

        int rank = static_cast<int>(card->rank);
        if (last_rank == 0)
        {
            last_rank = rank;
            continue;
        }
        if (last_rank - rank != 1) return false;
        last_rank = rank;
    }
    return true;
}

bool CardManager::isThreeOfAKind()
{
    if (count_selected_card < 3) return false;

    std::map<CardRank, int> ranks;
    for (auto &card : m_hand_cards)
    {
        if (!card->selected) continue;
        ranks[card->rank]++;
    }

    for (auto &rank : ranks)
    {
        if (rank.second >= 3)
        {
            return true;
        }
    }
    return false;
}

bool CardManager::isTwoPair()
{
    if (count_selected_card < 4) return false;

    std::map<CardRank, int> ranks;
    for (auto &card : m_hand_cards)
    {
        if (!card->selected) continue;
        ranks[card->rank]++;
    }

    int pair_count = 0;
    for (auto &rank : ranks)
    {
        if (rank.second >= 2)
        {
            pair_count++;
        }
    }
    return pair_count == 2;
}

bool CardManager::isPair()
{
    if (count_selected_card < 2) return false;

    std::map<CardRank, int> ranks;
    for (auto &card : m_hand_cards)
    {
        if (!card->selected) continue;
        ranks[card->rank]++;
    }

    for (auto &rank : ranks)
    {
        if (rank.second >= 2)
        {
            return true;
        }
    }
    return false;
}

TarotManager::TarotManager()
{
    tarots_actions.reserve(22);

    tarots_actions["chariot"] = {0, "", [](Game *game, int multiplier) {}};
    tarots_actions["death"] = {0, "", nullptr};
    tarots_actions["emperor"] = {0, "", nullptr};
    tarots_actions["empress"] = {0, "", nullptr};
    tarots_actions["hanged_man"] = {0, "", nullptr};
    tarots_actions["hierophant"] = {0, "", nullptr};
    tarots_actions["judgement"] = {0, "", nullptr};
    tarots_actions["justice"] = {0, "", nullptr};
    tarots_actions["magician"] = {0, "", nullptr};
    tarots_actions["priestess"] = {0, "", nullptr};
    tarots_actions["strength"] = {0, "", nullptr};
    tarots_actions["temperance"] = {0, "", nullptr};
    tarots_actions["the_devil"] = {0, "", nullptr};
    tarots_actions["the_fool"] = {0, "", nullptr};
    tarots_actions["the_hermit"] = {0, "", nullptr};
    tarots_actions["the_lovers"] = {0, "", nullptr};
    tarots_actions["the_moon"] = {0, "", nullptr};
    tarots_actions["the_star"] = {0, "", nullptr};
    tarots_actions["the_sun"] = {0, "", nullptr};
    tarots_actions["the_tower"] = {0, "", nullptr};
    tarots_actions["the_world"] = {0, "", nullptr};
    tarots_actions["wheel_of_fortune"] = {0, "", nullptr};

    m_tarots.resize(22);
    m_showed_tarots.resize(3);
    m_hand_tarots.resize(7);
    resetTarots(1);
}

void TarotManager::shuffleTarots()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(m_tarots.begin(), m_tarots.end(), gen);
}

void TarotManager::resetTarots(int round)
{
    auto atlas = TextureManager::instance()->getAtlas("tarot-card-atlas");
    for (auto key : tarots_actions)
    {
        auto &tarot = m_tarots.emplace_back(std::make_unique<Tarot>());
        tarot->name = utils::toTitleCase(key.first);
        tarot->atlas = atlas->getAtlas();
        tarot->tex_rect = atlas->getTextureInfo(key.first).rect;
        tarot->action = key.second;
        tarot->action.multiplier = round;
    }
}

void TarotManager::getNewShowedTarots()
{
    m_showed_tarots.clear();
    shuffleTarots();
    for (int i = 0; i < 3; i++)
    {
        if (m_tarots.empty()) {
            break; 
        }
        m_showed_tarots.push_back(std::move(m_tarots.back()));
        m_tarots.pop_back();
    }
}

void TarotManager::selectShowedTarot(size_t index)
{
    // -1 for no selected
    if (index < -1 || index >= m_showed_tarots.size())
    {
        return;
    }
    m_showed_selected_tarots = index;
}

void TarotManager::moveToHand()
{
    if (m_showed_selected_tarots == -1)
    {
        return;
    }
    m_hand_tarots.push_back(std::move(m_showed_tarots[m_showed_selected_tarots]));
    m_showed_tarots.erase(m_showed_tarots.begin() + m_showed_selected_tarots);
    m_showed_selected_tarots = -1;
}

void TarotManager::selectTarot(size_t index)
{
    if (index < -1 || index >= m_hand_tarots.size())
    {
        return;
    }
    m_selected_tarots = index;
}

void TarotManager::deleteSelectedTarots()
{
    if (m_selected_tarots == -1)
    {
        return;
    }
    m_hand_tarots.erase(m_hand_tarots.begin() + m_selected_tarots);
    m_selected_tarots = -1;
}

void TarotManager::applyTarots(Game *game)
{
    for (auto &tarot : m_hand_tarots)
    {
        if (tarot->action.action)
        {
            tarot->action.action(game, tarot->action.multiplier);
        }
    }
}
