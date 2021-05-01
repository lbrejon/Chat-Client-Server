#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <unistd.h>
#include <ctype.h>
#include <poll.h>
#include <assert.h>
#include <time.h>

#include "common.h"
#include "msg_struct.h"

#define BUF_SIZE 4096
#define MAX_CLIENT 10
#define DATE_LEN 128

struct client{
  int socket;
  char pseudo[NICK_LEN];
  char infos[INFOS_LEN];
  char date[DATE_LEN];
  char IP[BUF_SIZE];
  char port[BUF_SIZE];
  char room[NICK_LEN];
  struct client *next;
};

struct room{
  char room_name[NICK_LEN];
  int nb_user;
  struct room *next;
};

struct pollfd get_struct_pollfd(int i, struct pollfd *fds){
  return fds[i];
}

// Send the message structure + the message
void send_msg(int socket, struct message msgstruct, char *buffer){
  int sent1 = -1;
  int sent2 = -1;
  int len = msgstruct.pld_len;

  sent1 = send(socket, &msgstruct, sizeof(struct message), 0);
  if (sent1 <= 0){
    perror("Error while sending structure\n");
    exit(1);
  }
  sent2 = send(socket, buffer, len, 0);
  if (sent2 <= 0){
    perror("Error while sending message\n");
    exit(1);
  }
  printf("Structure sent : pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
  printf("Message sent : '%s' from client %i\n\n", buffer, socket-3);
}

//Initialisation of the room blockchain
struct room* room_list_init(){
  struct room *room_list_init = malloc(sizeof(*room_list_init));
  if (room_list_init == NULL){
    perror("Error while initializing client blockchain");
    exit(EXIT_FAILURE);
  }
  return room_list_init;
}

int check_name_room(struct room *room_list, char *name){
  if (room_list == NULL){
    perror("ERROR finding room");
    exit(EXIT_FAILURE);
  }
  struct room *tmp = room_list;

  if (!strcmp(tmp->room_name,name))
    return 1;
  if (tmp->next != NULL){
    tmp = tmp->next;
    while(tmp != NULL){
      if (!strcmp(tmp->room_name,name))
        return 1;
      tmp = tmp->next;
    }
  }
  return 0;
}

// Create a room to the blockchain
struct room* create_room(struct room* room_list, struct client* current_client, char * name, char * buffer){
  if (room_list == NULL){
    room_list = room_list_init();
  }

  struct room *tmp = room_list;
  struct room *new_room;
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));
  memset(buffer, '\0', BUF_SIZE);

  if (check_name_room(tmp,name) == 1){ // if room name is already used
    strcpy(buffer,"Error : room [");
    strcat(buffer,name);
    strcat(buffer,"] already exists");
    msgstruct.type = SERVER;
    strcpy(msgstruct.infos,"");
    msgstruct.pld_len = strlen(buffer);
    send_msg(current_client->socket, msgstruct, buffer);
    return NULL;
  }
  else{ // room name is valid
    if (strcmp(current_client->room,name) && strcmp(name,"") && strcmp(current_client->room,"")){ // if user is into a room
      strcpy(buffer,"Sorry, you are already in a room. If you want to create a new one, please quit this room before");
      msgstruct.type = ERROR_MULTICAST_CREATE;
      strcpy(msgstruct.infos,"");
      strcpy(msgstruct.nick_sender, current_client->pseudo);
      msgstruct.pld_len = strlen(buffer);
      send_msg(current_client->socket, msgstruct, buffer);
      return NULL;
    }
    else{
      tmp = tmp->next;
      while (tmp != NULL){
        if (strcmp(current_client->room,name) && strcmp(name,"") && strcmp(current_client->room,"") && strcmp(tmp->room_name,name)){ // if user is into a room
          strcpy(buffer,"Sorry, you are already in a room. If you want to create a new one, please quit this room before");
          msgstruct.type = ERROR_MULTICAST_CREATE;
          strcpy(msgstruct.infos,"");
          strcpy(msgstruct.nick_sender, current_client->pseudo);
          msgstruct.pld_len = strlen(buffer);
          send_msg(current_client->socket, msgstruct, buffer);
          return NULL;
        }
        tmp = tmp->next;
      }
      new_room = malloc(sizeof(*new_room));
      new_room->nb_user = 1;
      strcpy(new_room->room_name,name);
      new_room->next = NULL;
      strcpy(buffer,"You have joined channel [");
      strcat(buffer, name);
      strcat(buffer, "]");
      strcpy(current_client->room,name);
      msgstruct.type = MULTICAST_CREATE;

      strcpy(msgstruct.infos,name);
      strcpy(msgstruct.nick_sender, current_client->pseudo);
      msgstruct.pld_len = strlen(buffer);
      send_msg(current_client->socket, msgstruct, buffer);

      return new_room;
    }
  }
}


// Delete a client from the blockchain
struct room *delete_room(struct room *room_list,char *room_to_delete){
  if (room_list == NULL){
    perror("ERROR deleting room");
    exit(EXIT_FAILURE);
  }
  struct room* tmp = room_list;
  struct room* previous;

  if (strcmp(tmp->room_name,room_to_delete) == 0){
    room_list = tmp->next;
    free(tmp);
    return room_list;
  }

  previous = tmp->next;
  while (previous != NULL){
    if ((previous->next == NULL) && (strcmp(previous->room_name,room_to_delete) == 0)){
      tmp->next = NULL;
      return room_list;
    }
    if (strcmp(previous->room_name,room_to_delete) == 0){
      tmp->next = previous->next;
      free(previous);
      return room_list;
    }
    tmp = previous;
    previous = previous->next;
  }
  return room_list;
}

