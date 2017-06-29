/*Sean Langley
sean.langley22@gmail.com
504 661 838
*/

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>


void signalhandler(int signum)
{
  fprintf(stderr, "Segmentation fault caught with signal. Exiting with exit code *4*\n");
  exit(4);
}



void segfault(void)
{
  char *null = NULL;
  *null = 'k';
}

int main(int argc, char **argv)
{


  int c;
  int options_index;
  char *input_file = NULL;
  char *output_file = NULL;
  static int segflag = 0;
  static int catchflag = 0;
  while(1)
    {
      static struct option long_options[] = 
	{
	  {"input", required_argument, 0, 'i'},
	  {"output", required_argument, 0, 'o'},
	  {"segfault", no_argument, &segflag, 1},
	  {"catch", no_argument, &catchflag, 1},
	  {0,0,0,0}
	};

      options_index = 0;
      c = getopt_long(argc, argv, "iosc", long_options, &options_index);
      if(c == -1) 
	break;
      
      switch(c)
	{
	case 'i':
	  input_file = optarg;
	  break;
	case 'o':
	  output_file = optarg;
	  break;
	case 0:
	  break;
	default:
	  fprintf(stderr, "Incorrect argument used. Usage: ./lab0 --<argument>\nArguments: input, output, segfault, catch\nExiting with exit code *1*\n");
	  exit(1);
	  break;
	}

    }


  //if --catch option is used
  if(catchflag != 0)
    {

      signal(SIGSEGV, signalhandler);
      
      
    }




  //if --segfault option is used, cause a segfault
  if(segflag != 0)
    {
      segfault();

    }




  
  int fd0, fd1;

  char data[256];
  int nbytes = sizeof(data);


  //If --input option is used, read from a file
  if(input_file)
    {

      fd0 = open(input_file, O_RDONLY);
      if(fd0 < 0)
	{
	  fprintf(stderr, "Problem with --input. Unable to open file %s. Exiting with exit code *2*\n", input_file);
	  exit(2);
	}
      read(fd0, data, nbytes);
    }




  //Otherwise, read from stdin

  else if(read(0, data, nbytes) < 0)
	{
	  fprintf(stderr, "Error reading from stdin. Exiting with code *1*\n");
	  exit(2);
	}


  int newlineflag=0;
  int i = 0;


  for(i = 0; i < strlen(data); i++)
    {
      if(data[i] == '\n')
	  newlineflag = 1;
      if(newlineflag == 1)
	{
	data[i+1] = '\0';
	break;
	}
    }


  //If --output is used, create a file to write to
  if(output_file)
    {

      fd1 = creat(output_file, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if(fd1 < 0)
	{
	  fprintf(stderr, "Error creating file %s. Exiting with exit code *3*\n", output_file);
	  exit(3);
	}
      write(fd1, data, strlen(data));

 
    }


  else if(write(1, data, strlen(data)) < 0)
	{
	  fprintf(stderr, "Error writing to stdout. Exiting with exit code *3*\n");
	  exit(3);
	}
    
  close(fd0);
  close(fd1);
  exit(0);
}
