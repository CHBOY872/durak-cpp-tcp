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
#include <fcntl.h>
#include "Server.hpp"
#include "Durak.hpp"

static char controls[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
static char error_connect_msg[] =
    "The game was started or count of players are full\n";
static char greetings_msg[] =
    "Welcome to the game. To start game press Enter\n";
static char joined_to_game_msg[] =
    "New player joined to the game\n";
static char left_the_game_msg[] = "Some player left the game\n";
static char players_count_msg[] = "All players: ";

FdHandler::~FdHandler()
{
    close(fd);
}

EventSelector::~EventSelector()
{
    if (fd_array)
        delete[] fd_array;
}

void EventSelector::Add(FdHandler *h)
{
    int fd = h->GetFd();
    int i;
    if (!fd_array)
    {
        fd_array_len =
            fd > listen_count - 1 ? fd + 1 : listen_count;
        fd_array = new FdHandler *[fd_array_len];
        for (i = 0; i < fd_array_len; i++)
            fd_array[i] = 0;
        max_fd = -1;
    }
    if (fd_array_len <= fd)
    {
        FdHandler **tmp = new FdHandler *[fd + 1];
        for (i = 0; i <= fd; i++)
            tmp[i] = i < fd_array_len ? fd_array[i] : 0;
        fd_array_len = fd + 1;
        delete[] fd_array;
        fd_array = tmp;
    }
    if (fd > max_fd)
        max_fd = fd;
    fd_array[fd] = h;
}

void EventSelector::Remove(FdHandler *h)
{
    int fd = h->GetFd();
    if (fd >= fd_array_len || h != fd_array[fd])
        return;
    fd_array[fd] = 0;
    if (fd == max_fd)
    {
        while (max_fd >= 0 && !fd_array[max_fd])
            max_fd--;
    }
}

void EventSelector::Run()
{
    do
    {
        fd_set rds, wrs;
        FD_ZERO(&rds);
        FD_ZERO(&wrs);
        int i;
        for (i = 0; i <= max_fd; i++)
        {
            if (fd_array[i])
            {
                if (fd_array[i]->WandWrite())
                    FD_SET(i, &wrs);
                if (fd_array[i]->WantRead())
                    FD_SET(i, &rds);
            }
        }
        int stat = select(max_fd + 1, &rds, &wrs, 0, 0);
        if (stat <= 0)
        {
            quit_flag = false;
            return;
        }
        for (i = 0; i <= max_fd; i++)
        {
            if (fd_array[i])
            {
                bool r = FD_ISSET(i, &rds);
                bool w = FD_ISSET(i, &wrs);
                if (r || w)
                    fd_array[i]->Handle(r, w);
            }
        }
    } while (quit_flag);
}

////////////////////

Server::Server(int _fd, EventSelector *_the_selector)
    : FdHandler(_fd), the_selector(_the_selector),
      players(0), players_count(0), first(0), game(0)
{
    the_selector->Add(this);
}

Server::~Server()
{
    while (first)
    {
        item *tmp = first;
        first = first->next;
        the_selector->Remove(tmp->s);
        delete tmp->s;
        delete tmp;
    }
    if (the_selector)
        delete the_selector;
    if (game)
        delete game;
    if (players)
        delete[] players;
}

Server *Server::Start(int port, EventSelector *_the_selector)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd)
        return 0;

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
#ifdef DEBUG
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
#else
    addr.sin_addr.s_addr = htons(INADDR_ANY);
#endif

    if (-1 == bind(fd, (struct sockaddr *)&addr, sizeof(addr)))
        return 0;

    if (-1 == listen(fd, listen_count))
        return 0;
    return new Server(fd, _the_selector);
}

