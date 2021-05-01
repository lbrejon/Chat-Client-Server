// #include "textcolor.h"

#define RESET		0
#define BRIGHT 		1
#define DIM		2
#define UNDERLINE 	3
#define BLINK		4
#define REVERSE		7
#define HIDDEN		8

#define BLACK 		0
#define RED		1
#define GREEN		2
#define YELLOW		3
#define BLUE		4
#define MAGENTA		5
#define CYAN		6
#define	WHITE		7

#define BUFSIZE 4096

int convert_color(char *buffer){
  if (!strcmp(buffer,"BLACK"))
    return BLACK;
  else if (!strcmp(buffer,"RED"))
    return RED;
  else if (!strcmp(buffer,"GREEN"))
    return GREEN;
  else if (!strcmp(buffer,"YELLOW"))
    return YELLOW;
  else if (!strcmp(buffer,"BLUE"))
      return BLUE;
  else if (!strcmp(buffer,"MAGENTA"))
    return MAGENTA;
  else if (!strcmp(buffer,"CYAN"))
        return CYAN;
  else if (!strcmp(buffer,"WHITE"))
    return WHITE;
  else
    return -1;

}

void textcolor(int attr, int fg, int bg){
  char command[BUFSIZE]; 	/* Command is the control command to the terminal */
	sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
	printf("%s", command);
}

void pimp_my_server(char * buffer, int attr, int fg, int bg){
  textcolor(attr, fg, bg);
	printf("[Server] : %s\n", buffer);
	textcolor(RESET, WHITE, BLACK);
}

// int main(){
//   textcolor(BRIGHT, YELLOW, BLACK);
// 	printf("Test\n");
// 	textcolor(RESET, WHITE, BLACK);
// 	return 0;
// }
