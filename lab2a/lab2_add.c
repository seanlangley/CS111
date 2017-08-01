#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

/*Global variables*/
int thread_flag = 0;
int iteration_flag = 0;

/* NUM_THREADS and NUM_ITERATIONS are set to 1 by default*/
int NUM_THREADS = 1;
int NUM_ITERATIONS = 1;
long long counter = 0;
int opt_yield = 0;
int lock = 0;
char test_name[13];
char opt_sync = 'd';
pthread_mutex_t mutex;

void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
}

void compare_add(long long *pointer, long long value)
{
  long long prev, sum;
  do
    {
      prev = *pointer;
      sum = prev + value;
      if(opt_yield)
	sched_yield();      

    }while(__sync_val_compare_and_swap(pointer, prev, sum) != prev);
}

void *thread_function(void *counter)
{
  int i;  

  for(i = 0; i < NUM_ITERATIONS; i++)
    {
      if(opt_sync == 'm')
	{
	  pthread_mutex_lock(&mutex);
	  add((long long *) counter, 1);
	  pthread_mutex_unlock(&mutex);	  
	}
      else if(opt_sync == 'c')
	compare_add((long long*) counter, 1);

      else if(opt_sync == 's')
	{
	  while(__sync_lock_test_and_set(&lock, 1));
	  add((long long*) counter, 1);
	  __sync_lock_release(&lock);
	}
      
      else if(opt_sync == 'd')
	  add((long long *) counter, 1);

    }

  for(i = 0; i < NUM_ITERATIONS; i++)
    {
      if(opt_sync == 'm')
	{
	  pthread_mutex_lock(&mutex);
	  add((long long *) counter, -1);
	  pthread_mutex_unlock(&mutex);	  
	}
      else if(opt_sync == 'c')
	compare_add((long long*) counter, -1);
      else if(opt_sync == 's')
	{
	  while(__sync_lock_test_and_set(&lock, 1));
	  add((long long*) counter, -1);
	  __sync_lock_release(&lock);
	}
      else if(opt_sync == 'd')
	add((long long*) counter, -1);
    }


  pthread_exit(NULL);
  
}

void set_test_name()
{
  if(opt_yield)
    {
      if(opt_sync == 'd')
	strcpy(test_name, "add-yield-none");
      else if(opt_sync == 'm')
	strcpy(test_name, "add-yield-m");
      else if(opt_sync == 's')
	strcpy(test_name, "add-yield-s");
      else if(opt_sync == 'c')
	strcpy(test_name, "add-yield-c");
    }
  else
    {
      if(opt_sync == 'd')
	strcpy(test_name, "add-none");
      else if(opt_sync == 'm')
	strcpy(test_name, "add-m");
      else if(opt_sync == 's')
	strcpy(test_name, "add-s");
      else if(opt_sync == 'c')
	strcpy(test_name, "add-c");
    }



}

int main(int argc, char **argv)
{
  int c;
  while(1)
    {
      static struct option long_options[] = 
	{
	  {"threads", required_argument, 0, 't'}, 
	  {"iterations", required_argument, 0, 'i'}, 
	  {"yield", no_argument, 0, 'y'},
	  {"sync", required_argument, 0, 's'},
	  {0,0,0,0}
	};
      int option_index = 0;
      c = getopt_long(argc, argv, "t:i:y", long_options, &option_index);
      
      if(c == -1)
	break;
      switch(c)
	{
	case 's':	  
	  opt_sync = *optarg;
	  break;
	case 'y':
	  opt_yield = 1;
	  break;
	case 't':
	  thread_flag = 1;
	  NUM_THREADS = atoi(optarg);
	  break;
	case 'i':
	  iteration_flag = 1;
	  NUM_ITERATIONS = atoi(optarg);
	  break;
	default:	  
	  perror("");
	  exit(1);
	  break;
	}
    }

  struct timespec start, end;
  if(clock_gettime(CLOCK_MONOTONIC, &start) == -1)
    {
      perror("clock_gettime error");
      exit(1);
    }
  /*Create some threads*/
  pthread_t threads[NUM_THREADS];
  /*Create a mutex variable*/
  pthread_mutex_init(&mutex, NULL);
  int rc;
  long t;
  for(t = 0; t < NUM_THREADS; t++)
    {
      rc = pthread_create(&threads[t], NULL, thread_function, &counter);
      if(rc)
	{
	  perror("pthread_create error");
	  exit(1);
	}
    }  
  
  for(t = 0; t < NUM_THREADS; t++)
    {
      rc = pthread_join(threads[t], NULL);
      if(rc)
	{
	  perror("pthread_join error");
	  exit(1);
	}
    }

  if(clock_gettime(CLOCK_MONOTONIC, &end) == -1)
    {
      perror("Clock_gettime faulure");
      exit(1);
    }
  long long runtime = (end.tv_sec - start.tv_sec) * 1000000000
    + (end.tv_nsec - start.tv_nsec);
  
  long long num_ops = NUM_THREADS * NUM_ITERATIONS * 2;
  
  long long average = runtime / num_ops;
  
  
  set_test_name();
  printf("%s,%d,%d,%lld,%lld,%lld,%lld\n", test_name, 
	 NUM_THREADS, NUM_ITERATIONS, num_ops, runtime, average, counter);

  /*Destroy threads and mutex*/
  pthread_mutex_destroy(&mutex);
  pthread_exit(NULL);
  exit(0);
}