void Server::Handle(bool r, bool w)
{
    if (!r)
        return;
    struct sockaddr_in addr;
    socklen_t len = 0;
    int socket_client = accept(GetFd(), (struct sockaddr *)&addr, &len);

    if (-1 == socket_client)
        return;

    if ((players_count == listen_count) || game)
    {
        write(socket_client, error_connect_msg, sizeof(error_connect_msg));
        close(socket_client);
        return;
    }

    int flags = fcntl(socket_client, F_GETFL);
    fcntl(socket_client, F_SETFL, flags | O_NONBLOCK);

    item *p = new item;
    p->next = first;
    p->s = new Session(socket_client, this);
    if (!players)
    {
        players_count++;
        players = new Player *[players_count];
        players[0] = p->s->player;
    }
    else
    {
        int new_players_count = players_count + 1;
        Player **tmp = new Player *[new_players_count];
        int i;
        for (i = 0; i < players_count; i++)
            tmp[i] = players[i];
        tmp[i] = p->s->player;
        delete[] players;
        players = tmp;
        players_count = new_players_count;
    }
    char *msg =
        new char[sizeof(joined_to_game_msg) + sizeof(players_count_msg) + 4];
    sprintf(msg, "%s%s%d\n",
            joined_to_game_msg, players_count_msg, players_count);
    SendAll(msg, p->s);
    delete[] msg;
    first = p;

    the_selector->Add(p->s);
}

void Server::SendAll(const char *msg, Session *except)
{
    item *tmp = first;
    while (tmp)
    {
        if (except != tmp->s)
            tmp->s->Send(msg);

        tmp = tmp->next;
    }
}

void Server::RemoveSession(Session *s)
{
    the_selector->Remove(s);
    item **p;
    for (p = &first; *p; p = &((*p)->next))
    {
        if ((*p)->s == s)
        {
            item *tmp = *p;
            *p = tmp->next;
            delete tmp->s;
            delete tmp;
            players_count--;
            return;
        }
    }
}

