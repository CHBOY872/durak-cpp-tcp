# durak-cpp-tcp
Durak is a popular card game in Easter Europe. This game in my project was implemented like a game in Terminal. You can connect to this game via Telnet and play with your friends.
## How to build
To build a game server type in Terminal
```
make main
```
## How to run
To run a game server type in Terminal after building 
```
./main
```
## How to connect to the game server
To connect to the game server via Telnet type in Terminal
```
telnet <server ip> <server port>
```
- server ip - IP in which server was ran. Type the ip like `100.100.100.100` or like a domain (excample: `domain.com`) if your ip and all server in which game server was run related with some domain
- server port - port in which your game server was run. 
> Note: The standard port is 7708. If you have to change it, change in `main.cpp` file.
## How to play
After connecting to the server, you receive that message:
```
Trying 127.0.0.1...
Connected to 127.0.0.1.
Escape character is '^]'.
Welcome to the game. To start game press Enter
```
If you want to start game press `Enter`
> Note: you cannot start a game if you are only player in the server. After trying to connect you get a message `Cannot start a game with only-one player!`. Wait for other players!
If other player connect to server, you receive the message:
```
New player joined to the game
All players: 2
```
> Note: you cannot connect to the game if it had already run or the count of players are 8. After trying to connect you get a message `The game was started or count of players are full`
The game starts after pressing the `Enter`
You will have in Terminal like that:
```
<  6 >   
         
Your cards:                                      < &9  > [ 40 ]
[f: ^2 ]   [e: &Q ]   [d: #6 ]   [c: ^3 ]   [b: ?K ]   [a: &5 ]   
throw>
```
- `<  6 >` is another player, 6 is count of his cards in his hand.
- `< &9  >` is a trump card.
- `[ 40 ]` is a number of cards in main deck.
- `[f: ^2 ]   [e: &Q ]   [d: #6 ]   [c: ^3 ]   [b: ?K ]   [a: &5 ]` are your cards.
- `throw>` is what should you do.
> Note: in place where `throw>` was written it may be write also `def> , throw up> , beat> , take> , WON! , defeat`. It depends on your state in a game.
If you want to throw a card, throw up a card or defence, type a button that matches your card (if you want to throw a `^2`, press `f`).
If you want to set beat, or take cards, press `Enter`

After throwing card you will see like that:
```
<  6 >   
def      
^3  /    
Your cards:                                      < &9  > [ 40 ]
[f: ^2 ]   [e: &Q ]   [d: #6 ]   [b: ?K ]   [a: &5 ]   
throw up>
```
- `^3  /` card which was thrown. If player who defence throw his card, his card will be shown on the right

When the game ends the server shuts down.

Enjoy a game!
