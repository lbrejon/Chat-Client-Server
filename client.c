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
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"
#include "msg_struct.h"

#include "textcolor.h"

#define BUF_SIZE 4096

struct id_client{
  char pseudo[NICK_LEN];
  char room[NICK_LEN];
  char file_name_recv[NICK_LEN]; // username who will receive the file
  char file_name_to_transfer[NICK_LEN]; // name of transfered file
  char port[BUF_SIZE]; // port number used for sending/receiving a file
  char IP[BUF_SIZE];
  int new_socket; //socket used for sending/receiving a file
};

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
}

// Create client socket
int create_client_socket(){
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(sock_fd == -1){
      perror("Error while creating socket");
      exit(1);
  }
  return sock_fd;
}

// Create client address
struct sockaddr_in create_client_address(char * argv_1, char * argv_2){
  short port_number = (short) atoi(argv_2);
  char* ip_addr = argv_1;
  struct sockaddr_in server_addr;

  memset(&server_addr, '\0', sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port_number);
  inet_aton(ip_addr, &server_addr.sin_addr);

  return server_addr;
}

void client_connexion_to_server(int sock_fd, struct sockaddr_in server_addr){
  if (-1 == connect(sock_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr))){
    perror("Error while connecting");
    exit(2);
  }
}

// For connection peer_to_peer
struct sockaddr_in init_address(short port){ // Create server address
  struct sockaddr_in address;
  char* ip_address = "127.0.0.1";

  memset(&address, '\0', sizeof(struct sockaddr_in));
  address.sin_port = htons(port);
  inet_aton(ip_address, &address.sin_addr);
  address.sin_family = AF_INET;
  return address;
}

// For connection peer_to_peer
int create_listener_socket(struct sockaddr_in * address){
  int socket_listener = -1;
  if(-1 == (socket_listener = socket(AF_INET, SOCK_STREAM, 0))){
    perror("Error creating socket");
    exit(1);
  }
  if(-1 == bind(socket_listener, (struct sockaddr *)address, sizeof(*address))){
    perror("Error binding");
    exit(1);
  }
  if(-1 == listen(socket_listener, 1)){
      perror("Error while listening");
      return 1;
  }
  return socket_listener;
}

// For connection peer_to_peer
int accept_client(int socket_listener){
  struct sockaddr_in client_addr;
  int client_socket = -1;
  int socklen = sizeof(struct sockaddr_in);

  memset(&client_addr, '\0', sizeof(client_addr));

  if((client_socket = accept(socket_listener, (struct sockaddr *)&client_addr, (socklen_t *)&socklen)) == -1){
    perror("Error while accepting");
    return 1;
  }
  return client_socket;
}




struct message invalid_command(struct message msgstruct, struct id_client ID_client, char *buf){
  struct message msg;
  memset(&msg, 0, sizeof(struct message));

  if (!strcmp(buf,"Wrong answer")){ // a request has been sent
    memset(buf, '\0', BUF_SIZE);
    strcpy(buf,"INVALID COMMAND TO FILE REQUEST");
  }
  else if(!strcmp(buf, "INVALID COLOR")){
    memset(buf, '\0', BUF_SIZE);
    strcpy(buf,"INVALID COMMAND TO COLOR");
  }
  else{
    memset(buf, '\0', BUF_SIZE);
    strcpy(buf,"INVALID COMMAND");
  }
  msg.pld_len = strlen(buf);
  strcpy(msg.nick_sender,ID_client.pseudo);
  msg.type = ERROR_COMMAND;
  strcpy(msg.infos,"");

  return msg;
}


struct message error_login(struct message msgstruct, struct id_client ID_client, char *buf, char *login_test){
  struct message msg;
  memset(&msg, 0, sizeof(struct message));
  memset(buf, '\0', BUF_SIZE);

  strcpy(buf,login_test);
  msg.pld_len = strlen(buf);
  strcpy(msg.nick_sender,ID_client.pseudo);
  msg.type = ERROR_LOGIN;
  strcpy(msg.infos,"");

  return msg;
}


