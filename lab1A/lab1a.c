#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/wait.h>


static int shell_flag = 0;

void get_options(int argc, char *argv[])
{
  int c; 
  while(1)
    {
      static struct option long_options[] = 
	{
	  {"shell", no_argument, &shell_flag, 1},
	  {0, 0, 0, 0}
	};
      int option_index = 0;
      c = getopt_long(argc, argv, "", long_options, &option_index);
      if(c == -1)
	break;
      switch(c)
	{
	case 0:
	  break;
	default:
	  fprintf(stderr, "Error: %s\n", strerror(errno));
	  exit(1);
	}
    }
}

void reset_terminal(struct termios orig_termios)
{
  tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

void set_noncanon_mode(struct termios termios_p)
{
  termios_p.c_iflag = ISTRIP;
  termios_p.c_oflag = 0;
  termios_p.c_lflag = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &termios_p);
}

void read_into_buf(char *buf)
{
  int index = 0;
  while(1)
    {

      if(read(STDIN_FILENO, buf + index, 1) < 0){
	  fprintf(stderr, "Error: %s\n", strerror(errno));
	  exit(1);
      }
      if(write(STDOUT_FILENO, buf+index, 1) < 0){
	fprintf(stderr, "Error: %s\n", strerror(errno));
	exit(1);
      }

      if(buf[index] == EOF)
	break;
      index++;
      if(index >= 15)
	break;
      
    }
}

void create_new_process()
{ 
  int pipefd[2];
  
  if(pipe(pipefd) == -1)
    {
      fprintf(stderr, "Error: %s\n", strerror(errno));
      exit(1);
    }
  
  int rc = fork();
  if(rc < 0)    //Fork Failed
    {
      fprintf(stderr, "Error: %s\n", strerror(errno));
      exit(1);
    }
  else if(rc == 0) //Child process has rc = 0
    {
      char *myargs[2];
      myargs[0] = strdup("/bin/bash");
      myargs[1] = NULL;
      execv(myargs[0], myargs);
    }
  else //Parent goes here
    {
      int wc = wait(NULL);
    }
}

int main(int argc, char *argv[])
{

  struct termios termios_p;
  struct termios orig_termios;
  get_options(argc, argv);
    
  /*get original terminal settings and save another copy of original
    terminal settings so we can restore later */
  tcgetattr(STDIN_FILENO, &termios_p);
  orig_termios = termios_p;    
  set_noncanon_mode(termios_p);

  /*If the --shell argument is used, run a subroutine that forks a 
    new process and creates a new shell with /bin/bash*/
  if(shell_flag)    
    create_new_process();

  char buf[256]; 
  read_into_buf(buf);



  reset_terminal(orig_termios);
  exit(0);
}
