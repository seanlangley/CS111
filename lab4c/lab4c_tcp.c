#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include "mraa.h"
#define MAX_BUF_SIZE 256

typedef struct pollfd pollfd_t;


int PERIOD = 1;
char SCALE = 'F';
const float B = 4275.0;               // B value of the thermistor
//const float B = 200000.0;               // B value of the thermistor
const float  R0 = 100000.0;
int logfd = 0;
int log_flag = 0;
int end_sequence = 0;
int pause_flag = 0;
int create_thread = 0;
int button_read;
char chartemp[40];
int time_d[3];
char **time_s;
char *id = NULL;
char *host = NULL;
float temperature;

/*Socket globals*/
int sockfd, portno, n;
char buffer[256];
struct sockaddr_in serv_addr;
struct hostent *server;


void generate_report(int fd, int *time_d, char **time_s);


void exit_1(char *str)
{
  fprintf(stderr, "Error: ");
  if(strlen(str) != 0)
    fprintf(stderr, "%s\n", str);
  
  if(errno != 0)
    perror("");
  fprintf(stderr, "Exiting with error code 1.\n");
  exit(1);

}

void *get_command(void *arg)
{
  char buf[MAX_BUF_SIZE];
  const char scaleF[] = "SCALE=F\n";
  const char scaleC[] = "SCALE=C\n";
  const char period[] = "PERIOD=\n";
  const char stop[] = "STOP\n";
  const char start[] = "START\n";
  const char off[] = "OFF\n";
  const char newline[] = "\n";
  
  
  int bytes;
  
  
  while(1)
    {
      /*
	pollfd_t pfds[1];
	pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;
	
	
	poll(pfds, 1, -1);
	if(pfds[0].revents & POLLIN);
      */
      {
	bytes = read(sockfd, buf, MAX_BUF_SIZE);	
	
	if(! bytes)
	  exit_1("Stdin closed.");
      }
      

      buf[bytes] = 0;
      
      
      
      /*Split commands by space*/
      char **res = NULL;
      char *p = strtok(buf, " ");
      int n_spaces = 0, i;
      
      /*Splitting the buffer into several strings*/
      while(p)
	{
	  res = realloc(res, sizeof(char*) * ++n_spaces);
	  if(res == NULL)
	    exit_1("realloc failure.");
	  res[n_spaces - 1] = p;
	  p = strtok(NULL, " ");
	  
	}
      
      res = realloc(res, sizeof(char*) * n_spaces +1);
      res[n_spaces] = 0;
      
      /*Adding newlines at the end of an input*/
      for (i = 0; i < n_spaces; ++i)
	{
	  
	  int len = strlen(res[i]);
	  if(res[i][len-1] != '\n')
	    {
	      len = strlen(res[i]) + strlen(newline);
	      char *ret = (char *) malloc(len * sizeof(char) + 1);
	      *ret = '\0';
	      res[i] = strcat(strcat(ret, res[i]), newline);					
	      
	    }
	  
	}
      
      for(i = 0; i < n_spaces; i++)
	{
	  
	  if(log_flag)	
	    write(logfd, res[i], strlen(res[i]));
	  
	  
	  
	  int j;
	  for(j = 0; j < 8; j++)
	    {
	      
	      if(j == 7)
		{
		  int per = res[i][j] - '0';
		  PERIOD = per;
		  break;
		}
	      if(res[i][j] != period[j])
		break;
	      
	    }
	  
	  if(! strcmp(res[i], scaleF))
	    SCALE = 'F';
	  
	  else if(! strcmp(res[i], scaleC))
	    SCALE = 'C';
	  else if(! strcmp(res[i], start))
	    pause_flag = 0;
	  else if(! strcmp(res[i], stop))
	    pause_flag = 1;
	  else if(! strcmp(res[i], off))
	    {
	      end_sequence = 1;
	      if(log_flag)
		generate_report(logfd, time_d, time_s);
	      generate_report(STDOUT_FILENO, time_d, time_s);
	      return NULL;
	      
	    }
	  
	}
      
      if(end_sequence)
	{	
	  free(res);
	  break;
	}
      
      
    }
  return NULL;
  
}


void get_options(int argc, char **argv)
{
  
  int c;
  
  while(1)
    {
      static struct option long_options[] = 
	{
	  {"period", required_argument, 0, 'p'},
	  {"scale", required_argument, 0, 's'},
	  {"log", required_argument, 0, 'l'},
	  {"id", required_argument, 0, 'i'},
	  {"host", required_argument, 0, 'h'},
	  {0,0,0,0}
	};
      int option_index = 0;
      c = getopt_long (argc, argv, "p:s:l:", long_options, &option_index);
      if (c == -1)
	break;
      switch (c)
	{
	case 0:
	  /* If this option set a flag, do nothing else now. */
	  if (long_options[option_index].flag != 0)
	    break;
	  printf ("option %s", long_options[option_index].name);

	  if (optarg)
	    printf (" with arg %s", optarg);

	  printf ("\n");
	  break;

	case 'h':
	  host = malloc(sizeof(char) * (strlen(optarg) + 1));
	  strcpy(host, optarg);
	  break;

	case 'i':
	  if(strlen(optarg) != 9)
	    exit_1("Invalid ID");
	  id = malloc(sizeof(char) * (strlen(optarg)+ 1));
	  strcpy(id, optarg);

	  break;

	case 'p':
	  PERIOD = atoi(optarg);
	  break;
	case 'l':
	  log_flag = 1;
	  char *FILE = optarg;
	  logfd = open(FILE, O_NONBLOCK | O_RDWR | O_CREAT | O_TRUNC, 0666);
	  if(logfd < 0)
	    {
	      exit(0);
	    }
	  break;
	case 's':
	  if(*optarg == 'C')
	    {
	      SCALE = *optarg;
	      break;
	    }
	  else if(*optarg == 'F')
	    break;
	  else
	    exit_1("Scale option must be C or F.");
	  break;
	case '?':		
	  exit_1("");
	  /* getopt_long already printed an error message. */
	  break;
	  
	default:
	  exit_1("");
	  
	}
    }
  if(argv[optind] == NULL)
    exit_1("You must enter a port number.");
  else
    portno = atoi(argv[optind]);

}