struct message nick(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  char login[NICK_LEN];
  char login_test[NICK_LEN];
  const char parameter2[] = "";
  const char parameter3[] = " ,;:!?./§^$£&~#(){}[]+-*@";

  struct message msg;
  memset(&msg, 0, sizeof(struct message));

  memset(login, '\0', NICK_LEN);
  memset(login_test, '\0', NICK_LEN);

  if ( (strlen(buf)-strlen(command) == 1) || (strlen(buf)-strlen(command) == 2) )
    return invalid_command(msgstruct, ID_client, buf);
  else{
    command = strtok(NULL, parameter2);

    strncpy(login,command, strlen(command));
    strncpy(login_test, strtok (command,parameter3), strlen(command));

    if(strcmp(login,login_test) || (strlen(login) > NICK_LEN)){  // invalid login
      return error_login(msgstruct, ID_client, buf, login_test);
    }
    else{ //valid login
      memset(buf, '\0', BUF_SIZE);
      strcpy(buf,login);
      msg.pld_len = strlen(buf);
      strcpy(msg.nick_sender, ID_client.pseudo);
      msg.type = NICKNAME_NEW;
      strcpy(msg.infos,login);
    }
  }
  return msg;
}

struct message quit(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  char buffer[BUF_SIZE];
  char login[NICK_LEN];
  const char parameter2[] = "";
  struct message msg;

  memset(&msg, 0, sizeof(struct message));
  memset(buffer, '\0', BUF_SIZE);
  memset(login, '\0', NICK_LEN);
  strncpy(buffer, buf, strlen(buf)-1);

  if (strlen(ID_client.room) == 0){ // case where user wants to get disconnected
    if (strlen(buf)-strlen(command) == 1){ // valid command
      msg.pld_len = strlen(buffer);
      strcpy(msg.nick_sender,msgstruct.nick_sender);
      msg.type = QUIT;
      strcpy(msg.infos,"");
    }
    else // invalid command
      return invalid_command(msgstruct, ID_client, buf);
  }
  else{ // case where user wants to quit a room
    // printf("ID_client.room '%s'\n", ID_client.room);
    if (strlen(buf)-strlen(command) > 2){
      command = strtok(NULL, parameter2);
      strncpy(login,command, strlen(command));
      memset(buf, '\0', BUF_SIZE);
      strcpy(buf,login);
      msg.pld_len = strlen(buf);
      strcpy(msg.nick_sender,ID_client.pseudo);
      msg.type = MULTICAST_QUIT;
      strcpy(msg.infos,buf);
      }
    else // invalid command
      return invalid_command(msgstruct, ID_client, buf);
  }
  return msg;
}

struct message help(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  struct message msg;
  memset(&msg, 0, sizeof(struct message));

  if (strlen(buf)-strlen(command) == 1){ // valid command
    memset(buf, '\0', BUF_SIZE);
    strcpy(buf,"The list of commands is :");
    strcat(buf,"\n              /nick <pseudo> : to set/change your pseudo");
    strcat(buf,"\n              /who : to know the pseudo of connected users");
    strcat(buf,"\n              /whois <pseudo> : to get informations about <pseudo>'s connection");
    strcat(buf,"\n              /msg <pseudo> <msg> : to send a message <msg> to <pseudo>");
    strcat(buf,"\n              /msgall <msg> : to send a message <msg> to everyone\n");
    strcat(buf,"\n              /create <room> : to create and join the new room <room>");
    strcat(buf,"\n              /join <room> : to join the room <room>");
    strcat(buf,"\n              /quit <room> : to quit the room <room>");
    strcat(buf,"\n              /channel_list : to see all existant room\n");
    strcat(buf,"\n              /send <pseudo> <file> : to send the file <file> to <pseudo>\n");
    strcat(buf,"\n              /quit : to quit the chat if you are not in a room\n");
    strcat(buf,"\n              /color <COLOR> : to pimp your client interface in color <COLOR>");
    strcat(buf,"\n              /man_color : to check the manual of the command /color");
    msg.pld_len = strlen(buf);
    strcpy(msg.nick_sender,ID_client.pseudo);
    msg.type = ECHO_SEND;
    strcpy(msg.infos,"");
  }
  else //invalid command
    return invalid_command(msgstruct, ID_client, buf);

  return msg;
}