void channel_list(struct room *room_list, struct client *current_client, char* list){

  struct room *tmp = room_list;
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));
  memset(list, '\0', BUF_SIZE);

  if (strlen(tmp->room_name) == 0){
    strcpy(list,"There is no room");
    msgstruct.type = SERVER;
  }
  else{
    strcpy(list, "Rooms list is :");
    strcat(list,"\n               - ");
    strcat(list,tmp->room_name);
    msgstruct.type = MULTICAST_LIST;

    if (tmp->next != NULL){
      tmp = tmp->next;
      while(tmp != NULL){
        strcat(list,"\n               - ");
        strcat(list,tmp->room_name);
        tmp = tmp->next;
      }
    }
  }
  strcpy(msgstruct.nick_sender, current_client->pseudo);
  msgstruct.pld_len = strlen(list);
  send_msg(current_client->socket, msgstruct, list);
}


void quit_room(struct room *room_list, struct client* current_client, char *name, char *buffer){
    struct room *tmp = room_list;
    struct message msgstruct;
    memset(&msgstruct, 0, sizeof(struct message));
    memset(buffer, '\0', BUF_SIZE);

    if (check_name_room(room_list,name) == 0){
      strcpy(buffer, "Error : ");
      strcat(buffer,name);
      strcat(buffer," is not a channel");
      msgstruct.type = SERVER;
      strcpy(msgstruct.infos,name);
      strcpy(msgstruct.nick_sender, current_client->pseudo);
      msgstruct.pld_len = strlen(buffer);
      send_msg(current_client->socket, msgstruct, buffer);
    }

    else{
      if (!strcmp(current_client->room, tmp->room_name)){
        strcpy(current_client->room,"");
        strcpy(buffer,"You have left the channel [");
        strcat(buffer,name);
        strcat(buffer,"]");
        tmp->nb_user -= 1;
        if (tmp->nb_user == 0){
          printf("C\n");
          room_list = delete_room(room_list,tmp->room_name);
          strcat(buffer,". Channel [");
          strcat(buffer,name);
          strcat(buffer,"] has been destroyed");
        }
        msgstruct.type = QUIT_CHANNEL;
        strcpy(msgstruct.infos,name);
        strcpy(msgstruct.nick_sender, current_client->pseudo);
        msgstruct.pld_len = strlen(buffer);
        send_msg(current_client->socket, msgstruct, buffer);
      }
      else{
        tmp = tmp->next;
        while (tmp != NULL){
          if (!strcmp(current_client->room, tmp->room_name)){
            if (!strcmp(current_client->room, name)){
              strcpy(current_client->room,"");
              strcpy(buffer,"You have left the channel [");
              strcat(buffer,name);
              strcat(buffer,"]");
              tmp->nb_user -= 1;
              if (tmp->nb_user == 0){
                room_list = delete_room(room_list,tmp->room_name);
                strcat(buffer,". Channel [");
                strcat(buffer,name);
                strcat(buffer,"] has been destroyed");
              }
              msgstruct.type = QUIT_CHANNEL;
              strcpy(msgstruct.infos,name);
              strcpy(msgstruct.nick_sender, current_client->pseudo);
              msgstruct.pld_len = strlen(buffer);
              send_msg(current_client->socket, msgstruct, buffer);
              break;
            }
          }
          tmp = tmp->next;
        }
      }
    }
  }

void join_room(struct room *room_list, struct client *current_client, char *name, char *buffer){
  struct room *tmp = room_list;
  struct message msgstruct;
  int cpt = 0;
  memset(&msgstruct, 0, sizeof(struct message));
  memset(buffer, '\0', BUF_SIZE);

  if (check_name_room(tmp,name) == 0){ // room name doesn't exist
    strcpy(buffer,"Error : room [");
    strcat(buffer,name);
    strcat(buffer,"] doesn't exist");
    msgstruct.type = SERVER;
    strcpy(msgstruct.infos,"");
    msgstruct.pld_len = strlen(buffer);
    send_msg(current_client->socket, msgstruct, buffer);
  }
  else{ // room name exists
    if (strlen(current_client->room) != 0){ // client is already into a room

      if (!strcmp(name, current_client->room)){ // if client want to join his own room
        strcpy(buffer,"Error : you cannot join channel [");
        strcat(buffer,name);
        strcat(buffer,"] because you are already in");
        msgstruct.type = SERVER;
        strcpy(msgstruct.infos,"");
        msgstruct.pld_len = strlen(buffer);
        send_msg(current_client->socket, msgstruct, buffer);
        cpt = 1;
      }
      else{
        quit_room(room_list, current_client, current_client->room, buffer);
      }
      memset(buffer, '\0', BUF_SIZE);
    }
    if (cpt == 0){
      if (!strcmp(tmp->room_name,name)){
        strcpy(current_client->room,name);
        tmp->nb_user += 1;
        strcpy(buffer,"You have joined the channel ");
        strcat(buffer,name);
      }
      else{
        if (tmp->next != NULL){
          tmp = tmp->next;
          while(tmp != NULL){
            if (!strcmp(tmp->room_name,name)){
              strcpy(current_client->room,name);
              tmp->nb_user += 1;
              strcpy(buffer,"You have joined the channel ");
              strcat(buffer,name);
              break;
            }
            tmp = tmp->next;
          }
        }
      }
      msgstruct.type = MULTICAST_JOIN;
      strcpy(msgstruct.infos,name);
      msgstruct.pld_len = strlen(buffer);
      send_msg(current_client->socket, msgstruct, buffer);
    }
  }
}

