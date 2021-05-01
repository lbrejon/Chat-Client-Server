#include "textcolor.c"

// Convert color into a integer
int convert_color(char *buffer);

// Change the color of the text
void textcolor(int attr, int fg, int bg);

// Pimp the server with a color and letter cas
void pimp_my_server(char * buffer, int attr, int fg, int bg);