struct message who(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  struct message msg;
  memset(&msg, 0, sizeof(struct message));

  if (strlen(buf)-strlen(command) == 1){ // valid command
    msg.pld_len = strlen(buf)-1;
    strcpy(msg.nick_sender,ID_client.pseudo);
    msg.type = NICKNAME_LIST;
    strcpy(msg.infos,"");
  }
  else //invalid command
    return invalid_command(msgstruct, ID_client, buf);

  return msg;
}

struct message whois(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  char login[NICK_LEN];
  const char parameter2[] = "";
  struct message msg;

  memset(&msg, 0, sizeof(struct message));
  memset(login, '\0', NICK_LEN);

  if (strlen(buf)-strlen(command) > 2){ // invalid command
    command = strtok(NULL, parameter2);
    strncpy(login,command, strlen(command));
    memset(buf, '\0', BUF_SIZE);
    strcpy(buf,login);
    msg.pld_len = strlen(buf);
    strcpy(msg.nick_sender,ID_client.pseudo);
    msg.type = NICKNAME_INFOS;
    strcpy(msg.infos,buf);
  }

  else // invalid command
    return invalid_command(msgstruct, ID_client, buf);

  return msg;
}



struct message msgall(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  char login[NICK_LEN];
  const char parameter2[] = "";
  struct message msg;

  memset(&msg, 0, sizeof(struct message));
  memset(login, '\0', NICK_LEN);

  if (strlen(buf)-strlen(command) > 2){ // valid command
    command = strtok(NULL, parameter2);
    strncpy(login,command, strlen(command));
    memset(buf, '\0', BUF_SIZE);
    strcpy(buf,login);
    msg.pld_len = strlen(buf);
    strcpy(msg.nick_sender,ID_client.pseudo);
    msg.type = BROADCAST_SEND;
    strcpy(msg.infos,"");
  }
  else // invalid command
    return invalid_command(msgstruct, ID_client, buf);

  return msg;
}



struct message msgspe(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  char login[NICK_LEN];
  char login_test[NICK_LEN];
  const char parameter[] = " ";
  const char parameter2[] = "";
  struct message msg;

  memset(&msg, 0, sizeof(struct message));
  memset(login, '\0', NICK_LEN);
  memset(login_test, '\0', NICK_LEN);

  if ( (strlen(buf)-strlen(command) == 1) || (strlen(buf)-strlen(command) == 2) ){ // invalid command
    return invalid_command(msgstruct, ID_client, buf);
  }
  else{
    command = strtok(NULL, parameter2);
    if (strlen(command) == 0)
      return invalid_command(msgstruct, ID_client, buf);
    else{
      strcpy(login, strtok( command, parameter));
      strcpy(login_test, strtok( NULL, parameter2));

      if(strlen(login) == 0 || strlen(login_test) == 0){ // invalid login
        return invalid_command(msgstruct, ID_client, buf);
      }
      else{ //valid login
        memset(buf, '\0', BUF_SIZE);
        strcpy(buf,login_test);
        msg.pld_len = strlen(buf);
        strcpy(msg.nick_sender, ID_client.pseudo);
        msg.type = UNICAST_SEND;
        strcpy(msg.infos,login);
      }
    }
  }
  return msg;
}



struct message create_room(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  char login[NICK_LEN];
  char login_test[NICK_LEN];
  const char parameter2[] = "";
  const char parameter3[] = " ,;:!?./§^$£&~#(){}[]+-*@";
  struct message msg;

  memset(&msg, 0, sizeof(struct message));
  memset(login, '\0', NICK_LEN);
  memset(login_test, '\0', NICK_LEN);

  if ( (strlen(buf)-strlen(command) == 1) || (strlen(buf)-strlen(command) == 2) ){
    return invalid_command(msgstruct, ID_client, buf);
  }
  else{
    command = strtok(NULL, parameter2);

    strncpy(login,command, strlen(command));
    strncpy(login_test, strtok ( command, parameter3 ), strlen(command));

    if(strcmp(login,login_test) || (strlen(login) > NICK_LEN))// invalid login
      return invalid_command(msgstruct, ID_client, buf);

    else{ //valid login
      memset(buf, '\0', BUF_SIZE);
      strcpy(buf,login);
      msg.pld_len = strlen(buf);
      strcpy(msg.nick_sender, ID_client.pseudo);
      msg.type = MULTICAST_CREATE;
      strcpy(msg.infos,login);
    }
  }
  return msg;
}


