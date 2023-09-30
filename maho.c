#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <termios.h> //The termios functions describe a general terminal interface that is provided to control asynchronous communications ports.
#include <ctype.h> // To control the printable characters

struct termios orig_termios;
// Disable the raw mode when finishing writing else it will not exit the section
void disableRawMode(){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
// enabling raw mode that can display characters directly while writing, oppositely to canonical mode that waits for a specified  signal to print all typed characters. 
// Canonical mode is the default so we should enable the raw mode instead.
void enableRawMode()
{
  
  tcgetattr(STDIN_FILENO, &orig_termios); // to read the current attributes into a struct
  atexit(disableRawMode) ; // after read disable and comback to canonical mode for signals use
  
  struct termios raw = orig_termios;
  raw.c_iflag &= ~(ICRNL | IXON | BRKINT | INPCK | ISTRIP);  // disable CTRL+S and CTRL+Q and miscellaneous flags
  raw.c_oflag &= ~(OPOST); // disable all output processing like "\n"
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); // modifying the struct by hand 

 // The c_cc stands for "control characters" an array of bytes to control various terminal settings. 
  // Set the min number of bytes of input needed before read () can return.
  raw.c_cc[VMIN] = 0; 
  //  Set the max amount of time to wait before read() returns.
  raw.c_cc [VTIME] = 1;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // to write the new terminal attributes back out

}


int main ()
{
  // All the stuff did upside
  enableRawMode();

  // infinit loop till the user type 'q'
  char c = '\0';
  while(1) {
    
    // Reading ... 
    read(STDIN_FILENO, &c, 1);
    // does c a controle character ?
    if (iscntrl(c))  
    { 
      // print it 
      printf("%d\n", c);
    }else {
      //else to go back in a new line write the whole "\r\n"
      printf("%d ('%c')\r\n", c, c);
    }
    if (c == 'q')
      break;  
  }
  return 0;
}