void msg_room(struct room *room_list, struct client* client_list, struct client* current_client, char *name, char * buffer){
  if (client_list == NULL){
    perror("Error while finding client in blockchain");
    exit(EXIT_FAILURE);
  }

  int client_socket;
  struct client *tmp = client_list;
  struct client *tmp2;
  struct room *current_room = room_list;
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));

  while(strcmp(current_room->room_name,name)){
    current_room = current_room->next;
  }

  tmp2 = tmp->next;
  if (current_room->nb_user == 1){
    memset(buffer, '\0', BUF_SIZE);
    strcpy(buffer,"Sorry but you are alone in the channel");
    msgstruct.type = SERVER;
    strcpy(msgstruct.nick_sender,current_client->pseudo);
    strcpy(msgstruct.infos,"");
    msgstruct.pld_len = strlen(buffer);
    send_msg(tmp->socket, msgstruct, buffer);
  }

  else{
    while (tmp->next != NULL){
      if ((strlen(current_client->room) != 0) && (tmp2->next == NULL)){
        memset(buffer, '\0', BUF_SIZE);
        strcpy(buffer,"Sorry but you are the only connected user on the chat. Please try later");
        msgstruct.type = SERVER;
        strcpy(msgstruct.infos,"");
        strcpy(msgstruct.nick_sender,current_client->pseudo);
        msgstruct.pld_len = strlen(buffer);
        send_msg(tmp->socket, msgstruct, buffer);
      }
      else{
        if (strcmp(tmp->pseudo,current_client->pseudo) && (strlen(tmp->pseudo) != 0) && !(strcmp(current_client->room,tmp->room))){
          client_socket = tmp->socket;
          msgstruct.pld_len = strlen(buffer);
          strcpy(msgstruct.nick_sender,current_client->pseudo);
          msgstruct.type = MULTICAST_SEND;
          strcpy(msgstruct.infos,"\0");
          send_msg(client_socket, msgstruct, buffer);
        }
      }
      tmp = tmp->next;
    }
  }
}



//Initialization of the client blockchain
struct client* client_list_init(){
  struct client *client_list_init = malloc(sizeof(*client_list_init));

  if (client_list_init == NULL){
    perror("Error while initializing client blockchain");
    exit(EXIT_FAILURE);
  }
  return client_list_init;
}


//Add a client to the blockchain
struct client* add_client(struct client* client_list, int socket, char *port, char *IP){
  if (client_list == NULL){
    perror("Error while finding client in blockchain");
    exit(EXIT_FAILURE);
  }

  struct client *new_client = malloc(sizeof(*new_client));
  char client_date[DATE_LEN];
  struct tm* date;
  time_t client_time;

  // Setting the date when a client is connected
  time(&client_time);
  date = localtime(&client_time);
  memset(client_date,'\0',DATE_LEN);
  sprintf(client_date,"%d/%d/%d@%d:%d", date->tm_year + 1900, date->tm_mon + 1, date->tm_mday, date->tm_hour, date->tm_min);

  // Filling the client structure
  new_client->socket = socket;
  strcpy(new_client->pseudo,"");
  strcpy(new_client->infos,"");
  strcpy(new_client->date,client_date);
  strcpy(new_client->IP,IP);
  strcpy(new_client->port,port);
  strcpy(new_client->room,"");
  new_client->next = client_list;

  return new_client;
}

//Delete a client from the client blockchain
struct client *delete_client(struct client *client_list,int socket_fd){
  if (client_list == NULL){
    perror("Error while finding client in blockchain");
    exit(EXIT_FAILURE);
  }

  struct client* tmp = client_list;
  struct client* previous;

  if (tmp->socket == socket_fd){
    client_list=tmp->next;
    free(tmp);
    return client_list;
  }

  previous = tmp->next;
  while (previous != NULL){
    if (previous->next == NULL && previous->socket == socket_fd){
      tmp->next = NULL;
      return(client_list);
    }
    if (previous->socket == socket_fd){
      tmp->next = previous->next;
      free(previous);
      return(client_list);
    }
    tmp = previous;
    previous = previous->next;
  }
  return client_list;
}


// Get a client thanks to his socket number
struct client *update_client(struct client *client_list, int socket_number){
  if (client_list == NULL){
    perror("Error while finding client in blockchain");
    exit(EXIT_FAILURE);
  }

  struct client *tmp = client_list;

  while(tmp->next != NULL){
    if (tmp->socket == socket_number){
      return tmp;
    }
    tmp = tmp->next;
  }
  return tmp;
}


// Check if the pseudo of the client is in the client blockchain
int check_client_pseudo(struct client *client_list, char *client_pseudo){
  if (client_list == NULL){
    perror("Error while finding client in blockchain");
    exit(EXIT_FAILURE);
  }
  struct client *tmp = client_list;

  if (!strcmp(tmp->pseudo,client_pseudo)){
    return 1;
  }
  tmp = tmp->next;
  while(tmp->next != NULL){
    if (!strcmp(tmp->pseudo,client_pseudo)){
      return 1;
    }
    tmp = tmp->next;
  }
  return 0;
}