struct message channel_list(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  struct message msg;
  memset(&msg, 0, sizeof(struct message));

  if (strlen(buf)-strlen(command) == 1){
    msg.pld_len = strlen(buf)-1;
    strcpy(msg.nick_sender,ID_client.pseudo);
    msg.type = MULTICAST_LIST;
    strcpy(msg.infos,"");
  }
  else{ //invalid command
    return invalid_command(msgstruct, ID_client, buf);
  }
  return msg;
}

struct message join_room(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  char login[NICK_LEN];
  char login_test[NICK_LEN];
  const char parameter2[] = "";
  const char parameter3[] = " ,;:!?./§^$£&~#(){}[]+-*@";
  struct message msg;

  memset(&msg, 0, sizeof(struct message));
  memset(login, '\0', NICK_LEN);
  memset(login_test, '\0', NICK_LEN);

  if ( (strlen(buf)-strlen(command) == 1) || (strlen(buf)-strlen(command) == 2) ){
    return invalid_command(msgstruct, ID_client, buf);
  }
  else{
    command = strtok(NULL, parameter2);

    strncpy(login,command, strlen(command));
    strncpy(login_test, strtok ( command, parameter3 ), strlen(command));

    if(strcmp(login,login_test) || (strlen(login) > NICK_LEN)){ // invalid login
      return invalid_command(msgstruct, ID_client, buf);
    }
    else{ //valid login
      memset(buf, '\0', BUF_SIZE);
      strcpy(buf,login);
      msg.pld_len = strlen(buf);
      strcpy(msg.nick_sender, ID_client.pseudo);
      msg.type = MULTICAST_JOIN;
      strcpy(msg.infos,login);
    }
  }
  return msg;
}

struct message file_request(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  char login[NICK_LEN];
  char login_test[NICK_LEN];
  const char parameter[] = " ";
  const char parameter2[] = "";
  struct message msg;

  memset(&msg, 0, sizeof(struct message));
  memset(login, '\0', NICK_LEN);
  memset(login_test, '\0', NICK_LEN);

  if ( (strlen(buf)-strlen(command) == 1) || (strlen(buf)-strlen(command) == 2) ){ // invalid command
    return invalid_command(msgstruct, ID_client, buf);
  }
  else{
    command = strtok(NULL, parameter2);
    if (strlen(command) == 0)
      return invalid_command(msgstruct, ID_client, buf);
    else{
      strcpy(login, strtok( command, parameter));
      strcpy(login_test, strtok( NULL, parameter2));

      if(strlen(login) == 0 || strlen(login_test) == 0){ // invalid login
        return invalid_command(msgstruct, ID_client, buf);
      }
      else{ //valid login
        memset(buf, '\0', BUF_SIZE);
        strcpy(buf,login_test);
        msg.pld_len = strlen(buf);
        strcpy(msg.nick_sender, ID_client.pseudo);
        msg.type = FILE_REQUEST;
        strcpy(msg.infos,login);
      }
    }
  }
  return msg;
}


struct message answer_file_request(struct message msgstruct, struct id_client ID_client, char *buf){
  struct message msg;
  char buffer[BUF_SIZE];

  // struct sockaddr_in new_address;
  // int new_socket;
  int new_port = 8090;

  memset(&msg, 0, sizeof(struct message));
  memset(buffer, '\0', BUF_SIZE);
  strncpy(buffer, buf, strlen(buf)-1);

