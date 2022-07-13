#ifndef DURAK_HPP_SENTER
#define DURAK_HPP_SENTER

#include <stdio.h>

enum
{
    full_cards = 6,
};

struct Card
{
    char suit;
    char card[3];

    Card() {}
    Card(const char *_card, char _suit);
    Card(const Card &);
    const Card &operator=(const Card &a);

    friend bool operator==(const Card &, const Card &);
    friend bool operator<(const Card &, const Card &);
    friend bool operator>(const Card &, const Card &);
    friend bool operator!=(const Card &, const Card &); // return true if suits
                                                        // are not equal, and
private:                                                // cards are equal
    int GetCardIdx() const;
};

class Server;

class CardDeck
{
    struct item
    {
        int idx;
        Card *card;
        item *next;
    };
    int last_index;
    int cards_count;
    item *first;
    CardDeck(Card *arr, int len);

public:
    friend class Game;
    friend class Server;
    CardDeck();
    static CardDeck *MakeDeck();
    ~CardDeck();

    int GetCardsCount() const { return cards_count; }
    void GetLast(Card **crd) const;
    void GetFirst(Card **crd) const;
    Card **GetCardByIdx(int idx);

    item *operator->() { return first; }

    void ShowDeck() const
    {
        item *tmp = first;
        printf("\n\n\n\n\n\n\n");
        while (tmp)
        {
            printf("%c%s %d\n", tmp->card->suit, tmp->card->card, tmp->idx);
            tmp = tmp->next;
        }
        printf("\n\n\n\n\n\n\n");
    }

    void Push(Card *card);
    void Pop();
    void RemoveCard(Card *card);

    static int GetIdx(const char letter);
};

class Game;

class Player
{
public:
    friend class Game;
    enum player_step
    {
        nothing,
        is_def,
        is_throw,
        is_throw_up,
        beat,
        take,
        is_won,
        is_defeat
    };

private:
    CardDeck *deck;
    player_step step;
    Game *master;

public:
    Player();
    ~Player();

    CardDeck &GetDeck() const { return *deck; }
    player_step GetStep() const { return step; }
    void SetStep(player_step _step) { step = _step; }

    bool ThrowCard(int idx);
    bool ThrowUp(int idx);
    bool Def(int idx);
    void Beat();
    void Take();
};

class Game
{
public:
    enum game_stat
    {
        not_run_stat,
        game_process_stat,
        end_game_stat,
    };

private:
    friend class Server;

    struct player_item
    {
        Player *player;
        player_item *next;
    };
    player_item *first;

    int player_count;
    int won_count;
    CardDeck *deck;
    Card *trump_card;

    struct game_process
    {
        CardDeck *throw_deck;
        CardDeck *beat_deck;

        int beat_count;
        int def_cards_count;
        game_process();
        ~game_process();
    };
    game_process *process;

    game_stat stat;

public:
    Game(Player **player_arr, int len);
    ~Game();

    void StartGame();

    void GetCards();
    Card GetTrumpCard() const { return Card(*trump_card); }
    int GetPlayersCount() const { return player_count; }
    CardDeck &GetDeck() const { return *deck; }
    game_stat GetStat() const { return stat; }

    bool MakeMove(Player *who_throw, Card **card);
    bool ThrowUp(Player *who_throw_up, Card **card);
    bool Def(Player *who_def, Card **card);
    void SetBeat(Player *player);
    void CheckIfEndOfMove();
    void RemoveWinner(Player *who);

private:
    void PushPlayer(Player *player);
    void FindPlayerWithLowCard();
    bool IsExistCardNumber(Card *card);
    bool IsTrumpCard(Card *card);
    void DropCardsInBeat();

    bool IsWinner(Player *who);
    void TryRemoveWinner(Player *who);
};

/*
    Don't delete Cards in CardDeck... Just reown adresess
    to these cards
*/
#endif