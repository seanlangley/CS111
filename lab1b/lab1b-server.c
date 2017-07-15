/*NAME: Sean Langley
EMAIL: sean.langley22@gmail.com
ID: 504 661 838*/
#include <unistd.h>
#include <termios.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <mcrypt.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>

int sockfd, clientfd, portno, numbytes;
socklen_t clilen;
struct sockaddr_in serv_addr, cli_addr;
const char cr_lf[2] = {0x0D, 0x0A};
int logfd;
int keyfd;
int encrypt_flag = 0;
MCRYPT td;

pid_t shellpid;
#define BUFSIZE 1024




static void sigpipe_handler()
{
  fprintf(stderr, "Error: Broken pipe\n");
  kill(shellpid, SIGINT);
  exit(1);
}


void *read_from_shell(void *pipefd)
{
  /*Read from the pipe */
  int bytenum;
  char buf[BUFSIZE];
  int *fd = (int *)pipefd;
  
  while(1)
    {
      bytenum = read(*fd, buf, BUFSIZE);
      if(bytenum > 0)
	{
	  if(encrypt_flag)
	    mcrypt_generic(td, buf, bytenum);

	  write(clientfd, buf, bytenum);	  
	}
      else
	exit(1);
    }
}

void closeMcrypt()
{
  mcrypt_generic_deinit(td);
  mcrypt_module_close(td);
}


int main(int argc, char **argv)
{

  int pipetoshell[2];
  int pipefromshell[2];


  /*get options*/
  int c;
  static struct option long_opts[] = 
    {
      {"encrypt", required_argument, NULL, 'e'},
      {"port", required_argument, NULL, 'p'},
      {0, 0, 0, 0}
    };
  
  while(1)
    {
      int option_index = 0; 
      c = getopt_long(argc, argv, "ep:", long_opts, &option_index);
      if(c == -1)
	break;
      switch(c)
	{
	case 'e':  //Encrypt flag
	  keyfd = open(optarg, O_RDONLY);
	  encrypt_flag = 1;
	  break;
	case 'p':
	  portno = atoi(optarg);
	  break;
	default:
	  fprintf(stderr, "Error: %s\n", strerror(errno));
	  exit(1);
	}
    }
  if(!portno)
    portno = 9942;



  if(encrypt_flag)
    {
      char key[128];
      /*KEYLEN IS 7 FOR NOW*/
      read(keyfd, key, 7);
      
      td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
      char *IV = malloc(mcrypt_enc_get_iv_size(td));
      memset(IV, 0, sizeof(char) * mcrypt_enc_get_iv_size(td));

      mcrypt_generic_init(td, key, 7, IV);
      atexit(closeMcrypt);
			
      
    }


  /*Open socket*/
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
    {
      fprintf(stderr, "Error opening socket\n");
      exit(1);
    }

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  
  /*Bind socket*/
  if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
      fprintf(stderr, "Error binding socket: %s.\n", strerror(errno));
      exit(1); 
    }

  /*listen and accept the client*/
  clilen = sizeof(cli_addr);
  listen(sockfd, 4);
  printf("Connecting to client...\n");
  
  clientfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if(clientfd < 0)
    {
      fprintf(stderr, "Error on accept: %s\n", strerror(errno));
      exit(1);
    }

  printf("Connected to client!\n");

  signal(SIGPIPE, sigpipe_handler);
  

  /*Creating pipes*/
  if(pipe(pipetoshell) < 0)
    {
      fprintf(stderr, "Error creating pipe to shell\n");
      exit(1);
    }

  if(pipe(pipefromshell) < 0)
    {
      fprintf(stderr, "Error creating pipe from shell\n");
      exit(1);
    }


  /*Fork a new process*/
  shellpid = fork();  
  if(shellpid < 0)
    {
      fprintf(stderr, "Error: %s\n", strerror(errno));
      exit(1);
    }

  
  /*Child process*/
  if(shellpid == 0)
    {
      /*close read end of pipe from shell and write end of pipe to shell*/
      close(pipefromshell[0]);
      close(pipetoshell[1]);
      
      /*Replace stdin with a pipe into the shell*/
      dup2(pipetoshell[0], STDIN_FILENO);
      /*Replace stdout with a pipe away from the shell*/
      dup2(pipefromshell[1], STDOUT_FILENO);
      /*Replace stderr with a pipe out of the shell*/
      dup2(pipefromshell[1], STDERR_FILENO);

      execlp("/bin/bash", "bin/bash", NULL);

    }

  /*Parent process*/
  else if(shellpid != 0)
    {

      /*Close the read end of the pipe to the shell and write end
	of the pipe from the shell*/
      close(pipetoshell[0]);
      close(pipefromshell[1]);


      /*Create a thread to read input from shell pipe*/
      pthread_t shellid;     
      pthread_create(&shellid, NULL, read_from_shell, &pipefromshell[0]);
      

      /*Read input from the socket and write it to the shell*/
      char buf[BUFSIZE];
      while(1)
	{
	  /*Read from the socket*/
	  numbytes = read(clientfd, buf, BUFSIZE);

	  int index;
	  for(index = 0; index < strlen(buf); index++)
	    {
	      /*^D*/
	      if(buf[index] == 0x04)	
		{
		  char *D = "^D";
		  write(clientfd, D, strlen(D));
		  close(pipetoshell[1]);
		}
	      /*^C*/
	      if(buf[index] == 0x03)
		{
		  char *C = "^C";
		  write(clientfd, C, strlen(C));
		  kill(shellid, SIGINT);
		}
	    }

	  if(numbytes <= 0)
	    {
	      exit(1);
	    }

	  /*Decrypt the message before sending it to the shell*/
	  if(encrypt_flag)
	    mdecrypt_generic(td, buf, numbytes);


	  /*Write to the shell*/
	  write(pipetoshell[1], buf, numbytes);
	  
	  
	}
    }

  exit(0);
}
