#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include "Durak.hpp"

static char controls[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

////////////////////

void str_copy(char *dest, const char *from)
{
    int i;
    for (i = 0; *(from + i); i++)
        dest[i] = from[i];

    dest[i] = 0;
}

////////////////////

#define CARDS_WITHOUT_SUIT_COUNT 13
#define MAX_CARDS 52

static char cards_without_suits[CARDS_WITHOUT_SUIT_COUNT][3] =
    {"2", "3", "4", "5", "6", "7",
     "8", "9", "10", "J", "Q", "K", "A"};
static char suits[4] = {'^', '?', '#', '&'};

static Card card_deck_arr[MAX_CARDS];

////////////////////

Card::Card(const char *_card, char _suit) : suit(_suit)
{
    str_copy(card, _card);
}

const Card &Card::operator=(const Card &a)
{
    suit = a.suit;
    str_copy(card, a.card);
    return *this;
}

Card::Card(const Card &a)
{
    suit = a.suit;
    str_copy(card, a.card);
}

bool operator==(const Card &a, const Card &b)
{
    return !strcmp(a.card, b.card) && (a.suit == b.suit);
}

int Card::GetCardIdx() const
{
    int i;
    for (i = 0; i < CARDS_WITHOUT_SUIT_COUNT; i++)
    {
        if (!strcmp(card, cards_without_suits[i]))
            break;
    }
    return i;
}

bool operator<(const Card &a, const Card &b)
{
    return (a.suit == b.suit) && (a.GetCardIdx() < b.GetCardIdx());
}

bool operator>(const Card &a, const Card &b)
{
    return (a.suit == b.suit) && (a.GetCardIdx() > b.GetCardIdx());
}

bool operator!=(const Card &a, const Card &b)
{
    return (a.suit != b.suit) && !strcmp(a.card, b.card);
}

////////////////////

CardDeck::CardDeck() : last_index(0), cards_count(0), first(0)
{
}

CardDeck::CardDeck(Card *arr, int len)
    : last_index(0), cards_count(0), first(0)
{
    int i;
    for (i = 0; i < len; i++)
        Push(&arr[i]);
}

CardDeck::~CardDeck()
{
    while (first)
    {
        item *p = first;
        first = first->next;
        delete p;
    }
}

CardDeck *CardDeck::MakeDeck()
{
    srand(time(0));
    int i;
    int k = 0;
    for (i = 0; i < CARDS_WITHOUT_SUIT_COUNT; i++)
    {
        int j;
        for (j = 0; j < 4; j++)
        {
            card_deck_arr[k] = Card(cards_without_suits[i], suits[j]);
            k++;
        }
    }
    struct Card tmp;
    for (i = MAX_CARDS - 1; i >= 0; i--)
    {
        int idx = (int)((double)i * rand() / (RAND_MAX + 1.0));
        if (idx == i && i != 0)
            continue;
        tmp = card_deck_arr[i];
        card_deck_arr[i] = card_deck_arr[idx];
        card_deck_arr[idx] = tmp;
    }

    return new CardDeck(card_deck_arr, MAX_CARDS);
}

void CardDeck::Push(Card *card)
{
    item *tmp = new item;
    tmp->card = card;
    tmp->idx = last_index++;
    cards_count++;
    tmp->next = first;
    first = tmp;
}

void CardDeck::Pop()
{
    if (first)
    {
        item *tmp = first;
        first = first->next;
        delete tmp;
        last_index--;
        cards_count--;
    }
}

void CardDeck::RemoveCard(Card *card)
{
    item **tmp;
    for (tmp = &first; *tmp; tmp = &((*tmp)->next))
    {
        if (*(*tmp)->card == *card)
        {
            item *p = *tmp;
            *tmp = p->next;
            delete p;
            cards_count--;
            return;
        }
    }
}

void CardDeck::GetLast(Card **crd) const
{
    if (first)
        *crd = first->card;
}

void CardDeck::GetFirst(Card **crd) const
{
    item *tmp = first;
    while (tmp->next)
        tmp = tmp->next;
    *crd = tmp->card;
}

Card **CardDeck::GetCardByIdx(int idx)
{
    item *tmp = first;
    while (tmp)
    {
        if (tmp->idx == idx)
            return &(tmp->card);

        tmp = tmp->next;
    }
    return 0;
}

int CardDeck::GetIdx(const char letter)
{
    int idx;
    for (idx = 0; *(controls + idx); idx++)
    {
        if (controls[idx] == letter)
            return idx;
    }
    return -1;
}

////////////////////

Player::Player() : deck(new CardDeck()), step(nothing), master(0) {}

Player::~Player()
{
    delete deck;
}

bool Player::ThrowCard(int idx)
{
    Card **card = deck->GetCardByIdx(idx);
    if (!card)
        return false;

    return master->MakeMove(this, card);
}

bool Player::ThrowUp(int idx)
{
    Card **card = deck->GetCardByIdx(idx);
    if (!card)
        return false;

    return master->ThrowUp(this, card);
}

bool Player::Def(int idx)
{
    Card **card = deck->GetCardByIdx(idx);
    if (!card)
        return false;

    return master->Def(this, card);
}

void Player::Beat()
{
    switch (step)
    {
    case is_throw_up:
        step = beat;
        master->SetBeat(this);
        master->CheckIfEndOfMove();
        break;
    case beat:
        step = is_throw_up;
        master->SetBeat(this);
        break;
    default:
        break;
    }
}

void Player::Take()
{
    switch (step)
    {
    case take:
        step = is_def;
        master->CheckIfEndOfMove();
        break;
    case is_def:
        step = take;
        master->CheckIfEndOfMove();
        break;
    default:
        break;
    }
}

////////////////////

Game::game_process::game_process()
    : beat_count(0), def_cards_count(0)
{
    throw_deck = new CardDeck;
    beat_deck = new CardDeck;
}

Game::game_process::~game_process()
{
    delete throw_deck;
    delete beat_deck;
}

bool Game::IsWinner(Player *who)
{
    return !deck->cards_count && !who->deck->cards_count;
}

void Game::TryRemoveWinner(Player *who)
{
    if (IsWinner(who))
    {
        who->SetStep(Player::is_won);
        won_count++;
        player_count--;
        RemoveWinner(who);
    }
    if (player_count == 1)
    {
        stat = end_game_stat;
        player_item *tmp = first;
        while (tmp)
        {
            switch (tmp->player->step)
            {
            case Player::is_won:
                break;

            default:
                tmp->player->SetStep(Player::is_defeat);
                break;
            }
            tmp = tmp->next;
        }
    };
}

void Game::PushPlayer(Player *player)
{
    player_item *tmp = new player_item;
    tmp->player = player;
    tmp->player->master = this;
    tmp->next = first;
    first = tmp;
    player_count++;
}

void Game::GetCards()
{
    if (deck)
    {
        player_item *tmp = first;
        while (tmp)
        {
            int count = tmp->player->GetDeck().GetCardsCount();
            int i;
            for (i = 0; i < full_cards - count; i++)
            {
                Card *crd = 0;
                if (!deck->first)
                {
                    TryRemoveWinner(tmp->player);
                    break;
                }
                deck->GetLast(&crd);
                deck->Pop();
                tmp->player->GetDeck().Push(crd);
            }

            tmp = tmp->next;
        }
    }
}

Game::Game(Player **player_arr, int len) : won_count(0)
{
    first = 0;
    player_count = 0;
    int i;
    for (i = 0; i < len; i++)
        PushPlayer(player_arr[i]);

    deck = CardDeck::MakeDeck();
    trump_card = 0;
    deck->GetFirst(&trump_card);

    process = 0;
    stat = not_run_stat;
}

Game::~Game()
{
    while (first)
    {
        player_item *tmp = first;
        first = first->next;
        delete tmp;
    }

    if (deck)
        delete deck;
    if (process)
        delete process;
}

void Game::FindPlayerWithLowCard()
{
    player_item *tmp_player = first;
    Card *low_crd = trump_card;
    Player *player_with_low_card = 0;

    while (tmp_player)
    {
        CardDeck::item *tmp_deck = tmp_player->player->GetDeck().first;
        while (tmp_deck)
        {
            if (*(tmp_deck->card) < *low_crd)
            {
                low_crd = tmp_deck->card;
                player_with_low_card = tmp_player->player;
            }
            tmp_deck = tmp_deck->next;
        }

        tmp_player = tmp_player->next;
    }

    if (player_with_low_card)
        player_with_low_card->SetStep(Player::is_throw);
    else
        first->player->SetStep(Player::is_throw);
}

void Game::StartGame()
{
    GetCards();
    stat = game_process_stat;
    FindPlayerWithLowCard();
}

bool Game::MakeMove(Player *who_throw, Card **card)
{
    if (!card)
        return false;

    process = new game_process;

    player_item *tmp = first;
    while (tmp)
    {
        if (tmp->player->step != Player::is_def)
            tmp->player->step = Player::is_throw_up;
        if (tmp->player == who_throw)
        {
            if (tmp->next)
            {
                tmp->next->player->step = Player::is_def;
                process->def_cards_count =
                    tmp->next->player->GetDeck().cards_count;
            }
            else
            {
                first->player->step = Player::is_def;
                process->def_cards_count =
                    first->player->GetDeck().cards_count;
            }
        }

        tmp = tmp->next;
    }

    process->throw_deck->Push(*card);
    who_throw->deck->RemoveCard(*card);
    TryRemoveWinner(who_throw);

    return true;
}

bool Game::IsExistCardNumber(Card *card)
{
    CardDeck::item *tmp = process->beat_deck->first;
    while (tmp)
    {
        if (*(tmp->card) != *card)
            return true;
        tmp = tmp->next;
    }
    tmp = process->throw_deck->first;
    while (tmp)
    {
        if (*(tmp->card) != *card)
            return true;
        tmp = tmp->next;
    }
    return false;
}

bool Game::IsTrumpCard(Card *card)
{
    if (card->suit == trump_card->suit)
        return true;
    return false;
}

bool Game::ThrowUp(Player *who_throw_up, Card **card)
{
    if (process->def_cards_count <= process->throw_deck->cards_count ||
        process->throw_deck->cards_count == full_cards)
        return false;

    if (!IsExistCardNumber(*card))
        return false;

    process->throw_deck->Push(*card);
    who_throw_up->deck->RemoveCard(*card);
    TryRemoveWinner(who_throw_up);

    return true;
}

bool Game::Def(Player *who_def, Card **card)
{
    if (!(*card))
        return false;
    if (!process->throw_deck->first)
        return false;
    if (process->beat_deck->cards_count == process->throw_deck->cards_count)
        return false;

    int i;
    int iterations = process->throw_deck->cards_count -
                     (process->beat_deck->cards_count + 1);
    CardDeck::item *tmp = process->throw_deck->first;
    for (i = 0; i < iterations; i++)
        tmp = tmp->next;

    if (IsTrumpCard(*card))
    {
        if (IsTrumpCard(tmp->card))
        {
            if (**card > *tmp->card)
            {
                process->beat_deck->Push(*card);
                who_def->deck->RemoveCard(*card);

                TryRemoveWinner(who_def);

                return true;
            }
            return false;
        }
        else
        {
            process->beat_deck->Push(*card);
            who_def->deck->RemoveCard(*card);

            TryRemoveWinner(who_def);

            return true;
        }
    }
    else
    {
        if (**card > *tmp->card)
        {
            process->beat_deck->Push(*card);
            who_def->deck->RemoveCard(*card);

            TryRemoveWinner(who_def);
            return true;
        }
        return false;
    }
}

void Game::SetBeat(Player *player)
{
    switch (player->step)
    {
    case Player::beat:
        process->beat_count++;
        break;
    case Player::is_throw_up:
        process->beat_count--;
        break;
    default:
        break;
    }
}

void Game::DropCardsInBeat()
{
    CardDeck *tmp_deck = 0;

    tmp_deck = process->beat_deck;
    while (tmp_deck->first)
        tmp_deck->Pop();
    tmp_deck = process->throw_deck;
    while (tmp_deck->first)
        tmp_deck->Pop();
}

void Game::CheckIfEndOfMove()
{
    int beat_count = process->beat_count;
    if (beat_count == player_count - 1)
    {
        player_item *tmp = first;
        CardDeck *tmp_deck = 0;
        Card *crd = 0;
        bool new_game_flag = false;
        bool is_take_or_def = false;

        while (tmp)
        {
            if (tmp->player->step == Player::take ||
                tmp->player->step == Player::is_def)
            {
                is_take_or_def = true;
                break;
            }

            tmp = tmp->next;
        }

        if (is_take_or_def)
        {
            switch (tmp->player->step)
            {
            case Player::take:
                tmp_deck = process->beat_deck;
                while (tmp_deck->first)
                {
                    tmp_deck->GetFirst(&crd);
                    tmp_deck->RemoveCard(crd);
                    tmp->player->GetDeck().Push(crd);
                }
                tmp_deck = process->throw_deck;
                while (tmp_deck->first)
                {
                    tmp_deck->GetFirst(&crd);
                    tmp_deck->RemoveCard(crd);
                    tmp->player->GetDeck().Push(crd);
                }
                if (tmp->next)
                    tmp->next->player->step = Player::is_throw;
                else
                    first->player->step = Player::is_throw;

                new_game_flag = true;
                break;
            case Player::is_def:
                if (process->beat_deck->cards_count ==
                    process->throw_deck->cards_count)
                {
                    DropCardsInBeat();
                    tmp->player->step = Player::is_throw;
                    new_game_flag = true;
                }
                break;
            default:
                break;
            }
        }
        else
        {
            DropCardsInBeat();
            new_game_flag = true;
        }

        if (new_game_flag)
        {
            tmp = first;
            if (is_take_or_def)
            {
                while (tmp)
                {
                    if (tmp->player->step == Player::is_throw)
                    {
                        tmp = tmp->next;
                        continue;
                    }
                    tmp->player->step = Player::nothing;

                    tmp = tmp->next;
                }
            }
            else
            {
                switch (tmp->player->step)
                {
                case Player::is_defeat:
                    break;

                default:
                    tmp->player->step = Player::is_throw;
                    tmp = tmp->next;
                    while (tmp)
                    {
                        tmp->player->SetStep(Player::nothing);
                        tmp = tmp->next;
                    }
                    break;
                }
            }

            delete process;
            process = 0;
            GetCards();
        }
    }
}

void Game::RemoveWinner(Player *who)
{
    player_item **tmp = 0;
    for (tmp = &first; *tmp; tmp = &((*tmp)->next))
    {
        if ((*tmp)->player == who)
        {
            player_item *p = *tmp;
            *tmp = p->next;
            delete p;
            return;
        }
    }
}