  if (!strcmp(buffer, "Yes")){
    memset(buf, '\0', BUF_SIZE);
    sprintf(buf,"%s:%d",ID_client.IP,new_port);
    msg.pld_len = strlen(buf);
    strcpy(msg.nick_sender, ID_client.pseudo);
    msg.type = FILE_ACCEPT;
    strcpy(msg.infos,ID_client.file_name_recv);
    strcpy(ID_client.file_name_recv, "");

    // new_address = init_address(new_port);
    // new_socket = create_listener_socket(&new_address);
    // printf("ADDRESS CREATED\n");
    // accept_client(new_socket);
    // printf("ACCEPT CLIENT\n");


  }
  else if (!strcmp(buffer, "No")){
    memset(buf, '\0', BUF_SIZE);
    strcpy(buf, ID_client.pseudo);
    strcat(buf, " cancelled file transfer");
    msg.pld_len = strlen(buf);
    strcpy(msg.nick_sender, ID_client.pseudo);
    msg.type = FILE_REJECT;
    strcpy(msg.infos,ID_client.file_name_recv);
    strcpy(ID_client.file_name_recv, "");
  }
  else{
    memset(buf, '\0', BUF_SIZE);
    strcpy(buf,"Wrong answer");
    return invalid_command(msgstruct, ID_client, buf);
  }

  return msg;
}

void send_file_peer_to_peer(struct id_client ID_client, char *buffer, char *name){
  const char parameter[] = ":";
  char IP_recv[BUF_SIZE];
  char port_recv[BUF_SIZE];

  // struct sockaddr_in addr_recv;
  // int sock_recv;
  int fd_open = -1;
  struct message msgstruct;

  memset(IP_recv, '\0', BUF_SIZE);
  memset(port_recv, '\0', BUF_SIZE);
  memset(&msgstruct, 0, sizeof(struct message));

  //get @IP and port number from receiver
  strcpy(IP_recv, strtok(buffer, parameter));
  strcpy(port_recv,strtok(NULL, parameter));

  // sending_file;
  fd_open = open(ID_client.file_name_to_transfer, O_RDONLY);

  if (fd_open == -1)
    printf("File '%s' cannot be opened\n", ID_client.file_name_to_transfer);
  else{
    printf("File '%s' opened\n", ID_client.file_name_to_transfer);

    // read from file
    memset(buffer,'\0', BUF_SIZE);
    read(fd_open, buffer, BUF_SIZE);
    // printf("BUFFER lu '%s' après ouverture fichier '%s'\n", buffer, ID_client.file_name_to_transfer);

    // connection peer_to_peer
    // sock_recv = create_client_socket();
    // addr_recv = create_client_address(IP_recv, port_recv);
    // client_connexion_to_server(sock_recv, addr_recv); // same function por peer_to_peer
    // printf("Connection peer_to_peer done !\n");

    //send to client
    // strcpy(msgstruct.nick_sender,ID_client.pseudo);
    // msgstruct.type = FILE_SEND;
    // strcpy(msgstruct.infos,ID_client.file_name_to_transfer);
    // msgstruct.pld_len = strlen(buffer);
    // send_msg(new_socket, msgstruct, buffer);
  }
  close(fd_open);
}



struct message msg_room(struct message msgstruct, struct id_client ID_client, char *buf){
  struct message msg;
  memset(&msg, 0, sizeof(struct message));

  msg.pld_len = strlen(buf)-1;
  strcpy(msg.nick_sender, ID_client.pseudo);
  msg.type = MULTICAST_SEND;
  strcpy(msg.infos,ID_client.room);

  return msg;
}


struct message echo_reply(struct message msgstruct, struct id_client ID_client, char *buf){
  struct message msg;
  memset(&msg, 0, sizeof(struct message));

  msg.pld_len = strlen(buf)-1;
  strcpy(msg.nick_sender,ID_client.pseudo);
  msg.type = ECHO_SEND;
  strcpy(msg.infos,"");

  return msg;
}

struct message please_set_username(struct message msgstruct, struct id_client ID_client, char *buf){
  struct message msg;
  memset(&msg, 0, sizeof(struct message));

  msg.pld_len = strlen(buf)-1;
  strcpy(msg.nick_sender,ID_client.pseudo);
  msg.type = WELCOME;
  strcpy(msg.infos,"");

  return msg;
}

