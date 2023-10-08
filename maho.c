/***** Includes *****/


#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <termios.h> //The termios functions describe a general terminal interface that is provided to control asynchronous communications ports.
#include <ctype.h> // To control the printable characters
#include <errno.h>
#include <sys/ioctl.h> // for the window size
#include <string.h> 



/***** Defines *****/ 


// To define the qtrl Q as a quit 
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL,0}
#define MAHO_VERSION "0.0.1"


/***** Data *****/

// For the terminal size rows 
struct editorConfig {
  //for screen size ioctl request
  int cx,cy;
  int screenrows;
  int screencols;
  struct termios orig_termios;
};

struct editorConfig E;

struct abuf {
  char *b;
  int len;
};

enum editorKey {
  ARROW_LEFT = 1000,
  ARROW_RIGHT ,
  ARROW_UP,
  ARROW_DOWN,
  PAGE_UP,
  PAGE_DOWN
};


/***** Terminal *****/

// Some error handling stuff ...
void die(const char *s)
{
  // clear the screen when exit
  write (STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  // if anything went wrong error and run (exit).
  perror (s);
  exit(1);
}


// Disable the raw mode when finishing writing else it will not exit the section
void disableRawMode(){
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetarr");
}


// enabling raw mode that can display characters directly while writing, oppositely to canonical mode that waits for a specified  signal to print all typed characters. 
// Canonical mode is the default so we should enable the raw mode instead.
void enableRawMode()
{
  // read the current attributes into a struct
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
    die ("tcsetattr");

  atexit(disableRawMode) ; // after read disable and comback to canonical mode for signals use
  

  struct termios raw = E.orig_termios;
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



//Reading keypress in lowleveel
int editorReadKey()
{
  int nread;
  char c;
  while((nread = read(STDIN_FILENO, &c, 1)) != 1)
  {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }

  if (c == '\x1b')
  {
    char seq[3];

    if(read(STDIN_FILENO,&seq[0],1) != 1) return '\x1b';
    if(read(STDIN_FILENO,&seq[1],1) != 1) return '\x1b';

    if (seq[0] == '[')
    {
      if (seq[1] >= '0' && seq[1] <= '9')
      {
        if (read(STDIN_FILENO,&seq[2],1) != 1) return '\x1b';
        if(seq[2] == '~'){
          switch(seq[1]){
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
          }
        }
      }else {
          switch (seq[1])
          {
            case 'A': return ARROW_UP;
            case 'B': return ARROW_DOWN;
            case 'C': return ARROW_RIGHT;
            case 'D': return ARROW_LEFT;
          }
      }
  }
    return '\x1b';
  }else {
    return c;
  }
}



//Get Cursor position 
int getCursorPosition(int *rows,int *cols)
{
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO,"\x1b[6n",4 )!= 4)
    return -1;

   
  while (i < sizeof(buf) -1){
    if (read (STDIN_FILENO, &buf[i] ,1)!=1)
      break;
    if (buf[i] == 'R')
      break;
    i++;
  }
  buf[i] = '\0';

  if(buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2],"%d;%d",rows,cols) != 2) return -1;
  
  return 0;
}
 


//Get window size with IOCTL built in request: TIOCGWINSZ 
// Stands for Input/Output Control Get WINdow size

int getWindowSize(int *rows, int *cols)
{
  struct winsize ws;
  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)== -1 || ws.ws_col == 0)
  {
    //To move the cursor to the bottom-right corner
    //The 999 is to say the largest value to move it to the right ,The C to say move forward,the B is to say move it down.
    if (write(STDOUT_FILENO,"\x1b[999C\x1b[999B",12) != 12)
      return -1;
    return getCursorPosition (rows,cols);
  }else{  
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}



/***** Append buffer *****/ 

// For reallocating the memory block to the size of the current string and to copy it after the end of the current data in the buffer,later update the length and and the pointer of abuf with the new values
void abAppend(struct abuf *ab,const char *s,int len)
{
  char *new = realloc(ab ->b ,ab -> len + len);

  if (new == NULL) return ;
  memcpy(&new[ab ->len],s ,len);
  ab -> b = new;
  ab ->len += len;
}

void abFree(struct abuf *ab)
{
  free (ab ->b );
}


/***** Inputs *****/ 

//Cursor moves 
void editorMoveCursor(int key)
{
  switch(key){
    case ARROW_LEFT:
      if (E.cx > 0){
        E.cx--;
      }
      break;
    case ARROW_RIGHT:
      if (E.cx < E.screencols -1) E.cx++;
      break;
    case ARROW_UP:
      if (E.cy > 0)E.cy--;
      break;
    case ARROW_DOWN:
      if (E.cy < E.screenrows -1) E.cy ++;
      break;
  }
}

//Mapping the keypress to editor operations
void editorProcessKeypress()
{
  int c = editorReadKey();

  switch(c) 
  {
    case CTRL_KEY('q'):
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H" , 3);
      exit(0);
      break;

    case PAGE_UP:
    case PAGE_DOWN:
      {
        int times = E.screenrows;
        while (times--)
               editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      }
    break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editorMoveCursor(c);
      break;
  }
}




/***** Outputs *****/ 


//Adding tilde like VIM did ^^ 
void editorDrawRows(struct abuf *ab)
{
  int i;
  for (i = 0; i < E.screenrows; i++)
  {
    if (i == E.screenrows /3 ){
      char welcome[80];
      int welcomelen = snprintf(welcome,sizeof(welcome),"Maho text editor -- version %s",MAHO_VERSION);
      if (welcomelen > E.screencols) welcomelen = E.screencols;
      int padding = (E.screencols - welcomelen) / 2;
      if (padding){
        abAppend(ab,"~",1);
        padding--;
      }
      while (padding--) abAppend(ab," ",1);
      abAppend(ab,welcome,welcomelen);
    }else {
      abAppend(ab ,"~",1);
    }

  
    //Last line
    //K to erase the rest of the current line not all the screen
    abAppend(ab,"\x1b[k",3);
    if(i <E.screenrows -1)
      abAppend(ab,"\r\n",2);
  }
}


//clearing the screen
void editorRefreshscreen()
{
  struct abuf ab = ABUF_INIT;
  
  // The \x1b is the escape character
  abAppend(&ab ,"\x1b[?25l",6);
  
  // Position cursor at top-left corner
  abAppend( &ab,"\x1b[H", 3);
  //Draw rows
  editorDrawRows(&ab);

  //position the cursor at the current position
  char buf[32];
  snprintf(buf,sizeof(buf), "\x1b[%d;%dH",E.cy +1,E.cx +1);
  abAppend(&ab,buf,strlen(buf));
  
  //show cursor
  abAppend(&ab,"\x1b[?25h",6);

  //write the buffer's contents out to standard output
  write (STDOUT_FILENO,ab.b ,ab.len);
  abFree(&ab);
}




/***** Init *****/


void initEditor()
{
  E.cx = 0;
  E.cy = 0;

  if(getWindowSize(&E.screenrows, &E.screencols) == -1)
    die("getWindowSize");
}

int main ()
{
  // All the stuff did upside
  enableRawMode();
  initEditor();
  //Reading chars 
  while(1) {
  	char c = editorReadKey();
    // does c a control character ?
        if (iscntrl(c)) { 
            // print it 
            printf("%d\n", c);
        } else {
            // else to go back in a new line write the whole "\r\n"
            printf("%d ('%c')\r\n", c, c);
        }

    //Process the characters	  
    editorProcessKeypress();
    //Flush the screen
    editorRefreshscreen(); 
  }
  return 0;
}
