/*NAME: Sean Langley
ID: 504 661 838
EMAIL: sean.langley22@gmail.com
*/
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

static int shell_flag = 0;
int shell_process;
struct termios orig_term, new_term;
char cr_lf[2] = {0x0D, 0x0A};

void reset_terminal(void)
{
  tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
}

void set_noecho_mode(void)
{
  tcgetattr(STDIN_FILENO, &orig_term);
  atexit(reset_terminal);
  new_term = orig_term;
  new_term.c_lflag &= ~(ICANON | ECHO);
  tcsetattr (STDIN_FILENO, TCSANOW, &new_term);
}

void *read_into_buf(void *arg)
{
  int *pipe = (int *)arg;
  int numbytes;
  char buf[1];
  
  while(1)
    {
      numbytes = read(*pipe, buf, 1);
      if(*buf == 0x04)
	break;
      if(numbytes > 0)
	write(STDOUT_FILENO, buf, 1);
    }
  return NULL;
}

static void SIGPIPE_handler(int signo)
{
  printf("SIGPIPE; exiting with code 1.\n");
  exit(1);
}

void get_options(int argc, char **argv)
{
  int c;
  static struct option longopts[] = {
    {"shell", no_argument, &shell_flag, 's'},
    {0,0,0,0}
  };

  while(1)
    {
      int option_index = 0;
      c = getopt_long(argc, argv, "s", longopts, &option_index);
      if(c == -1)
	break;
    }
}


void regular_echo(char *buf)
{
  int numbytes;
  while(1)
    {
      numbytes = read(STDIN_FILENO, buf, 1);
      if(*buf == 0x04)
	break;
      if(numbytes > 0)
	{
	  if(*buf == 0x0D || *buf == 0x0A)
	    write(STDOUT_FILENO, cr_lf, 2);
	  else
	    write(STDOUT_FILENO, buf, 1);
	}
    }
}


int main(int argc, char **argv)
{
  get_options(argc, argv);
  int numbytes;
  char buf[1];
  set_noecho_mode();
  int pipe1[2];
  int pipe2[2];
  pid_t process_id;

  if(!shell_flag)
    regular_echo(buf);
  
  else
    {
      if(pipe(pipe1) == -1)
	{
	  perror("pipe1 failure");
	  exit(EXIT_FAILURE);
	}
      if(pipe(pipe2) == -1)
	{
	  perror("Pipe2 failure");
	  exit(EXIT_FAILURE);
	}

      process_id = fork();
      if(process_id == -1)
	{
	  perror("Fork failure.\n");
	  exit(EXIT_FAILURE);
	}
      if(process_id == 0)
	{
	  signal(SIGPIPE, SIGPIPE_handler);
	  close(pipe1[1]);
	  close(STDIN_FILENO);
	  dup(pipe1[0]);
	  close(pipe1[0]);
	  
	  close(STDOUT_FILENO);
	  dup(pipe2[1]);
	  close(STDERR_FILENO);
	  dup(pipe2[1]);
	  close(pipe2[1]);

	  char *args[] = {"/bin/bash", NULL};

	  execvp(args[0], args);
	}
      
      else
	{
	  shell_process = process_id;
	  
	  pthread_t my_thread;
	  close(pipe1[0]);
	  close(pipe2[1]);
	  pthread_create(&my_thread, NULL, read_into_buf, &pipe2[0]);
	  
	  while(1)
	    {
	      numbytes = read(STDIN_FILENO, buf, 1);
	      if(*buf == 0x04)
		{
		  printf("^D");
		  exit(0);
		}
	      if(*buf == 0x03)
		{
		  printf("^C");
		  kill(shell_process, SIGINT);
		}

	      if(numbytes > 0)
		{
		  write(STDOUT_FILENO, buf, 1);
		  write(pipe1[1], buf, 1);
		}
	    }
	}
    }
}