struct message press_enter(struct message msgstruct, struct id_client ID_client, char *buf){
  struct message msg;
  memset(&msg, 0, sizeof(struct message));

  msg.pld_len = strlen(buf);
  strcpy(msg.nick_sender,ID_client.pseudo);
  msg.type = ECHO_SEND;
  strcpy(msg.infos,"");

  return msg;
}


struct message color(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  char login[NICK_LEN];
  const char parameter2[] = "";
  struct message msg;

  memset(&msg, 0, sizeof(struct message));
  memset(login, '\0', NICK_LEN);

  if (strlen(buf)-strlen(command) > 2){
    command = strtok(NULL, parameter2);
    strncpy(login,command, strlen(command));
    memset(buf, '\0', BUF_SIZE);
    strcpy(buf,login);
    msg.pld_len = strlen(buf);
    strcpy(msg.nick_sender,ID_client.pseudo);
    msg.type = COLOR;
    strcpy(msg.infos,buf);
    }

  else{ // invalid command
    memset(buf, '\0', BUF_SIZE);
    strcpy(buf, "INVALID COLOR");
    return invalid_command(msgstruct, ID_client, buf);
  }
  return msg;
}


struct message manual_color(struct message msgstruct, struct id_client ID_client, char *buf, char *command){
  struct message msg;
  memset(&msg, 0, sizeof(struct message));

  if (strlen(buf)-strlen(command) == 1){
    msg.pld_len = strlen(buf)-1;
    strcpy(msg.nick_sender,ID_client.pseudo);
    msg.type = MAN_COLOR;
    strcpy(msg.infos,"");
  }
  else{ //invalid command
    return invalid_command(msgstruct, ID_client, buf);
  }
  return msg;
}


struct message command(char *buf, struct message msgstruct, struct id_client ID_client){
  char buffer[BUF_SIZE];
  const char parameter[] = " ";

  memset(buffer, '\0', BUF_SIZE);
  strncpy(buffer, buf, strlen(buf)-1);

  char *command = strtok(buffer, parameter);

  if (!strcmp(buf,"\n")) // case where user push "Enter"
    return press_enter(msgstruct, ID_client, buf);

  else if (!strcmp(command,"/nick"))
    return nick(msgstruct, ID_client, buf, command);

  else if (!strcmp(command,"/quit"))
    return quit(msgstruct, ID_client, buf, command);

  if (strcmp(ID_client.pseudo,"\0")){ // if user has a pseudo
    if (!strcmp(command,"/help"))
      return help(msgstruct, ID_client, buf, command);

    else if (!strcmp(command,"/who"))
      return who(msgstruct, ID_client, buf, command);

    else if (!strcmp(command,"/whois"))
      return whois(msgstruct, ID_client, buf, command);

    else if (!strcmp(command,"/msgall"))
      return msgall(msgstruct, ID_client, buf, command);

    else if (!strcmp(command,"/msg"))
      return msgspe(msgstruct, ID_client, buf, command);

    else if (!strcmp(command,"/create"))
      return create_room(msgstruct, ID_client, buf, command);

    else if (!strcmp(command,"/channel_list"))
      return channel_list(msgstruct, ID_client, buf, command);

    else if (!strcmp(command,"/join"))
      return join_room(msgstruct, ID_client, buf, command);

    else if (!strcmp(command,"/color"))
      return color(msgstruct, ID_client, buf, command);

    else if (!strcmp(command,"/man_color"))
      return manual_color(msgstruct, ID_client, buf, command);

    else if (!strcmp(command,"/send"))
      return file_request(msgstruct, ID_client, buf, command);

    else if (strlen(ID_client.file_name_recv) != 0)
      return answer_file_request(msgstruct, ID_client, buf);

    else if(strlen(ID_client.room) != 0)
      return msg_room(msgstruct, ID_client, buf);

    else // case where server echo-reply
      return echo_reply(msgstruct, ID_client, buf);
  }

  else // case where user hasn't a pseudo
    return please_set_username(msgstruct, ID_client, buf);
}


