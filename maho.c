/***** Includes *****/


#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <termios.h> //The termios functions describe a general terminal interface that is provided to control asynchronous communications ports.
#include <ctype.h> // To control the printable characters
#include <errno.h>



/***** Defines *****/ 


// To define the qtrl Q as a quit 
#define CTRL_KEY(k) ((k) & 0x1f)



/***** Data *****/

struct termios orig_termios;

/***** Terminal *****/

// Some error handling stuff ...
void die(const char *s)
{
  // if anything went wrong error and run (exit).
  perror (s);
  exit(1);
}


// Disable the raw mode when finishing writing else it will not exit the section
void disableRawMode(){
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    die("tcsetarr");
}
// enabling raw mode that can display characters directly while writing, oppositely to canonical mode that waits for a specified  signal to print all typed characters. 
// Canonical mode is the default so we should enable the raw mode instead.
void enableRawMode()
{
  // read the current attributes into a struct
  if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
    die ("tcsetattr");

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
  
  // To write the new terminal attriobutes back out
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die ("tcsetattr");

}




/***** Init *****/

int main ()
{
  // All the stuff did upside
  enableRawMode();

  // infinit loop till the user type 'q'
  char c = '\0';
  while(1) {
    
    // Reading ... 
    if (read(STDIN_FILENO, &c, 1)== -1 && errno != EAGAIN)
      die ("read");
    // does c a controle character ?
    if (iscntrl(c))  
    { 
      // print it 
      printf("%d\n", c);
    }else {
      //else to go back in a new line write the whole "\r\n"
      printf("%d ('%c')\r\n", c, c);
    }
    if (c == CTRL_KEY('q'))
      break;  
  }
  return 0;
}