// Message sent while username has not been set
void welcome(struct client *client_list, struct client *current_client, char * buffer){
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));

  memset(buffer, '\0', BUF_SIZE);
  strcpy(buffer,"Please choose your pseudo with /nick <pseudo>");
  msgstruct.type = SERVER;
  strcpy(msgstruct.infos,"");
  msgstruct.pld_len = strlen(buffer);
  send_msg(current_client->socket, msgstruct, buffer);
}

// Set a new username
void nickname(struct client *client_list, struct client *current_client, char *buffer, char *info){
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));

  if (!strcmp(current_client->pseudo,"\0")){
    if (check_client_pseudo(client_list,info) == 1){ // user has choosen an invalid pseudo for his first pseudo
      memset(buffer, '\0', BUF_SIZE);
      msgstruct.type = SERVER;
      strcpy(buffer,">Error : pseudo already used, please choose a new one with /nick <pseudo>");
      memset(msgstruct.infos,'\0',INFOS_LEN);
    }
    else{ // user choose a pseudo for the first time
      strcpy(msgstruct.nick_sender,info);
      strcpy(msgstruct.infos,info);
      memset(buffer, '\0', BUF_SIZE);
      strcpy(buffer,"--- Welcome on the chat ");
      strcat(buffer, info);
      strcat(buffer, " ---      To see all commands, use command /help");
      strcpy(current_client->pseudo,info);
    }
  }
  else{ //if the user wants to change his pseudo
    strcpy(msgstruct.nick_sender,info);
    if (check_client_pseudo(client_list,info) == 1){ // pseudo is already used
      memset(buffer, '\0', BUF_SIZE);
      strcpy(buffer,"Error : pseudo already used, please choose a new one with /nick <pseudo>");
      strcpy(msgstruct.nick_sender,current_client->pseudo);
      msgstruct.type = SERVER;
      memset(msgstruct.infos,'\0',INFOS_LEN);
    }
    else{ // pseudo changed successfully
      strcpy(msgstruct.nick_sender,buffer);
      memset(buffer, '\0', BUF_SIZE);
      strcpy(buffer,"Pseudo changed successfully in ");
      strcat(buffer, info); //---
      strcpy(msgstruct.infos,info);
      strcpy(current_client->pseudo,info);
    }
  }
  msgstruct.pld_len = strlen(buffer);
  send_msg(current_client->socket, msgstruct, buffer);
}

// List connected users which have an username
void who(struct client *client_list, struct client *current_client, char * list){
  if (client_list == NULL){
    perror("Error while finding client in blockchain");
    exit(EXIT_FAILURE);
  }

  struct client *tmp = client_list;
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));
  memset(list, '\0', BUF_SIZE);
  strcpy(list, "Online users are");

  if (strlen(tmp->pseudo) != 0){
    strcat(list,"\n               - ");
    strcat(list,tmp->pseudo);
  }
  tmp = tmp->next;
  while(tmp->next != NULL){
    if (strlen(tmp->pseudo) != 0){
      strcat(list,"\n               - ");
      strcat(list,tmp->pseudo);
    }
    tmp = tmp->next;
  }

  strcpy(msgstruct.nick_sender,current_client->pseudo);
  msgstruct.type = NICKNAME_LIST;
  msgstruct.pld_len = strlen(list);
  send_msg(current_client->socket, msgstruct, list);
  strcpy(current_client->infos,msgstruct.infos);
}

// Give informations about a specific client
void whois(struct client *client_list, struct client *current_client, char * list, char * info){
  if (client_list == NULL){
    perror("Error while finding client in blockchain");
    exit(EXIT_FAILURE);
  }

  struct client *tmp = client_list;
  struct message msgstruct;

  memset(&msgstruct, 0, sizeof(struct message));
  memset(list, '\0', BUF_SIZE);

  if (!strcmp(tmp->pseudo, info)){
    strcpy(list, tmp->pseudo);
    strcat(list, " connected since ");
    strcat(list,tmp->date);
    strcat(list," with IP address ");
    strcat(list,tmp->IP);
    strcat(list," and port number ");
    strcat(list,tmp->port);
    msgstruct.type = NICKNAME_INFOS;
  }
  else{
    tmp = tmp->next;
    while(tmp->next != NULL){
      if (!strcmp(tmp->pseudo, info)){
        strcpy(list, tmp->pseudo);
        strcat(list, " connected since ");
        strcat(list,tmp->date);
        strcat(list," with IP address ");
        strcat(list,tmp->IP);
        strcat(list," and port number ");
        strcat(list,tmp->port);
        msgstruct.type = NICKNAME_INFOS;
        break;
      }
      tmp = tmp->next;
    }
  }

  if(strlen(list) == 0){
    strcpy(list,"Error : ");
    strcat(list, info);
    strcat(list, " is not an user.");
    msgstruct.type = SERVER;
  }

  strcpy(msgstruct.nick_sender,current_client->pseudo);
  msgstruct.pld_len = strlen(list);
  send_msg(current_client->socket, msgstruct, list);
  strcpy(current_client->infos,msgstruct.infos);
}

