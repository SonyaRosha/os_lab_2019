#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct Server
{
  char ip[255];
  int port;
};

bool ConvertStringToUI64(const char* str, uint64_t* val)
{
  char* end = NULL;
  // функция strtoull конвертирует строку в беззнаковое целое чилсло
  unsigned long long i = strtoull(str, &end, 10);
  // если данная строка не входит в обрабатываемый диапазон чисел
  if (errno == ERANGE)
  {
    fprintf(stderr, "\nOut of uint64_t range: %s\n\n", str);
    return false;
  }

  if (errno != 0)
  return false;

  *val = i;
  return true;
}

int main(int argc, char **argv)
{
  uint64_t k = -1;
  uint64_t mod = -1;
  unsigned int servers_num = 0; 
  FILE* pf;

  // IP-адрес представляет собой число размером 32 бита
  // Адрес делится на четыре октета, по 8 бит каждый, которые могут
  // иметь значение от 0 (00000000) до 255 (11111111)
  char servers[255] = {'\0'};

  while (true)
  {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
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
            ConvertStringToUI64(optarg, &k);
            if (k <= 0)
            {
              printf("\nThe factorial argument is a positive number!\n\n");
              return 1;
            }
          }
          break;

          case 1:
          {
            ConvertStringToUI64(optarg, &mod);
            if (mod <= 0)
            {
              printf("\nThe factorial modul's is a positive number!\n\n");
              return 1;
            }
          }
          break;

          case 2:
          {
            memcpy(servers, optarg, strlen(optarg));
            if((pf = fopen(servers, "r")) == NULL)
            {
              printf("\nCan\'t open file\n\n");
              return 1;
            }
            else
            {
              while (!feof(pf)) 
              {
                char buf[64];
                if (fscanf(pf, "%s\n", buf) < 1)
                {
                  printf("\nCan\'t read file\n\n");
                  fclose(pf);
                  exit(1);
                }
                else
                {
                  servers_num++;
                }
              }
              fclose(pf);
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
        printf("Arguments error\n");
      }
      break;

      default:
      fprintf(stderr, "\ngetopt returned character code 0%o?\n\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers))
  {
    fprintf(stderr, "\nUsing: %s --k 1000 --mod 5 --servers /path/to/file\n\n", argv[0]);
    return 1;
  }

  // открываем файл, в котором лежат адреса серверов
  if((pf=fopen(servers, "r")) == NULL)
  {
    printf("\nCan\'t open file\n\n");
    fclose(pf);
    return 1;
  }

  struct Server* to = malloc(sizeof(struct Server) * servers_num); 
  
  for(int i = 0; i < servers_num; i++)
  { 
    char buf[64], port[16];
    int num = fscanf(pf, "%s\n", buf);
    char* twoPoints = strchr(buf, ':');
     
    if (num < 1)
    {
      printf("\nCan\'t read file, num = %d\n\n", num);
      fclose(pf);
      exit(1);
    }

    if(twoPoints == NULL)
    {
      printf("\nIncorrect address entry for the server\n\n");
      fclose(pf);
      exit(1);
    }

    memcpy(to[i].ip, buf, twoPoints - buf);
    memcpy(port, twoPoints + 1, strlen(buf) - (twoPoints - buf));
    to[i].port = atoi(port);
  }
  fclose(pf);

  // нагрузка на каждый сервер 
  int range = k / servers_num;
  uint64_t answer = 1;
  for (int i = 0; i < servers_num; i++)
  {
    struct hostent* host_name = gethostbyname(to[i].ip); 
    if (host_name == NULL)
    {
      fprintf(stderr, "\ngethostbyname failed with %s\n\n", to[i].ip);
      exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(to[i].port);
    server.sin_addr.s_addr = *((unsigned long*)host_name->h_addr_list[0]);

    int client_sock = socket(AF_INET, SOCK_STREAM, 0); 
    if (client_sock < 0)
    {
      fprintf(stderr, "\nSocket creation failed\n\n");
      exit(1);
    }

    if (connect(client_sock, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
      fprintf(stderr, "\nConnection failed\n\n");
      exit(1);
    }

    // задания
    uint64_t begin = i * range + 1;
    uint64_t end = (i + 1) * range;
    
    char task[sizeof(uint64_t) * 3];
    memcpy(task, &begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &mod, sizeof(uint64_t));

    // записываем задания в сокет
    if (send(client_sock, task, sizeof(task), 0) < 0)
    {
      fprintf(stderr, "\nSend failed\n\n");
      exit(1);
    }
    
    // получаем ответ
    char response[sizeof(uint64_t)];
    if (recv(client_sock, response, sizeof(response), 0) < 0)
    {
      fprintf(stderr, "\nRecieve failed\n\n");
      exit(1);
    }

    uint64_t temp;
    if (ConvertStringToUI64(response, &temp) == false)
    {
      printf("\nError converting string to uint64_t\n\n");
      exit(1);
    }
    printf("temp = %llu\n", (unsigned long long)temp);
    answer *= temp;
    close(client_sock);
  }
  printf("\nanswer: %llu\n", (unsigned long long)answer % mod);
  free(to);
  return 0;
}