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
#include <sys/stat.h>

int portno;
static int logfd;
struct termios orig_term, new_term;
char cr_lf[2] = {0x0D, 0x0A};
#define BUFSIZE 1024
int log_flag = 0;
int encrypt_flag = 0;
int keyfd;
int sockfd;
MCRYPT td;

/*So I can pass multiple file descriptors into the new thread*/
struct descriptors {
  int sockdesc;
  int logdesc;
};





void reset_terminal(void)
{
  
  tcsetattr(STDIN_FILENO, TCSANOW, &orig_term);
  close(sockfd);
}


void set_terminal(void)
{
  tcgetattr(STDIN_FILENO, &orig_term);
  new_term = orig_term;
  new_term.c_lflag &= ~(ICANON | ECHO);
  new_term.c_cc[VMIN] = 1;
  new_term.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

  atexit(reset_terminal);
}

void closemcrypt()
{
  mcrypt_generic_deinit(td);
  mcrypt_module_close(td);
}


void *read_from_server(void *mystruct)
{
  struct descriptors *descs = (struct descriptors *) mystruct;
  int newlogfd = descs->logdesc;
  int *serverfd = &(descs->sockdesc);
  int numbytes;
  int logsize;
  char buf[BUFSIZE + 1];
  char logbuf[BUFSIZE];
  
  while(1)
    {
      numbytes = read(*serverfd, buf, BUFSIZE);
      buf[numbytes] = '\0';
      if(numbytes <= 0)
	{
	  exit(1);
	}
 
      if(log_flag)
	{

	  logsize = sprintf(logbuf, "RECEIVED %d bytes: %s\n", numbytes * sizeof(char), buf);  
	  write(newlogfd, logbuf, logsize);
	}
      
      if(encrypt_flag)
	mdecrypt_generic(td, buf, numbytes);
      
      write(1, buf, numbytes);
    }
  
  return NULL;
}


int main(int argc, char **argv)
{
  int clilen, logfd;
  struct sockaddr_in serv_addr; 
  struct hostent *server;

  int c;  
  static struct option long_options[] = 
    {
      {"encrypt", required_argument, NULL, 'e'}, 
      {"log", required_argument, NULL, 'l'}, 
      {"port", required_argument, NULL, 'p'},
      {0,0,0,0}
    };
      
  while(1)
    {
      int option_index = 0;
      c = getopt_long(argc, argv, "p:l:e", long_options, &option_index);
      if(c == -1)
	break;
      switch(c)
	{
	case 'p':
	  portno = atoi(optarg);
	  break;
	case 'l':
	  log_flag = 1;
	  char *myfile = optarg;
	  logfd = creat(myfile, 0666);
	  if(logfd < 0)
	    {
	      fprintf(stderr, "Error: %s\n", strerror(errno));
	      exit(1);
	    }

	  break;      
	case 'e':
	  encrypt_flag = 1;
	  keyfd = open(optarg, O_RDONLY);
	  break;
	default: 
	  fprintf(stderr, "Error: %s\n", strerror(errno));
	  exit(1);
	}

    }
  /*Analyze flags*/
  if(!portno)
    portno = 9942;

  if(encrypt_flag)
    {
      char key[128];
      /*Let the key read 7 bytes from key*/
      read(keyfd, key, 7);
      td = mcrypt_module_open("twofish", NULL, "cfb", NULL);

      char *IV = malloc(mcrypt_enc_get_iv_size(td));
      
      memset(IV, 0, sizeof(char) * mcrypt_enc_get_iv_size(td));
      
      mcrypt_generic_init(td, key, 7, IV);
      atexit(closemcrypt);
      
    }

  set_terminal();
  
  /*Here, begin to open the socket, get the server, and prepare communications*/
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
    {
      fprintf(stderr, "Error creating socket\n");
      exit(1);
    }

  server = gethostbyname("localhost");
  if(server == NULL)
    {
      fprintf(stderr, "Error creating host\n");
      exit(1);
    }



  /*Set the fields in serv_addr*/
  serv_addr.sin_family = AF_INET;
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  serv_addr.sin_port = htons(portno);
  

  /*Connect to server*/
  if(connect(sockfd, (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
      fprintf(stderr, "Error connecting to server.\n");
      exit(1);
    }

  /*Success statement*/
  else fprintf(stderr, "Conected to server!\n");
  pthread_t thread_id;


  struct descriptors mystruct;
  mystruct.sockdesc = sockfd;
  mystruct.logdesc = logfd;
  pthread_create(&thread_id, NULL, read_from_server, &mystruct);
  /*Read from stdin to server*/

  char buf[BUFSIZE+1];
  while(1)
    {

      int bytenum;     
      bytenum = read(STDIN_FILENO, buf, BUFSIZE);
      buf[bytenum] = '\0';
      if(bytenum <=0)
	{
	  exit(1);
	}
      int i;
      for(i = 0; i < bytenum; i++)
	{

	  if(buf[i] == 0x0A || buf[i] == 0x0D)
	    {
	      write(STDOUT_FILENO, cr_lf, 2);
	      buf[i] = '\n';
	    }

	  
	  else
	    write(1, buf, 1);

	}
      
      if(encrypt_flag)
	mcrypt_generic(td, buf, bytenum);
      
      int len = write(sockfd, buf, 1);

      if(log_flag)
	{
	  char logbuf[BUFSIZE];
	  int bytes = sprintf(logbuf, "SENT %d bytes: %s \n", len, buf);
	  write(logfd, logbuf, bytes);
	}


    }

  reset_terminal();
}
