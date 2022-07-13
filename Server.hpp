#ifndef SERVER_HPP_SENTER
#define SERVER_HPP_SENTER

#include "Durak.hpp"

enum
{
    buffsize = 1024,
    listen_count = 8,
};

class FdHandler
{
    int fd;

public:
    FdHandler(int _fd) : fd(_fd) {}
    virtual ~FdHandler();

    bool WantRead() { return true; }
    bool WandWrite() { return false; }
    int GetFd() { return fd; }

    virtual void Handle(bool r, bool w) = 0;
};

class EventSelector
{
    FdHandler **fd_array;
    int fd_array_len;
    int max_fd;
    bool quit_flag;

public:
    EventSelector()
        : fd_array(0), quit_flag(true) {}
    ~EventSelector();

    void Add(FdHandler *h);
    void Remove(FdHandler *h);

    void End() { quit_flag = false; }
    void Run();
    void BreakLoop() { quit_flag = false; }
};

class Server;

class Session : public FdHandler
{
    friend class Server;
    Player *player;
    Server *the_master;

    char buf[buffsize];
    int buf_used;

    Session(int _fd, Server *_the_master);
    ~Session();

    void Send(const char *msg);
    void GameHandle(const char *msg);

    virtual void Handle(bool r, bool w);
};

class Server : public FdHandler
{
    EventSelector *the_selector;
    struct item
    {
        Session *s;
        item *next;
    };

    Player **players;
    int players_count;

    item *first;
    Game *game;

    Server(int _fd, EventSelector *_the_selector);

public:
    static Server *Start(int port, EventSelector *_the_selector);
    virtual ~Server();

    void RemoveSession(Session *s);
    void StartGame()
    {
        game = new Game(players, players_count);
        game->StartGame();
    }
    void ShowUI();
    void SendAll(const char *msg, Session *except = 0);

    Game *GetGame() { return game; }
    void EndGame() { the_selector->End(); }
    int GetPlayersCount() { return players_count; }

private:
    virtual void Handle(bool r, bool w);
};

#endif