// Send a message to all connected user
void msg_all(struct client* client_list, struct client* current_client, char * buffer){
  if (client_list == NULL){
    perror("Error while finding client in blockchain");
    exit(EXIT_FAILURE);
  }

  int client_socket;
  struct client *tmp = client_list;
  struct client *tmp2;
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));

  tmp2 = tmp->next;
  if (tmp2->next == NULL){
    memset(buffer, '\0', BUF_SIZE);
    strcpy(buffer,"Sorry but you are the only connected user on the chat. Please try later");
    strcpy(msgstruct.nick_sender, current_client->pseudo);
    msgstruct.type = SERVER;
    msgstruct.pld_len = strlen(buffer);
    send_msg(current_client->socket, msgstruct, buffer);
  }
  else{
    while (tmp->next != NULL){
      if (strlen(tmp->pseudo) == 0){
        memset(buffer, '\0', BUF_SIZE);
        strcpy(buffer,"Sorry but you are the only connected user on the chat. Please try later");
        msgstruct.type = SERVER;
        strcpy(msgstruct.nick_sender, current_client->pseudo);
        msgstruct.pld_len = strlen(buffer);
        send_msg(current_client->socket, msgstruct, buffer);
      }
      else{
        if (strcmp(tmp->pseudo,current_client->pseudo) && (strlen(tmp->pseudo) != 0)){
          client_socket = tmp->socket;
          msgstruct.pld_len = strlen(buffer);
          strcpy(msgstruct.nick_sender,current_client->pseudo);
          msgstruct.type = BROADCAST_SEND;
          strcpy(msgstruct.infos,"\0");
          send_msg(client_socket, msgstruct, buffer);
        }
      }
      tmp = tmp->next;
    }
  }
}

  void msg_spe(struct client *client_list, struct client * current_client, char * buffer, char * info){
    if (client_list == NULL){
      perror("Error while finding client in blockchain");
      exit(EXIT_FAILURE);
    }

    struct client *tmp = client_list;
    struct message msgstruct;
    int socket;
    memset(&msgstruct, 0, sizeof(struct message));

    if (check_client_pseudo(client_list,info) == 0){
      memset(buffer, '\0', BUF_SIZE);
      strcpy(buffer,"Error : user ");
      strcat(buffer, info);
      strcat(buffer, " does not exist");
      strcpy(msgstruct.nick_sender, current_client->pseudo);
      msgstruct.type = SERVER;
      strcpy(msgstruct.infos,info);
      msgstruct.pld_len = strlen(buffer);
      send_msg(current_client->socket, msgstruct, buffer);
    }
    else{
      msgstruct.pld_len = strlen(buffer);
      strcpy(msgstruct.nick_sender, current_client->pseudo);
      msgstruct.type = UNICAST_SEND;
      strcpy(msgstruct.infos,current_client->pseudo);

      if (!strcmp(tmp->pseudo, info) && strcmp(current_client->pseudo, info)){
        socket = tmp->socket;
        send_msg(socket, msgstruct, buffer);
      }
      else{
        tmp = tmp->next;
        while(tmp->next != NULL){
          if (!strcmp(tmp->pseudo, info) && strcmp(current_client->pseudo, info)){
            socket = tmp->socket;
            send_msg(socket, msgstruct, buffer);
            break;
          }
          tmp = tmp->next;
        }
      }
    }
  }


void msg_file_request(struct client *client_list, struct client * current_client, char * buffer, char * info){
  if (client_list == NULL){
    perror("Error while finding client in blockchain");
    exit(EXIT_FAILURE);
  }

  struct client *tmp = client_list;
  struct message msgstruct;
  char buf[BUF_SIZE];
  int socket;
  memset(&msgstruct, 0, sizeof(struct message));
  memset(buf, '\0', BUF_SIZE);

  if (check_client_pseudo(client_list,info) == 0){
    memset(buffer, '\0', BUF_SIZE);
    strcpy(buffer,"Error : user ");
    strcat(buffer, info);
    strcat(buffer, " does not exist");
    strcpy(msgstruct.nick_sender, current_client->pseudo);
    msgstruct.type = SERVER;
    strcpy(msgstruct.infos,"");
    msgstruct.pld_len = strlen(buffer);
    send_msg(current_client->socket, msgstruct, buffer);
  }
  else{
    strcpy(buf, buffer);
    memset(buffer, '\0', BUF_SIZE);
    strcpy(buffer, current_client->pseudo);
    strcat(buffer, " wants you to accept the transfer of the file named ");
    strcat(buffer, buf);
    strcat(buffer, " . Do you accept ? [Yes/No]");
    msgstruct.pld_len = strlen(buffer);
    strcpy(msgstruct.nick_sender, current_client->pseudo);
    msgstruct.type = FILE_REQUEST;
    strcpy(msgstruct.infos, info);

    if (!strcmp(tmp->pseudo, info) && strcmp(current_client->pseudo, info)){
      socket = tmp->socket;
      send_msg(socket, msgstruct, buffer);
    }
    else{
      tmp = tmp->next;
      while(tmp->next != NULL){
        if (!strcmp(tmp->pseudo, info) && strcmp(current_client->pseudo, info)){
          socket = tmp->socket;
          send_msg(socket, msgstruct, buffer);
          break;
        }
        tmp = tmp->next;
      }
    }
  }
}

void msg_file_accept(struct client *client_list, struct client * current_client, char * buffer, char * info){
  if (client_list == NULL){
    perror("Error while finding client in blockchain");
    exit(EXIT_FAILURE);
  }

  struct client *tmp = client_list;
  struct message msgstruct;
  int socket;
  memset(&msgstruct, 0, sizeof(struct message));

  msgstruct.pld_len = strlen(buffer);
  strcpy(msgstruct.nick_sender, current_client->pseudo);
  msgstruct.type = FILE_ACCEPT;
  strcpy(msgstruct.infos, info);

  if (!strcmp(tmp->pseudo, info) && strcmp(current_client->pseudo, info)){
    socket = tmp->socket;
    send_msg(socket, msgstruct, buffer);
  }
  else{
    tmp = tmp->next;
    while(tmp->next != NULL){
      if (!strcmp(tmp->pseudo, info) && strcmp(current_client->pseudo, info)){
        socket = tmp->socket;
        send_msg(socket, msgstruct, buffer);
        break;
      }
      tmp = tmp->next;
    }
  }
}

