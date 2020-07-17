#include <getopt.h>
#include <netinet/in.h> // определяет структуру sockaddr_in
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct FactorialArgs 
{
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod)
{
  uint64_t result = 0;
  a = a % mod;
  while (b > 0)
  {
    if (b % 2 == 1)
    {
      result = (result + a) % mod;
    }
    a = (a * 2) % mod;
    b /= 2;
  }
  return result % mod;
}

uint64_t Factorial(const struct FactorialArgs* args)
{
  uint64_t ans = 1;
  uint64_t start = args->begin;
  uint64_t end = args->end;
  uint64_t mod = args->mod;
  
  for (uint64_t i = start; i <= end; ++i)
  {
    ans *= i;
    ans %= mod;
  }
  return ans;
}

void* ThreadFactorial(void* args)
{
  struct FactorialArgs* fargs = (struct FactorialArgs*)args;
  return (void*)(uint64_t*)Factorial(fargs);
}

int main(int argc, char **argv)
{
  int tnum = -1;
  int port = -1;

  while (1)
  {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
    break;

    switch (c)
    {
      case 0:
      {
        switch (option_index)
        {
          case 0:
          {
            port = atoi(optarg);
            if (port < 0)
            {
              printf("\nThe port is a positive number\n\n");
              return 1;
            }
          }
          break;
          
          case 1:
          {
            tnum = atoi(optarg);
            if (tnum < 0)
            {
              printf("\nThe tnum is a positive number\n\n");
              return 1;
            }
          }
          break;
          
          default:
          printf("\nIndex %d is out of options\n\n", option_index);
        }
      }
      break;
      
      case '?':
      {
        printf("\nUnknown argument\n\n");
        return 1;
      }
      break;
    
      default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (port == -1 || tnum == -1)
  {
    fprintf(stderr, "\nUsing: %s --port 20001 --tnum 4\n\n", argv[0]);
    return 1;
  }

  // создание сокета, ориентированного на соединения
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0)
  {
    fprintf(stderr, "\nCan\'t create server socket\n\n");
    return 1;
  }

  // объявление адресной структуры
  struct sockaddr_in server_name;
  server_name.sin_family = AF_INET;
  // возвращает значение port, приведенное к сетевому порядку следования байтов
  // (от старшего байта к младшему)
  server_name.sin_port = htons((uint16_t)port);
  server_name.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  // устанавливаем флаги на сокете
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  // привязка сокета к адресу
  if (bind(server_fd, (struct sockaddr*)&server_name, sizeof(server_name)) < 0)
  {
    fprintf(stderr, "\nCan\'t bind to socket\n\n");
    return 1;
  }
  
  // перевод сокета в слушающий режим
  if (listen(server_fd, 128) < 0)
  {
    fprintf(stderr, "\nCould\'t listen on socket\n\n");
    return 1;
  }

  printf("\nServer listening at %d\n\n", port);

  while (1)
  {
    struct sockaddr_in client_name;
    socklen_t client_len = sizeof(client_name);

    // создание передающего сокета
    int client_fd = accept(server_fd, (struct sockaddr*)&client_name, &client_len);
    
    if (client_fd < 0)
    {
      fprintf(stderr, "\nCould\'t establish new connection\n\n");
      continue;
    }

    while (1)
    {
      char from_client[sizeof(uint64_t) * 3];
      //получение данных от клиента
      int read = recv(client_fd, from_client, sizeof(uint64_t) * 3, 0);

      if (!read)
      break;

      if (read < 0)
      {
        fprintf(stderr, "\nClient read failed\n\n");
        break;

      }
      
      if (read < sizeof(uint64_t) * 3)
      {
        fprintf(stderr, "\nClient send wrong data format\n\n");
        break;
      }

      pthread_t threads[tnum];

      uint64_t begin = 0;
      uint64_t end = 0;
      uint64_t mod = 0;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      fprintf(stdout, "\nReceive: %llu %llu %llu\n\n", (unsigned long long)begin, (unsigned long long)end, (unsigned long long)mod);

      struct FactorialArgs args[tnum];
      uint64_t range = (end - begin + 1) / tnum;
      
      for (int i = 0; i < tnum; i++)
      {
        args[i].begin = begin + i * range;
        args[i].end = args[i].begin + range - 1;         
        args[i].mod = mod;

        if (pthread_create(&threads[i], NULL, ThreadFactorial, (void*)&args[i]))
        {
          printf("\nError: pthread_create failed\n\n");
          return 1;
        }
      }

      uint64_t total = 1;
      for (int i = 0; i < tnum; i++)
      {
        uint64_t result = 0;
        pthread_join(threads[i], (void**)&result);
        total = MultModulo(total, result, mod);
      }

      printf("\nTotal: %llu\n\n", (unsigned long long)total);

      char buffer[sizeof(total)];
      sprintf(buffer, "%llu", (unsigned long long)total);
      
      if (send(client_fd, buffer, sizeof(total), 0) < 0)
      {
        fprintf(stderr, "\nCan\'t send data to client\n\n");
        break;
      }
    }

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
  }

  return 0;
}