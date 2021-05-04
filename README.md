# Chat Client/Server [CLOSED]
The objective of the project is the realization of a client/server chat application in C allowing to exchange messages between 2 users, between several users, or to all the users connected on the network (localhost), as well as to send files.

## Table of contents
* [My goals](#my-goals)
* [Technologies](#technologies)
* [Description](#description)
* [Acquired skills](#acquired-skills)
* [Informations](#informations)

## My goals
- Manipulate network primitives and POSIX sockets in C : socket(), bind(), listen(), connect(), accept(), send(), recv() 
- Implement communications over TCP/IP 

## Technologies
The project is developped with **C**.

## Description

#### Step 1 : Realization of an IPv4 client/server model where the server sends the previously sent string back to the client (100%)
- In this communication model, there is a **client part** (see _client.c_ file) and a **server part** (see _server.c_ file). For the first part, the client will have to create a TCP socket (with an IPv4 address and a port number given as an argument to the program), establish a connection to the server socket. The sending and receiving of data can then be done for the client. At the same time, on the server's side, a socket must be created, linked to a listening port and the new connection must be located. Once the connection has been accepted, data can be received and sent. In this milestone, once the connection is established, the client must send a string typed on the keyboard to the server. In response, the server retrieves this string and sends it directly to the user in question who must display it on his screen.
- The server must be **able to manage several clients in parallel**. To do this, a single process is used to perform all the tasks using the poll() function. The client information (client socket file descriptor and address/port) is stored by the server in a linked list.
- The client must be **able to handle both messages from the server and text typed on the keyboard**. To do this, the poll() function is also be used on the client side on the standard program input and the socket for communication with the server.

**Connexion to the serve as a client and discussion with the server (automatic response) :**
![chat_connexion](https://user-images.githubusercontent.com/56866008/116995572-84746280-acda-11eb-9876-9bf74c3b06ba.png)


#### Step 2 : Authorization of the server to retrieve, store and use information about users of the instant messaging service (100%)
- The server is no longer, as in Step 1, just a repetitive server but rather **the intermediary between users for sending messages**. With this, users will be able to choose a nickname, see the nicknames of other connected users, retrieve information about those users, send private messages to one user, and send broadcast messages to all users. To do this, users will need to type commands before their message (/nick, /who, /whois , /msgall , /msg ) that will be interpreted by the client and server.
- From this milestone onwards, a **specific structure is sent** instead of simple for messages (see in the _msg_struct.h_ file).

**Discussions bewteen two clients :**
![chat_discussions](https://user-images.githubusercontent.com/56866008/116995577-850cf900-acda-11eb-81ca-967c4a8b67e6.png)

#### Step 3 : Realization of messages between users so that your application becomes a full-fledged instant messaging application (100%)
- Until now, the server only allowed to send private messages in unicast and broadcast messages that's why the notion of **application multicast** is introduced here for the management of chat rooms.
- For application multicast, a user can **create a room**.
- Users can then **join this room**, and once registered, the users of the room can **exchange messages** between themselves.
- Users can **leave the room or change rooms** whenever they want.

**Create, join, leave a room and discuss with the members present :**
![chat_room_1](https://user-images.githubusercontent.com/56866008/116995580-85a58f80-acda-11eb-8996-15bc5c16b016.png)
![chat_room_2](https://user-images.githubusercontent.com/56866008/116995581-85a58f80-acda-11eb-9267-4458b1b5244f.png)

#### Step 4 : Authorization of a user to send a file to another user (50%)
- Several schemes are possible to implement this functionality but **the P2P (Peer to Peer) mode** (which does not require passing the server to exchange data) has been retained. The server is only used to connect the users.

#### BONUS :
Additional commands have been created such as :
- /help : to see all commands
- /color : to change the color of the terminal (see _textcolor.c_ and _textcolor.h_ files)
- /man_color : to see the manual of the /color command

**View all commands :**
![chat_command_help](https://user-images.githubusercontent.com/56866008/116995576-850cf900-acda-11eb-8563-1cfb3d174559.png)

## Acquired skills
- Manipulation of C structures
- Implementation of TCP/IP communications 

## Informations
This project is an academic project for "RE216 - Projet de programmation réseau" during my 2nd year of engineering school at ENSEIRB-MATMECA.
