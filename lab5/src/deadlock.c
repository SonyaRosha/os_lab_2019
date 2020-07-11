#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

int numb = 0;

void some_func(int*);

// создаю мьютекс (по умолчанию создается быстрый мьютекс)
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main()
{
  pthread_t thread;
  // создаю поток
  if (pthread_create(&thread, NULL, (void*)some_func, (void*)&numb) != 0)
  {
    perror("\nERROR in pthread_create\n\n");
    exit(1);
  }

  if (pthread_join(thread, NULL) != 0) 
  {
    perror("\nERROR in pthread_join\n\n");
    exit(1);
  }
  
  printf("\nnumb = %i;\n\n", numb);

  return 0;
}

void some_func(int* number)
{
  int work;
  // поток захватывает мьютекс
  pthread_mutex_lock(&mutex);

  work = *number;
  work++;

  // поток вновь пытается захватить один и тот же мьютекс,
  // но так как он заблокирован, сделать у него это не получится
  // возникает состояние deadlock или взаимоблокировка
  pthread_mutex_lock(&mutex);

  work++;
  *number = work;

  pthread_mutex_unlock(&mutex);
  pthread_mutex_unlock(&mutex);
}