void interface(struct id_client ID_client){
  if (strcmp(ID_client.pseudo,"") && !strcmp(ID_client.room,"")){
    printf("[%s] : ", ID_client.pseudo);
    fflush(0);
  }
  else if(strcmp(ID_client.pseudo,"") && strcmp(ID_client.room,"")){
    printf("[%s][%s] : ", ID_client.pseudo,ID_client.room);
    fflush(0);
  }
}


// void action(int sock_fd, int sock_fd_recv, int sock_fd_send){
void action(int sock_fd){

  int nds = 2;
  int ret = -1;
  int rec1, rec2;
  struct pollfd fds[nds];
  char buff[BUF_SIZE];
  char buf[BUF_SIZE];
  struct message msgstruct;
  struct id_client ID_client;

  memset(fds, 0, nds*sizeof(struct pollfd)); // en soit, il n'y a qu'un serveur

  fds[0].fd = 0; // STDIN
  fds[0].events = POLLIN;
  fds[1].fd = sock_fd;
  fds[1].events = POLLIN;

  // fds[2].fd = sock_fd_recv;
  // fds[2].events = POLLIN;
  // fds[3].fd = sock_fd_send;
  // fds[3].events = POLLIN;

  // textcolor(RESET, WHITE, BLACK);
  printf("Connecting to server... done !\n");

  memset(buf, '\0', BUF_SIZE);
  memset(buff, '\0', BUF_SIZE);
  memset(&msgstruct, 0, sizeof(struct message));
  memset(&ID_client, 0, sizeof(struct id_client));

  strcpy(ID_client.IP,"127.0.0.1");

  while(1){


    ret = poll(fds,2,-1);
    if(ret == -1){
      perror("Error while poll");
      exit(1);
    }

    if (fds[0].revents & POLLIN){
      // Receiving structure + message
      memset(&msgstruct, 0, sizeof(struct message));
      strcpy(msgstruct.nick_sender, ID_client.pseudo);

      rec2 = read(fds[0].fd, buf, BUF_SIZE);
      if (rec2 == -1){
        perror("Error while receiving message\n");
        break;
      }
      if (rec2 == 0){
        close(fds[0].fd);
        close(fds[1].fd);
        exit(1);
      }

      msgstruct = command(buf, msgstruct, ID_client);

      if (msgstruct.type == FILE_REQUEST){
        strcpy(ID_client.file_name_to_transfer,buf);
      }

      // Sending structure + message
      send_msg(fds[1].fd, msgstruct, buf);

      // printf("Structure sent : pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
      // printf("Message sent : %s\n", buf);

      memset(buf, '\0', BUF_SIZE);
      fds[0].revents = 0;
      fds[1].revents = 0;

      if (msgstruct.type == QUIT){
          printf(">>> Client successfully disconnected because command '/quit' sent\n");
          close(fds[0].fd);
          close(fds[1].fd);
          exit(1);
      }

    }


    else if (fds[1].revents & POLLIN){
      memset(&msgstruct, 0, sizeof(struct message));

      rec1 = recv(fds[1].fd, &msgstruct, sizeof(struct message), 0);
      if (rec1 == -1){
        perror("Error while receiving structure\n");
        break;
      }
      rec2 = recv(fds[1].fd, buff, msgstruct.pld_len, 0);
      if (rec2 == -1){
        perror("Error while receiving message\n");
        break;
      }
      if (rec1 == 0 || rec2 == 0){
        printf(">>> Client successfully disconnected because server used CTRL+C\n");
        close(fds[0].fd);
        close(fds[1].fd);
        exit(1);
      }
      // printf("\nStructure received : next msg pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);

      switch(msgstruct.type){
        case(WELCOME):
          printf("[Server] : %s\n", buff);
          break;

        case(NICKNAME_NEW): // command /nick <pseudo>
          memset(ID_client.pseudo, '\0', NICK_LEN);
          strcpy(ID_client.pseudo, msgstruct.infos);
          printf("[Server] : %s\n", buff);
          break;

        case(NICKNAME_LIST): // command /who
          printf("[Server] : %s\n", buff);
          break;

        case(NICKNAME_INFOS): // command /whois <pseudo>
          printf("[Server] : %s\n", buff);
          break;

        case(BROADCAST_SEND): // command /msgall <message>
          printf("[%s] : %s\n", msgstruct.nick_sender, buff);
          break;

        case(UNICAST_SEND): // command /msg <pseudo> <message>
          printf("[%s] : %s\n", msgstruct.nick_sender, buff); // users has received a /msg
          printf("[%s] : \n", ID_client.pseudo);
          break;

        case(MULTICAST_CREATE): // comand /create <room>
          memset(ID_client.room, '\0', NICK_LEN);
          strcpy(ID_client.room, msgstruct.infos);
          printf("[%s] : You have created channel %s\n", ID_client.pseudo,ID_client.room);
          printf("[%s][%s] : %s\n", ID_client.pseudo,ID_client.room,buff);
          break;

        case(MULTICAST_JOIN): // command /join <room>
          strcpy(ID_client.room,msgstruct.infos);
          printf("[%s][%s] : %s\n",ID_client.pseudo, ID_client.room, buff);
          printf("[%s][%s] : \n",ID_client.pseudo, ID_client.room);
          break;

        case(MULTICAST_SEND):
          printf("[%s][%s] : %s\n",msgstruct.nick_sender,ID_client.room, buff);
          break;

        case(MULTICAST_LIST): // command /channel_list
          printf("[Server] : %s\n", buff);
          break;

        case(QUIT_CHANNEL): // command /quit <room>
          printf("[%s] : %s\n",ID_client.pseudo, buff);
          strcpy(ID_client.room,"");
          break;

        case(FILE_REQUEST): // command /send <pseudo> <file name>
          strcpy(ID_client.file_name_recv, msgstruct.nick_sender);
          printf("[Server] : %s\n", buff);
          break;

        case(FILE_ACCEPT):
          printf("[Server] : %s accepted file transfer\n", msgstruct.nick_sender);
          printf("[Server] : Connecting to %s and sending the file '%s'..\n", msgstruct.nick_sender, ID_client.file_name_to_transfer);
          // printf("IP:port de '%s' is %s\n", msgstruct.nick_sender, buff);
          send_file_peer_to_peer(ID_client, buff, msgstruct.nick_sender); // send file to msgstruct.nick_sender
          break;

        case(FILE_REJECT):
          strcpy(ID_client.file_name_recv, "");
          printf("[Server] : %s\n", buff);
          break;

        case(ECHO_SEND):
          printf("[Server] : %s\n", buff);
          break;

        case(ERROR_QUIT_CHANNEL):
          printf("[%s][%s] : %s\n",ID_client.pseudo, ID_client.room, buff);
          printf("[%s][%s] : \n",ID_client.pseudo, ID_client.room);
          break;

        case(ERROR_MULTICAST_JOIN):
          printf("[Server] : %s\n", buff);
          break;

        case(SERVER):
          printf("[Server] : %s\n", buff);
          break;

        case(COLOR):
          if (!strcmp(msgstruct.infos,"RESET"))
            textcolor(RESET, WHITE, BLACK);
          else
            textcolor(BRIGHT, convert_color(msgstruct.infos), BLACK);
          printf("[Server] : %s\n", buff);
          break;

        case(MAN_COLOR):
          printf("[Server] : %s\n", buff);
          break;

        default:
          printf("[SOMETHING WRONG] : %s\n", buff);
          break;
      }
      interface(ID_client);

      memset(buff, '\0', BUF_SIZE);
    }
  }
}


int main(int argc, char* argv[]){
  if( argc != 3 ){
    printf("Usage : /pgm ip_addr #port\n");
    return 0;
  }

  //Create client socket
  int sock_fd = create_client_socket();

  //Create client sockets for peer-to-peer connection
  // int sock_fd_recv = create_client_socket();
  // int sock_fd_send = create_client_socket();

  //Create client address
  struct sockaddr_in server_addr = create_client_address(argv[1], argv[2]);

  //Connection to server
  client_connexion_to_server(sock_fd, server_addr);

  // Client actions
  action(sock_fd);

  // Close connection
  close(sock_fd);
  // close(sock_fd_recv);
  // close(sock_fd_send);

  return 0;
}
