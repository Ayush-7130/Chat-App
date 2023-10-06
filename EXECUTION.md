# How to Execute the Program (UBUNTU): -

**Prerequisites:** installed gcc, mingw, preferably an IDE supporting C++ like VScode

The steps to execute the code are as follows:
1.	Compile the C program files “Server.c” and “Client.c” by using the follow-ing command, “gcc -o s Server.c -lpthread” for the server file and “gcc -o c Client.c -lpthread” for the client file.
2.	Then, open three terminals on the working directory. Run “./s” on one terminal and “./c” on the other two terminals.
3.	Now, we are ready to connect those two clients via the server.
4.	On the client-side terminal, we can see various data that show a par-ticular client’s Instance ID and various options such as “STATUS, CHAT, CLOSE”.
5.	So, if we want to chat between clients then we have to give the command “CHAT {Instance ID of another client}”, if that client is free the connec-tion will be established between both of them.
6.	To close the connection between clients, we have to give the “GOOD-BYE!” command.
7.	And if we chat, then we can see the server gets the encrypted message but another client receives the original message as other client types.
8.	For closing the client, we have to give the command “CLOSE” after we are done with the “GOODBYE!” command. And for the server, we have to force stop by “ctrl + c”.