void msg_file_reject(struct client *client_list, struct client * current_client, char * buffer, char * info){
  if (client_list == NULL){
    perror("Error while finding client in blockchain");
    exit(EXIT_FAILURE);
  }

  struct client *tmp = client_list;
  struct message msgstruct;
  int socket;
  memset(&msgstruct, 0, sizeof(struct message));

  msgstruct.pld_len = strlen(buffer);
  strcpy(msgstruct.nick_sender, current_client->pseudo);
  msgstruct.type = FILE_REJECT;
  strcpy(msgstruct.infos, info);

  if (!strcmp(tmp->pseudo, info) && strcmp(current_client->pseudo, info)){
    socket = tmp->socket;
    send_msg(socket, msgstruct, buffer);
  }
  else{
    tmp = tmp->next;
    while(tmp->next != NULL){
      if (!strcmp(tmp->pseudo, info) && strcmp(current_client->pseudo, info)){
        socket = tmp->socket;
        send_msg(socket, msgstruct, buffer);
        break;
      }
      tmp = tmp->next;
    }
  }
}




// Send a echo-reply message to client
void echo_reply(struct client *current_client, char * buffer){
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));

  strcpy(msgstruct.nick_sender,current_client->pseudo);
  strcpy(msgstruct.infos,"");
  msgstruct.type = ECHO_SEND;
  msgstruct.pld_len = strlen(buffer);
  send_msg(current_client->socket, msgstruct, buffer);
}


// Send a error login message to client
void error_login(struct client *current_client, char * buffer){
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));

  if(strlen(buffer) > NICK_LEN){
    memset(buffer, '\0', BUF_SIZE);
    strcpy(buffer,"Error : pseudo too long (max length = 128 ). Please choose a valid pseudo !");
  }
  else{
    memset(buffer, '\0', BUF_SIZE);
    strcpy(buffer,"Error : pseudo erroned (invalid characters). Please choose a valid pseudo with /nick <pseudo>");
  }
  msgstruct.type = SERVER;
  strcpy(msgstruct.infos,"");
  msgstruct.pld_len = strlen(buffer);
  send_msg(current_client->socket, msgstruct, buffer);
}


// Send a invalid command message to client
void error_command(struct client *current_client, char * buffer){
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));

  if (!strcmp(buffer,"INVALID COMMAND TO FILE REQUEST")){
    memset(buffer, '\0', BUF_SIZE);
    strcpy(buffer,"Error : please answer with Yes or No");
    msgstruct.type = SERVER;
    strcpy(msgstruct.infos,"");
    msgstruct.pld_len = strlen(buffer);
    send_msg(current_client->socket, msgstruct, buffer);
  }
  else if (!strcmp(buffer,"INVALID COMMAND TO COLOR")){
    memset(buffer, '\0', BUF_SIZE);
    strcpy(buffer,"Error : please check the manual of /color command with /man_color");
    msgstruct.type = SERVER;
    strcpy(msgstruct.infos,"");
    msgstruct.pld_len = strlen(buffer);
    send_msg(current_client->socket, msgstruct, buffer);
  }
  else{
    memset(buffer, '\0', BUF_SIZE);
    strcpy(buffer,"Error : invalid command. Use command /help to see all commands");
    msgstruct.type = SERVER;
    strcpy(msgstruct.infos,"");
    msgstruct.pld_len = strlen(buffer);
    send_msg(current_client->socket, msgstruct, buffer);
  }
}

// Send a message which will pimp client interface
void color_client(struct client *current_client, char * buffer){
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));

  if (!strcmp(buffer,"RESET") || !strcmp(buffer,"BLACK") || !strcmp(buffer,"RED") || !strcmp(buffer,"GREEN") || !strcmp(buffer,"YELLOW") || !strcmp(buffer,"BLUE") || !strcmp(buffer,"MAGENTA") || !strcmp(buffer,"CYAN") || !strcmp(buffer,"WHITE")){
    strcpy(msgstruct.nick_sender,current_client->pseudo);
    strcpy(msgstruct.infos,buffer);
    memset(buffer, '\0', BUF_SIZE);
    sprintf(buffer,"You pimp your client/server in %s color", msgstruct.infos);
    msgstruct.type = COLOR;
  }
  else{
    memset(buffer, '\0', BUF_SIZE);
    strcpy(buffer,"Error : color invalid. Use command /man_color to see the manual");
    strcpy(msgstruct.nick_sender,current_client->pseudo);
    msgstruct.type = SERVER;
    memset(msgstruct.infos,'\0',INFOS_LEN);
  }
  msgstruct.pld_len = strlen(buffer);
  send_msg(current_client->socket, msgstruct, buffer);
}

