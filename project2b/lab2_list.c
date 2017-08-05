/*
NAME: Sean Langley
EMAIL: sean.langley22@gmail.com
ID: 504 661 838
*/
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include "SortedList.h"

typedef struct __synced_list {
  SortedList_t *list;
  pthread_mutex_t lock;
  int s_lock;
  
} SyncedList_t;

typedef struct timespec timespec_t;

timespec_t start, end;

int *threadid;
long long locktime = 0;
int NUM_THREADS = 1;
int NUM_ITERATIONS = 1;
int NUM_LISTS = 1;
int NUM_ELEMS;
char opt_sync = 'd';
int opt_yield = 0;

SortedList_t *list = NULL;
SortedListElement_t *elements;
SyncedList_t **synced_lists = NULL;


pthread_mutex_t mutex;
long long counter = 0;
char test_name[15] = "list-";
int t_len;
int lock = 0;
char *char_bank = "ABCDEFGHIJKLMNOPQRSTUVWXYZabdefghijklmnopqrstuvwxyz";




/*Hashing algorithm*/
unsigned long
hash(const char *str)
{
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}


void free_memory()
{
  int i;
  
  for(i = 0; i < NUM_ELEMS; i++)
    free((void *)elements[i].key);


  if(synced_lists != NULL)
    for(i = 0; i < NUM_LISTS; i++)
    {     
      if(synced_lists[i] != NULL)
      	free(synced_lists[i]->list);	
      free(synced_lists[i]);

    }


  free(synced_lists);
  free(list);
  free(elements);
  free(threadid);
  return;
}


void exit_failure1(char *str)
{
  fprintf(stderr, "Error: ");
  if(strlen(str) != 0)
    fprintf(stderr, "%s\n", str);
  
  if(errno != 0)
    perror("");
  fprintf(stderr, "Exiting with error code 1.\n");
  free_memory();
  exit(1);
}

void exit_failure2(char *str)
{
  fprintf(stderr, "Program error: ");
  if(strlen(str) != 0)
    fprintf(stderr, "%s\n", str);
  
  if(errno != 0)
    perror("");
  fprintf(stderr, "Exiting with error code 2.\n");
  free_memory();
  exit(2);

}

void add_time(timespec_t *lockstart, timespec_t *lockend)
{
  locktime += (lockend->tv_sec - lockstart->tv_sec) *1000000000\
    + (lockend->tv_nsec - lockstart->tv_nsec);
}

int list_length()
{
  if(NUM_LISTS < 1)
    exit_failure2("list_length called with 1 list.");
  int i;
  int list_len = 0;
  for(i = 0; i < NUM_LISTS; i++)
    {
      int p = SortedList_length(synced_lists[i]->list);
      list_len += p;
    }
  return list_len;
}


