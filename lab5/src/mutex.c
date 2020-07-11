/********************************************************
 * An example source module to accompany...
 *
 * "Using POSIX Threads: Programming with Pthreads"
 *     by Brad nichols, Dick Buttlar, Jackie Farrell
 *     O'Reilly & Associates, Inc.
 *  Modified by A.Kostin
 ********************************************************
 * mutex.c
 *
 * Simple multi-threaded example with a mutex lock.
 */

 
/*при запуске без мьютекса потоки наперегонки инкрементируют 
общую перемунную, но тк циклы выполняются быстро, то оба потока одновременно 
присвают common одно и то же значение. если количество итераций
 вложенного цикла в одной из функций увеличить или уменьшить, то
 один поток будет обгонять другой, а тот поток, который отстает
 будет продолжать инкрементирование с того числа, на котором закончил
 предыдущий поток
 при запуске с мьютексами один поток блокирует общую переменную и не отпускает 
 пока не завершится. потом проделывает ту же работу другой поток*/
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void do_one_thing(int *);
void do_another_thing(int *);
void do_wrap_up(int);
int common = 0; // общая переменная для двух потоков
int r1 = 0, r2 = 0, r3 = 0;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;  // создаем исключающий семафор (мьютекс)

// Мьютекс можно создать еще одним способом:
// pthread_mutex_t mutex;
// pthread_mutex_init (&mutex, NULL);

int main()
{
  pthread_t thread1, thread2;

  if (pthread_create(&thread1, NULL, (void *)do_one_thing, (void *)&common) != 0)
  {
    perror("pthread_create 1");
    exit(1);
  }

  if (pthread_create(&thread2, NULL, (void *)do_another_thing, (void *)&common) != 0)
  {
    perror("pthread_create 2");
    exit(1);
  }

  if (pthread_join(thread1, NULL) != 0) 
  {
    perror("pthread_join 1");
    exit(1);
  }

  if (pthread_join(thread2, NULL) != 0) 
  {
    perror("pthread_join 2");
    exit(1);
  }

  do_wrap_up(common);

  return 0;
}

void do_one_thing(int *pnum_times)
{
  int i, j, x;
  unsigned long k;
  int work;

  for (i = 0; i < 50; i++) 
  {
    pthread_mutex_lock(&mut);  // поток пытается захватить мьютекс
    
    printf("\nDOING ONE THING\n");
    work = *pnum_times;
    printf("\tcounter = %d\n", work);
    work++;
    for (k = 0; k < 500000; k++);
    *pnum_times = work;
	
    pthread_mutex_unlock(&mut);  // освобождение мьютекса
  }
}

void do_another_thing(int *pnum_times) 
{
  int i, j, x;
  unsigned long k;
  int work;
  
  for (i = 0; i < 50; i++)
  {
    pthread_mutex_lock(&mut);  // поток пытается захватить мьютекс
    
    printf("\nDOING ANOTHER THING\n");
    work = *pnum_times;
    printf("\tcounter = %d\n", work);
    work++;
    for (k = 0; k < 500000; k++);
    *pnum_times = work;
    
    pthread_mutex_unlock(&mut);  // освобождение мьютекса
  }
}

void do_wrap_up(int counter)
{
  int total;
  printf("\nALL DONE\n\tcounter = %d\n", counter);
}