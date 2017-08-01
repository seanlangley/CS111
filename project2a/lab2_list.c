#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include "SortedList.h"

struct timespec start, end;
int NUM_THREADS = 1;
int NUM_ITERATIONS = 1;
char opt_sync = 'd';
int opt_yield = 0;
SortedList_t *list;
SortedListElement_t *elements;
pthread_mutex_t mutex;
long long counter = 0;
char test_name[15] = "list-";
int t_len;
int lock = 0;
char *char_bank = "ABCDEFGHIJKLMNOPQRSTUVWXYZabdefghijklmnopqrstuvwxyz";




void *thread_function(void *arg)
{
  int threadid = *(int *)arg;
  int i;
  for(i = threadid; i < NUM_ITERATIONS * NUM_THREADS; i += NUM_THREADS)
    {
      if(opt_sync == 'm')
	{
	  pthread_mutex_lock(&mutex);
	  SortedList_insert(list, &elements[i]);
	  pthread_mutex_unlock(&mutex);

	}
      else if(opt_sync == 's')
	{
	  while(__sync_lock_test_and_set(&lock, 1) == 1);
	  SortedList_insert(list, &elements[i]);
	  __sync_lock_release(&lock);
	}
      else
	SortedList_insert(list, &elements[i]);
    }
  
  SortedList_length(list);
  SortedListElement_t *temp;
  for(i = threadid; i < NUM_ITERATIONS * NUM_THREADS; i += NUM_THREADS)
    {
      if(opt_sync == 'm')
	{
	  pthread_mutex_lock(&mutex);
	  temp = SortedList_lookup(list, elements[i].key);
	  if(temp == NULL)
	    {
	      fprintf(stderr, "Error: Key not found!\n");
	      exit(1);
	    }
	  SortedList_delete(temp);
	  pthread_mutex_unlock(&mutex);
	}
      else if(opt_sync == 's')
	{
	  while(__sync_lock_test_and_set(&lock, 1) == 1);
	  temp = SortedList_lookup(list, elements[i].key);
	  if(temp == NULL)
	    {
	      fprintf(stderr, "Error: Key not found!\n");
	      exit(1);
	    }
	  SortedList_delete(temp);
	  __sync_lock_release(&lock);

	}
      else
	{
	  temp = SortedList_lookup(list, elements[i].key);
	  if(temp == NULL)
	    {
	      fprintf(stderr, "Error: Key not found!\n");
	      exit(1);
	    }
	  SortedList_delete(temp);
	}
    }
  pthread_exit(NULL);
  return NULL;
}

void set_test_name()
{
  if(!opt_yield)
    strcat(test_name, "none");
  strcat(test_name, "-");
  if(opt_sync == 'm' || opt_sync == 's')
    {
      t_len = strlen(test_name);
      test_name[t_len] = opt_sync;
      test_name[t_len + 1] = '\0';
      
    }
  else if(opt_sync == 'd')
    strcat(test_name, "none");
    
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
	  {"yield", required_argument, 0, 'y'},
	  {"sync", required_argument, 0, 's'},
	  {0,0,0,0}
	};
      int option_index = 0;
      c = getopt_long(argc, argv, "t:i:y:", long_options, &option_index);
      
      if(c == -1)
	break;
      switch(c)
	{
	case 't':
	  NUM_THREADS = atoi(optarg);
	  break;
	case 'i':
	  NUM_ITERATIONS = atoi(optarg);
	  break;
	case 's':
	  if(strlen(optarg) != 1)
	    break;
	  if(*optarg == 'm' || *optarg == 's')
	    {
	      opt_sync = *optarg;
	      break;
	    }
	  else
	    {
	      fprintf(stderr, "Invalid sync option\n");
	      exit(1);

	    }
	     
	  break;
	case 'y':
	  if(strlen(optarg) == 0 || strlen(optarg) > 3)
	    {
	      fprintf(stderr, "Incorrect yield options");
	      exit(1);
	    }
	  strcat(test_name, optarg);
	  int i;
	  for(i = 0; i < strlen(optarg); i++)
	    {
	      if(optarg[i] == 'i')
		opt_yield |= INSERT_YIELD;
	      else if(optarg[i] == 'd')
		opt_yield = DELETE_YIELD;
	      else if(optarg[i] == 'l')
		opt_yield |= LOOKUP_YIELD;
	      else
		{
		  fprintf(stderr, "Incorrect yield options");
		  exit(1);
		}
	    } 
	  break;
	default:
	  perror("");
	  exit(1);
	  break;
	}
    }

  set_test_name();

  list = malloc(sizeof(SortedList_t));
  list->key = NULL;
  list->next = list;
  list->prev = list;

  int num_elems = NUM_ITERATIONS * NUM_THREADS;
  
  elements = malloc(sizeof(SortedListElement_t) * num_elems);
  pthread_t threads[NUM_THREADS];

  int *threadid = malloc(NUM_THREADS * sizeof(int));
  
  srand(time(NULL));
  int i;

  for(i = 0; i < num_elems; i++)
    {
      int key_len = rand() % 5 + 10;
      char *key = malloc((key_len + 1) * sizeof(char));
      int j;
      for(j = 0; j < key_len; j++)
	  key[j] = char_bank[rand() % strlen(char_bank)];

      key[j] = '\0';
      elements[i].key = key;
	  

    }
  

  
  if(opt_sync == 'm')
    pthread_mutex_init(&mutex, NULL);
  
  clock_gettime(CLOCK_MONOTONIC, &start);
  for(i = 0; i < NUM_THREADS; i++)
    {
      threadid[i] = i;
      if(pthread_create(&threads[i], NULL, thread_function, &threadid[i]) < 0)
	{
	  perror("pthread create error");
	  exit(1);
	}
    }
  
  for(i = 0; i < NUM_THREADS; i++)
    {
      if(pthread_join(threads[i], NULL) < 0)
	{
	  perror("pthread join error");
	  exit(1);
	}
    }
  clock_gettime(CLOCK_MONOTONIC, &end);
  
  long long runtime = (end.tv_sec - start.tv_sec) *1000000000 + (end.tv_nsec - start.tv_nsec);
  int num_ops = 3 * NUM_THREADS * NUM_ITERATIONS;

  int list_len = SortedList_length(list);
  if(list_len != 0)
    {
      fprintf(stderr, "Error: List length is %d\n", list_len);
      exit(1);
    }
  
  printf("%s,%d,%d,%d,%d,%lld,%lld\n",test_name, NUM_THREADS, NUM_ITERATIONS, 1, num_ops, runtime, runtime/num_ops);
  
  free(elements);
  free(threadid);
  exit(0);
}