void *thread_function(void *arg)
{
  int threadid = *(int *)arg;
  timespec_t lockstart, lockend;
  int i;
  for(i = threadid; i < NUM_ITERATIONS * NUM_THREADS; i += NUM_THREADS)
    {

      if(NUM_LISTS > 1)
	{
	  //Get the list that the key should be insterted in
	  int listnum = hash(elements[i].key) % NUM_LISTS;

	  if(opt_sync == 'm')
	    {
	      clock_gettime(CLOCK_MONOTONIC, &lockstart);
	      pthread_mutex_lock(&(synced_lists[listnum]->lock));
	      clock_gettime(CLOCK_MONOTONIC, &lockend);
	      add_time(&lockstart, &lockend);
	      
	      SortedList_insert(synced_lists[listnum]->list, &elements[i]);
	      pthread_mutex_unlock(&(synced_lists[listnum]->lock));
	    }
	  else if(opt_sync == 's')
	    {
	      clock_gettime(CLOCK_MONOTONIC, &lockstart);
	      while(__sync_lock_test_and_set(&synced_lists[listnum]->s_lock, 1) == 1);
	      clock_gettime(CLOCK_MONOTONIC, &lockend);
	      add_time(&lockstart, &lockend);

	      SortedList_insert(synced_lists[listnum]->list, &elements[i]);
	      __sync_lock_release(&synced_lists[listnum]->s_lock);
	    }

	  else
	    SortedList_insert(synced_lists[listnum]->list, &elements[i]);
	}
    


      /*If the --lists option isn't used*/
      else if(NUM_LISTS == 1)
	{
	  if(opt_sync == 'm')
	    {
	      clock_gettime(CLOCK_MONOTONIC, &lockstart);
	      pthread_mutex_lock(&mutex);
	      clock_gettime(CLOCK_MONOTONIC, &lockend);
	      add_time(&lockstart, &lockend);
	  
	      SortedList_insert(list, &elements[i]);
	      pthread_mutex_unlock(&mutex);


	    }
	  else if(opt_sync == 's')
	    {
	  
	      clock_gettime(CLOCK_MONOTONIC, &lockstart);
	      while(__sync_lock_test_and_set(&lock, 1) == 1);
	      clock_gettime(CLOCK_MONOTONIC, &lockend);
	      add_time(&lockstart, &lockend);

	      SortedList_insert(list, &elements[i]);
	      __sync_lock_release(&lock);

	    }
	  else	    
	    SortedList_insert(list, &elements[i]);

	}      

    }    

  if(NUM_LISTS == 1)
    SortedList_length(list);
  else if(NUM_LISTS > 1)
    list_length();
  SortedListElement_t *temp;
  for(i = threadid; i < NUM_ITERATIONS * NUM_THREADS; i += NUM_THREADS)
    {
      
      if(NUM_LISTS > 1)
	{
	  int listnum = hash(elements[i].key) % NUM_LISTS;

	  if(opt_sync == 'm')
	    {
	      clock_gettime(CLOCK_MONOTONIC, &lockstart);
	      pthread_mutex_lock(&(synced_lists[listnum]->lock));
	      clock_gettime(CLOCK_MONOTONIC, &lockend);
	      add_time(&lockstart, &lockend);

	      temp = SortedList_lookup(synced_lists[listnum]->list, elements[i].key);

	      if(temp == NULL)
		exit_failure2("Key not found.");
	      SortedList_delete(temp);
	      pthread_mutex_unlock(&(synced_lists[listnum]->lock));
	    }
	  else if(opt_sync == 's')
	    {
	      clock_gettime(CLOCK_MONOTONIC, &lockstart);
	      while(__sync_lock_test_and_set(&synced_lists[listnum]->s_lock, 1) == 1);
	      clock_gettime(CLOCK_MONOTONIC, &lockend);
	      add_time(&lockstart, &lockend);

	      temp = SortedList_lookup(synced_lists[listnum]->list, elements[i].key);
	      if(temp == NULL)
		exit_failure2("Key not found.");
	      
	      SortedList_delete(temp);
	      __sync_lock_release(&synced_lists[listnum]->s_lock);

	    }
      
	  else
	    {
	      temp = SortedList_lookup(synced_lists[listnum]->list, elements[i].key);
	      if(temp == NULL)
		exit_failure2("Key not found.");
	      
	      SortedList_delete(temp);
	    }
	}


      /*If the --lists option isn't used*/
      else if(NUM_LISTS == 1)
	{
	  if(opt_sync == 'm')
	    {
	  
	      clock_gettime(CLOCK_MONOTONIC, &lockstart);
	      pthread_mutex_lock(&mutex);
	      clock_gettime(CLOCK_MONOTONIC, &lockend);
	      add_time(&lockstart, &lockend);


	      temp = SortedList_lookup(list, elements[i].key);
	      if(temp == NULL)
		exit_failure2("Key not found.");

	      SortedList_delete(temp);
	      pthread_mutex_unlock(&mutex);
	    }
	  else if(opt_sync == 's')
	    {

	      clock_gettime(CLOCK_MONOTONIC, &lockstart);
	      while(__sync_lock_test_and_set(&lock, 1) == 1);
	      clock_gettime(CLOCK_MONOTONIC, &lockend);
	      add_time(&lockstart, &lockend);
	  
	      temp = SortedList_lookup(list, elements[i].key);
	      if(temp == NULL)	       
		exit_failure2("Key not found.");

	      SortedList_delete(temp);
	      __sync_lock_release(&lock);

	    }
	  else
	    {
	      temp = SortedList_lookup(list, elements[i].key);
	      if(temp == NULL)
		exit_failure2("Key not found.");

	      SortedList_delete(temp);
	    }
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
  int i;
  while(1)
    {
      static struct option long_options[] = 
	{
	  {"threads", required_argument, 0, 't'},
	  {"iterations", required_argument, 0, 'i'},
	  {"yield", required_argument, 0, 'y'},
	  {"sync", required_argument, 0, 's'},
	  {"lists", required_argument, 0, 'l'},
	  {0,0,0,0}
	};
      int option_index = 0;
      c = getopt_long(argc, argv, "t:i:y:", long_options, &option_index);
      
      if(c == -1)
	break;
      switch(c)
	{
	case 'l':
	  NUM_LISTS = atoi(optarg);
	  if(NUM_LISTS < 1)
	    exit_failure1("List number must be greater than 0.");
	  break;
	case 't':
	  NUM_THREADS = atoi(optarg);
	  if(NUM_THREADS < 1)
	    exit_failure1("Thread number must be greater than 0.");
	  break;
	case 'i':
	  NUM_ITERATIONS = atoi(optarg);
	  if(NUM_ITERATIONS < 1)
	    exit_failure1("Iteration number must be greater than 0.");


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
	    exit_failure1("Invalid sync option.");
	     
	  break;
	case 'y':
	  if(strlen(optarg) == 0 || strlen(optarg) > 3)	      	    
	    exit_failure1("Invalid yield options.");


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
		exit_failure1("Invalid yield options.");

	    } 
	  break;
	default:
	  exit_failure1("");
	  break;
	}
    }

  set_test_name();





  /*If we have multiple lists, allocate the array of synced lists*/
  if(NUM_LISTS > 1)
    {
      //Allocate an array of SyncedList_t pointers
      synced_lists = malloc(sizeof(SyncedList_t*) * NUM_LISTS);

      if(synced_lists == NULL)
	exit_failure2("Synced lists creation error");
      //Allocate SyncedLists in each spot of the synced_lists array
      for(i = 0; i < NUM_LISTS; i++)
	{	  
	  synced_lists[i] = malloc(sizeof(SyncedList_t));
	  synced_lists[i]->list = malloc(sizeof(SortedList_t));
					  
	  if(synced_lists[i] == NULL || synced_lists[i]->list == NULL)					  	  
	    exit_failure2("Synced lists creation error");
	  
	  synced_lists[i]->list->key = NULL;
	  synced_lists[i]->list->next = synced_lists[i]->list;
	  synced_lists[i]->list->prev = synced_lists[i]->list;

	  synced_lists[i]->s_lock = 0;
	  pthread_mutex_init(&synced_lists[i]->lock, NULL);
	  


	}
      
    }



  /*If we only have one list, just allocate the list as normal*/
  if(NUM_LISTS == 1)
    {
      list = malloc(sizeof(SortedList_t));
      list->key = NULL;
      list->next = list;
      list->prev = list;

    }



  /*Create the array of elements to be inserted into the list*/

  NUM_ELEMS = NUM_THREADS * NUM_ITERATIONS;
  elements = malloc(sizeof(SortedListElement_t) * NUM_ELEMS);
  srand(time(NULL));
  for(i = 0; i < NUM_ELEMS; i++)
    {
      int key_len = rand() % 5 + 10;
      char *key = malloc((key_len + 1) * sizeof(char));
      int j;
      for(j = 0; j < key_len; j++)
	key[j] = char_bank[rand() % strlen(char_bank)];

      key[j] = '\0';
      elements[i].key = key;
	  
    }
  

  /*Start threading the application*/
  pthread_t threads[NUM_THREADS];
  threadid = malloc(NUM_THREADS * sizeof(int));
  
  if(opt_sync == 'm')
    pthread_mutex_init(&mutex, NULL);
  
  clock_gettime(CLOCK_MONOTONIC, &start);
  for(i = 0; i < NUM_THREADS; i++)
    {
      threadid[i] = i;
      if(pthread_create(&threads[i], NULL, thread_function, &threadid[i]) < 0)	
	exit_failure1("Pthread create error");

    }

  for(i = 0; i < NUM_THREADS; i++)
    if(pthread_join(threads[i], NULL) < 0)
      exit_failure1("Pthread join error");



  /*Threads are terminated, begin the administrative work*/
  clock_gettime(CLOCK_MONOTONIC, &end);
  
  long long runtime = (end.tv_sec - start.tv_sec) *1000000000 + (end.tv_nsec - start.tv_nsec);

  int num_ops = 3 * NUM_THREADS * NUM_ITERATIONS;


  int list_len;
  /* --lists option isn't used*/
  if(NUM_LISTS == 1)
    list_len = SortedList_length(list);

  /* --lists option is used*/
  if(NUM_LISTS > 1)
    list_len = list_length();


  if(list_len != 0)
    {
      fprintf(stderr, "List length is %d.", list_len);
      exit_failure2("");
    }


  
  long long time_per_op = runtime/num_ops;
  
  long long lock_ops = 2 * NUM_THREADS * NUM_ITERATIONS;


  long long time_per_lock = locktime / lock_ops;
  
  printf("%s,%d,%d,%d,%d,%lld,%lld,%lld\n",test_name, NUM_THREADS, \
	 NUM_ITERATIONS, NUM_LISTS, num_ops, runtime, time_per_op, time_per_lock);



  free_memory();
  exit(0);
}
