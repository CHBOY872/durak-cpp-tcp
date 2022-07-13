#include <stdio.h>
#include "Server.hpp"
#include <string.h>

static int port = 7708;

int main()
{
    EventSelector *selector = new EventSelector();
    Server *serv = Server::Start(port, selector);
    if (!serv)
    {
        perror("server");
        return 1;
    }
    selector->Run();
    delete serv;
    return 0;
}