void Server::ShowUI()
{
    item *tmp = first;
    while (tmp)
    {
        if (tmp->s->player)
        {
            char *show_count_cards_players =
                new char[(6 + 3) * (players_count - 1) + 2];
            char *show_who_def_throw =
                new char[(6 + 3) * (players_count - 1) + 2];
            char *ptr_show_count_cards = show_count_cards_players;
            char *ptr_show_who_def = show_who_def_throw;
            item *t_tmp = first;
            while (t_tmp)
            {

                if (t_tmp->s->player != tmp->s->player)
                {
                    if (t_tmp->s->player->GetStep() != Player::is_won)
                        sprintf(ptr_show_count_cards, "< %2d >   ",
                                t_tmp->s->player->GetDeck().GetCardsCount());
                    else
                        sprintf(ptr_show_count_cards, "< 0  >   ");

                    ptr_show_count_cards += 9;
                    switch (t_tmp->s->player->GetStep())
                    {
                    case Player::is_def:
                        sprintf(ptr_show_who_def, "def      ");
                        break;
                    case Player::is_throw:
                        sprintf(ptr_show_who_def, "throw    ");
                        break;
                    case Player::take:
                        sprintf(ptr_show_who_def, "take     ");
                        break;
                    case Player::beat:
                        sprintf(ptr_show_who_def, "beat     ");
                        break;
                    case Player::is_throw_up:
                        sprintf(ptr_show_who_def, "throup   ");
                        break;
                    case Player::is_won:
                        sprintf(ptr_show_who_def, "won      ");
                        break;
                    case Player::is_defeat:
                        sprintf(ptr_show_who_def, "defeat   ");
                        break;
                    default:
                        sprintf(ptr_show_who_def, "         ");
                        break;
                    }
                    ptr_show_who_def += 9;
                }
                t_tmp = t_tmp->next;
            }
            sprintf(ptr_show_count_cards, "\n");
            sprintf(ptr_show_who_def, "\n");
            tmp->s->Send("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            *(ptr_show_count_cards + 1) = 0;
            *(ptr_show_who_def + 1) = 0;
            tmp->s->Send(show_count_cards_players);
            tmp->s->Send(show_who_def_throw);
            tmp->s->Send("\n\n\n\n");

            char *beat_line = new char[11];

            if (game->process)
            {
                CardDeck::item *process_throw =
                    game->process->throw_deck->first;
                CardDeck::item *process_def =
                    game->process->beat_deck->first;
                int i;
                for (i = 0;
                     i < game->process->throw_deck->cards_count -
                             game->process->beat_deck->cards_count;
                     i++)
                {
                    sprintf(beat_line, "%c%-2s /    \n",
                            process_throw->card->suit,
                            process_throw->card->card);

                    tmp->s->Send(beat_line);
                    process_throw = process_throw->next;
                }

                while (process_throw)
                {
                    sprintf(beat_line, "%c%-2s / %c%-2s\n",
                            process_throw->card->suit,
                            process_throw->card->card,
                            process_def->card->suit,
                            process_def->card->card);
                    process_def = process_def->next;

                    tmp->s->Send(beat_line);
                    process_throw = process_throw->next;
                }
            }

            tmp->s->Send("\n\n\n\n");
            char *cards_count = new char[65];
            sprintf(cards_count,
                    "Your cards:                                      < %c%-2s > [ %2d ]\n",
                    game->trump_card->suit, game->trump_card->card,
                    game->deck->cards_count);
            tmp->s->Send(cards_count);
            if (tmp->s->player->GetStep() != Player::is_won)
            {
                int players_cards_count =
                    tmp->s->player->GetDeck().cards_count;
                char *s = new char[11 * players_cards_count + 2];
                char *s_ptr = s;
                CardDeck::item *player_card_itm =
                    tmp->s->player->GetDeck().first;
                while (player_card_itm)
                {
                    sprintf(s_ptr, "[%c: %c%-2s]   ",
                            controls[player_card_itm->idx],
                            player_card_itm->card->suit,
                            player_card_itm->card->card);
                    s_ptr += 11;
                    player_card_itm = player_card_itm->next;
                }
                *(s_ptr) = '\n';
                *(s_ptr + 1) = 0;
                tmp->s->Send(s);
                delete[] s;
            }

            switch (tmp->s->player->GetStep())
            {
            case Player::is_def:
                tmp->s->Send("def>");
                break;
            case Player::is_throw:
                tmp->s->Send("throw>");
                break;
            case Player::is_throw_up:
                tmp->s->Send("throw up>");
                break;
            case Player::take:
                tmp->s->Send("take>");
                break;
            case Player::beat:
                tmp->s->Send("beat>");
                break;
            case Player::is_won:
                tmp->s->Send("WON!");
                break;
            case Player::is_defeat:
                tmp->s->Send("Defeat");
            default:
                break;
            }

            delete[] show_count_cards_players;
            delete[] show_who_def_throw;
            delete[] beat_line;
            delete[] cards_count;
        }

        tmp = tmp->next;
    }
}

////////////////////

Session::Session(int _fd, Server *_the_master)
    : FdHandler(_fd), player(new Player()),
      the_master(_the_master), buf_used(0)
{
    Send(greetings_msg);
}

Session::~Session()
{
    if (player)
        delete player;
}

void Session::Send(const char *msg)
{
    write(GetFd(), msg, strlen(msg));
}

void Session::GameHandle(const char *msg)
{
    if (!the_master->GetGame())
    {
        if (msg[0] == '\n' && the_master->GetPlayersCount() > 1)
        {
            the_master->StartGame();
            the_master->ShowUI();
        }
        else
            Send("Cannot start a game with only-one player!\n");
        return;
    }
    int idx = CardDeck::GetIdx(msg[0]);
    switch (the_master->GetGame()->GetStat())
    {
    case Game::game_process_stat:
        switch (player->GetStep())
        {
        case Player::is_throw:

            player->ThrowCard(idx);
            break;

        case Player::is_throw_up:

            if (msg[0] == '\n')
                player->Beat();

            player->ThrowUp(idx);
            break;

        case Player::is_def:

            if (msg[0] == '\n')
                player->Take();

            player->Def(idx);
            break;

        case Player::take:

            player->Take();
            break;

        case Player::beat:

            player->Beat();
            break;

        default:
            break;
        }
        the_master->ShowUI();

        if (the_master->GetGame()->GetStat() == Game::end_game_stat)
            the_master->EndGame();

        break;
    case Game::end_game_stat:
        the_master->EndGame();
        break;
    default:
        break;
    }
}

void Session::Handle(bool r, bool w)
{
    int fd = GetFd();
    int rc = read(fd, buf, buffsize - buf_used);
    int i;

    if (rc <= 0 || buf_used + rc >= buffsize)
    {
        the_master->SendAll(left_the_game_msg, this);
        the_master->RemoveSession(this);
        return;
    }

    char *msg = new char[rc + 1];
    int k = 0;
    for (i = 0; i < rc; i++)
    {
        if (buf[i + buf_used] == '\r')
            continue;

        msg[k] = buf[i + buf_used];
        k++;
    }

    msg[k] = 0;

    GameHandle(msg);

    delete[] msg;
    bzero(msg, buf_used);
    buf_used = 0;
}