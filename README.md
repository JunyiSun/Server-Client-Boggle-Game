To compile, simply type in the project directory: make

1. RUN THE SERVER: ./game_server

2. Run THE CLIENT IN A DIFFERENT SHELL: ./game_client 127.0.0.1 
(this is just an example if you run the client on the local machine) 

3. Then you will type in your user name

4. After login, you can give commands like :

top_3
all_players
new_game

5. After a game board is displayed, you can type the word you find. 
The boggle game will check if the word you provide is valid or not. 
If it’s valid, your score will be updated automatically.

You can type add_score (you don’t need any parameters this time) to submit your score and end the current game. 

For every 2 minutes, the server will automatically end the current game and submit every client’s current score. A new game board is generated if you want to play a new game now. 

6. If the client wants to quit the game, type: quit


