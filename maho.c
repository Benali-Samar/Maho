#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <termios.h> //The termios functions describe a general terminal interface that is provided to control asynchronous communications ports.
#include <ctype.h> // To control the printable characters

struct termios orig_termios;

void disableRawMode(){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode()
{
  
  tcgetattr(STDIN_FILENO, &orig_termios); // to read the current attributes into a struct
  atexit(disableRawMode) ; // used to disable raw mode function 
  
  struct termios raw = orig_termios;
  raw.c_iflag &= ~(ICRNL | IXON | BRKINT | INPCK | ISTRIP);  // disable CTRL+S and CTRL+Q and miscellaneous flags
  raw.c_oflag &= ~(OPOST); // disable all output processing like "\n"
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); // modifying the struct by hand 
  raw.c_cc[VMIN] = 0; 
  raw.c_cc [VTIME] = 1;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // to write the new terminal attributes back out

}


int main ()
{
  enableRawMode();

  
  while(1) {
  char c = '\0';
  read(STDIN_FILENO, &c, 1);
  if (iscntrl(c))  
  {
      printf("%d\n", c);
  }else {
      printf("%d ('%c')\n", c, c);
    }
  if (c == 'q') break;  
  }
  return 0;
}