// Send a message which explain command /color
void man_color_client(struct client *current_client, char * buffer){
  struct message msgstruct;
  memset(&msgstruct, 0, sizeof(struct message));


  memset(buffer, '\0', BUF_SIZE);
  strcpy(buffer,"\n\nCOLOR(1)                             Linux Programmer's Manual                             COLOR(1)\n\n");
  strcat(buffer,"NAME\n       color, color_letter_case - color and change the letter case of the interface\n\n");
  strcat(buffer,"SYNOPSIS\n       #include<sys/color.h>\n       #include<sys/lettercase.h>\n\n       void /color char *col\n       void /color_letter_case char *col int flags\n\n");
  strcat(buffer,"DESCRIPTION\n       Commands /color and /color_letter_case are used to channge the color and modificate the letter case of the interface.\n\n");
  strcat(buffer,"       The /color command enable to change the interface color in color col with a bright letter case. Valids colors col are : \n");
  strcat(buffer,"              - BLACK\n              - RED\n              - GREEN\n              - YELLOW\n              - BLUE\n              - MAGENTA\n              - CYAN\n              - WHITE\n");
  strcat(buffer,"       The /color_letter_case command enable to change the interface color in color col and modificate the letter case with the flag flags. Valids colors col are the same than for the command /color and valids flags flag are : \n");
  strcat(buffer,"              - BRIGHT              - DIM              - UNDERLINE\n              - BLINK              - REVERSE              - HIDDEN\n\n");
  strcat(buffer,"RETURN VALUE\n       On success, change and modificate color/letter case but in case of error, return an error message which encourage you to check his manual page\n\n");
  strcat(buffer,"NOTE\n       /color RESET will reset the color\n\n");
  strcat(buffer,"EXAMPLE\n       /color RED will pimp your client interface in red color with bright letter case \n\n");
  strcat(buffer,"HISTORY\n       Once upon a time, a student in Telecommunications from a french engineering school ENSEIRB-MATMECA, has to work on a programmation network project. He was blocked on the fourth jalon where he should implement a peer-to-peer connection between to clients. He didn't know how to do that although he did some attempts. That's why he decided to not waste his time and did his best to get the +2 bonus points with the command /help, /color and /man_color. In fact, he wanted to pimp his client interface, so the creation of this command.\n\n");
  strcat(buffer,"AUTHOR\n       Written by Louis BREJON \n\n");
  strcat(buffer,"15 Nov 2020                             V1.0                             COLOR(1)\n\n");
  strcpy(msgstruct.nick_sender,current_client->pseudo);
  msgstruct.type = MAN_COLOR;
  memset(msgstruct.infos,'\0',INFOS_LEN);
  msgstruct.pld_len = strlen(buffer);
  send_msg(current_client->socket, msgstruct, buffer);
}


void send_welcoming_msg(int client_socket){ // Client actions
  char buffer[BUF_SIZE];
  struct message msgstruct;

  memset(buffer, '\0', BUF_SIZE);
  memset(&msgstruct, 0, sizeof(struct message));

  strcpy(buffer,"Please choose your pseudo with /nick <pseudo>");
  msgstruct.pld_len = strlen(buffer);
  strcpy(msgstruct.nick_sender,"\0");
  msgstruct.type = WELCOME;
  strcpy(msgstruct.nick_sender,"\0");

  send_msg(client_socket, msgstruct, buffer);
}


struct sockaddr_in init_address(short port){ // Create server address
  struct sockaddr_in address;
  char* ip_address = "127.0.0.1";

  memset(&address, '\0', sizeof(struct sockaddr_in));
  address.sin_port = htons(port);
  inet_aton(ip_address, &address.sin_addr);
  address.sin_family = AF_INET;
  return address;
}


// Set up sockets
int create_listener_socket(struct sockaddr_in * address){
  int socket_listener = -1;

  printf("Creating socket...\n");
  if(-1 == (socket_listener = socket(AF_INET, SOCK_STREAM, 0))){
    perror("Error creating socket");
    exit(1);
  }
  printf("Binding...\n");
  if(-1 == bind(socket_listener, (struct sockaddr *)address, sizeof(*address))){
    perror("Error binding");
    exit(1);
  }
  printf("Listening...\n");
  if(-1 == listen(socket_listener, MAX_CLIENT+1)){
      perror("Error while listening");
      return 1;
  }
  return socket_listener;
}

// Accept incoming connections
int accept_client(int socket_listener, char *port, char *IP){
  struct sockaddr_in client_addr;
  int client_socket = -1;
  int socklen = sizeof(struct sockaddr_in);

  memset(&client_addr, '\0', sizeof(client_addr));
  if((client_socket = accept(socket_listener, (struct sockaddr *)&client_addr, (socklen_t *)&socklen)) == -1){
    perror("Error while accepting");
    return 1;
  }
  strcpy(IP,inet_ntoa(client_addr.sin_addr));
  // printf("\n>>>IP is %s\n", IP);
  sprintf(port,"%d",htons(client_addr.sin_port));
  // printf(">>>Port is %s\n", port);

  return client_socket;
}