void get_time(int *arg)
{
  struct tm *mytime;
  time_t timep;
  time(&timep);
  mytime = localtime(&timep);
  
  arg[0] = mytime->tm_hour;
  arg[1] = mytime->tm_min;
  arg[2] = mytime->tm_sec;
}

float get_temperature(mraa_aio_context *tmpr)
{
  uint16_t value;
  value = mraa_aio_read(*tmpr);      
  float R = 1023.0/value-1.0;
  R = R0*R;    
  float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15 + 10;
  if(SCALE == 'F')
  temperature = temperature * 1.8 + 32;
  
  return temperature;
  }


void generate_report(int fd, int *time_d, char **time_s)
{

  if(button_read || end_sequence)
    dprintf(fd, "%02d:%02d:%02d SHUTDOWN\n", 
	    time_d[0], time_d[1], time_d[2]);



  else
    dprintf(fd, "%02d:%02d:%02d %.1f\n", 
	    time_d[0], time_d[1], time_d[2], temperature);


}



int main(int argc, char **argv)
{
  get_options(argc, argv);
  
  //  if(id == NULL)
    //    exit_1("You must enter an ID.");
  if(id == NULL)
    {
      char temp[] = {"504661838"};
      id = malloc(sizeof(char) * (strlen(temp) + 1));
      strcpy(id, temp);
    }



  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0)
    exit_1("Error opening socket.");
  
  if(host == NULL)
    server = gethostbyname("lever.cs.ucla.edu");

  else
    server = gethostbyname(host);

  if(server == NULL)
    exit_1("Error, no such host");
  
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
	(char *)&serv_addr.sin_addr.s_addr, 
	server->h_length);

  serv_addr.sin_port = htons(portno);

  if(connect(sockfd,(const struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    exit_1("Error connecting.");

  char *message = malloc(sizeof(char) * (strlen(id) + 10));
  char temp0[] = {"ID="};
  strcat(message, temp0);
  strcat(message, id);

  dprintf(sockfd, "ID=%s\n", message);
  
  /*Initialize the temperature sensor*/
  mraa_aio_context tmpr;
  mraa_result_t r1 = MRAA_SUCCESS;
  mraa_result_t r2 = MRAA_SUCCESS;
  tmpr = mraa_aio_init(0);

  
  /*Initialize the button*/
  mraa_gpio_context button;
  button = mraa_gpio_init(3);

  
  mraa_gpio_dir(button, MRAA_GPIO_IN);
  
  
  /*Conversions from thermistor reading to temperature*/
  signal(SIGINT, SIG_DFL);
  
  
  pthread_t threadid;
  time_s = malloc(sizeof(char*) * 3);
  
  
  
  while (1)
    {
      button_read = mraa_gpio_read(button);
      button_read = 0;
      //Still need to exit the while loop if shutdown is detected
      
      if(end_sequence)
	break;
      
      if(button_read)
	{
	  if(log_flag)
	    generate_report(logfd, time_d, time_s);
	  generate_report(STDOUT_FILENO, time_d, time_s);
	  break;
	}

      
      if(pause_flag)
	{
	  sleep(PERIOD);
	  continue;
	}
      
      get_time(time_d);
      
      int i;
      for(i = 0; i < 3; i++)
	{
	  time_s[i] = malloc(sizeof(char) * 4);
	  sprintf(time_s[i], "%d", time_d[i]);
	}
      
      /*Temperature Readings*/
      temperature = get_temperature(&tmpr);

      

      
      
      if(log_flag)   
	generate_report(logfd, time_d, time_s);
      generate_report(STDOUT_FILENO, time_d, time_s);
      generate_report(sockfd, time_d, time_s);
      
      if(! create_thread)
	{
	  if(pthread_create(&threadid, NULL, get_command, NULL) < 0)
	    exit_1("pthread_create failure.");
	  create_thread = 1;
	}
      
      
      sleep(PERIOD);  
      
    }
  
  
  r1 = mraa_aio_close(tmpr);
  r2 = mraa_gpio_close(button);
  close(logfd);

  if (r1 != MRAA_SUCCESS || r2 != MRAA_SUCCESS )
    {
      mraa_result_print(r1);
      mraa_result_print(r2);
    }
  
  int i;
  for(i = 0; i < 3; i++)
    free(time_s[i]);
  free(time_s);
  
  return 0;
}