void action(int server_sock){
  int ret = -1;
  int rec1 = -1,rec2 = -1;
  int current_fd = 0;
  int client_socket = -1;

  char port[BUF_SIZE];
  char IP[BUF_SIZE];

  char buffer[BUF_SIZE];
  struct message msgstruct;
  struct pollfd fds[MAX_CLIENT];

  // Client blockchain
  struct client* client_list = client_list_init();
  struct client* current_client;

  // Room blockchain
  struct room* room_list = room_list_init();
  struct room* new_room;

  memset(buffer, '\0', BUF_SIZE);
  memset(&msgstruct, 0, sizeof(struct message));
  memset(fds,'\0', MAX_CLIENT*sizeof(struct pollfd));

  // Initialization fds[MAX_CLIENT]
  for(int i = 0; i < MAX_CLIENT; i++){
    fds[i].fd = -1;
    fds[i].events = POLLIN;
  }
  fds[0].fd = server_sock;

  while(1){
    ret = poll(fds,MAX_CLIENT,-1);

    if(ret == -1){
      perror("Error while poll");
      exit(1);
    }

    for(int i = 0; i < MAX_CLIENT; i++){

      if(i == 0 && (fds[i].revents & POLLIN) > 0){ // incoming connection from a client to the server
        client_socket = accept_client(server_sock, port, IP);
        current_fd += 1;

        for (int j = 1; j < MAX_CLIENT; j++){
          if (fds[j].fd == -1){
            fds[j].fd = client_socket;
            printf("\nAssigned %i fd to client socket %i (client %i)\nNumber of remaining connections to server : %i/%i\n\n", current_fd, client_socket, client_socket-3, MAX_CLIENT-(client_socket-3), MAX_CLIENT);
            client_list = add_client(client_list,fds[j].fd, port, IP);
            send_welcoming_msg(fds[j].fd);
            break;
          }
        }

        if (current_fd == MAX_CLIENT){
          int sock_too_much = client_socket;
          printf("Cannot accept connection : too many clients !\n");
          close(sock_too_much);
          current_fd -= 1;
          break;
        }
      }

      else if(i != 0 && (fds[i].revents & POLLIN)){
        // Receiving structure + message
        current_client = update_client(client_list,fds[i].fd);
        memset(buffer, '\0', BUF_SIZE);
        memset(&msgstruct, 0, sizeof(struct message));

        rec1 = recv(fds[i].fd, &msgstruct, sizeof(struct message), 0);
        if (rec1 == -1){
          perror("Error while receiving structure\n");
          exit(1);
        }

        rec2 = recv(fds[i].fd, buffer, msgstruct.pld_len, 0);
        if (rec2 == -1){
          perror("Error while receiving message\n");
          exit(1);
        }

        if ((rec1 == 0) || (rec2 == 0)){
          printf(">>> '%s' Server successfully disconnected because client %i used CTRL+C\n", buffer, client_socket-3);
          fds[i].events = 0;
          exit(1);
        }

        printf("Structure received : pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
        printf("Message received : '%s' from client %i\n\n", buffer, client_socket-3);


        // Sending structure + message
        switch(msgstruct.type){
          case(WELCOME):
            welcome(client_list, current_client, buffer);
            break;

          case (NICKNAME_NEW): // command /nick <pseudo>
            nickname(client_list, current_client, buffer, msgstruct.infos);
            break;

          case(NICKNAME_LIST): // command /who
            who(client_list, current_client, buffer);
            break;

          case(NICKNAME_INFOS): // command /whois <pseudo>
            whois(client_list, current_client, buffer, msgstruct.infos);
            break;

          case(BROADCAST_SEND): // command /msgall <msg>
            msg_all(client_list, current_client, buffer);
            // fds[i].revents = 0;
            break;

          case(UNICAST_SEND): // command /msg <pseudo> <msg>
            msg_spe(client_list, current_client, buffer, msgstruct.infos);
            // fds[i].revents = 0;
            break;

          case(MULTICAST_CREATE): // command /create <room>
            new_room = create_room(room_list, current_client, msgstruct.infos, buffer);
            if (new_room != NULL)
              room_list = new_room;
            break;

          case(MULTICAST_LIST): // command /channel_list
            channel_list(room_list, current_client, buffer);
            break;

          case(MULTICAST_QUIT): // command /quit <room>
            quit_room(room_list, current_client, msgstruct.infos,buffer);
            break;

          case(MULTICAST_JOIN): // command /join <room>
            join_room(room_list, current_client, msgstruct.infos, buffer);
            break;

          case(MULTICAST_SEND):
            msg_room(room_list, client_list, current_client, msgstruct.infos, buffer);
            break;

          case(FILE_REQUEST): // command /send <pseudo> <msg>
            msg_file_request(client_list, current_client, buffer, msgstruct.infos);
            // fds[i].revents = 0;
            break;

          case(FILE_ACCEPT):
            msg_file_accept(client_list, current_client, buffer, msgstruct.infos);
            break;

          case(FILE_REJECT):
            msg_file_reject(client_list, current_client, buffer, msgstruct.infos);
            break;

          case(ECHO_SEND):
            echo_reply(current_client, buffer);
            break;

          case(QUIT): // command /quit
            client_list = delete_client(client_list,fds[i].fd);
            printf(">>> Client %i successfully disconnected\n", fds[i].fd-3);
            close(fds[i].fd);
            fds[i].revents = 0;
            break;

          case(ERROR_LOGIN):
            error_login(current_client, buffer);
            break;

          case (ERROR_COMMAND):
            error_command(current_client, buffer);
            break;

          case (COLOR):
            color_client(current_client, buffer);
            break;

          case (MAN_COLOR):
            man_color_client(current_client, buffer);
            break;

          default:
            printf("SOMETHING IS WRONG\n");
            break;
        }
        memset(buffer, '\0', BUF_SIZE);
        memset(&msgstruct, 0, sizeof(struct message));

      }
    }
  }
}


int main(int argc, char* argv[]){
  if( argc != 2 ){
    printf("Usage : ./pgm #port\n");
    return 0;
  }
  // Set the port
  short port = atoi(argv[1]);
  // Initialize address
  struct sockaddr_in address = init_address(port);
  //Create client address
  int socket_listener = create_listener_socket(&address);
  // Server actions
  action(socket_listener);
  // Close connection
  close(socket_listener);

  exit(EXIT_SUCCESS);
}
