// Funções para manipulação de endereços IP
#include <arpa/inet.h>
// Definições de constantes, estruturas e funções relacionadas a operações de soquetes
#include <sys/socket.h>
// Definições de estruturas e funções relacionadas a operações de banco de dados de rede
#include <netdb.h>
// Definições de tipos de dados utilizados em várias chamadas de sistema
#include <sys/types.h>
// Definições de funções de entrada/saída padrão
#include <stdio.h>
// Definições de funções de utilidade, como alocação de memória e conversão de tipos
#include <stdlib.h>
// Definições de funções para manipulação de strings
#include <string.h>
// Definições de chamadas de sistema e constantes para operações E/S, como leitura e gravação em arquivos
#include <unistd.h>

// Configura a estrutura `addrinfo`, que é usada para fornecer informações de endereço
// para as funções de criação de socket (`socket`) e conexão (`connect`).
void setup_addrinfo(struct addrinfo **servinfo, char *hostname, char *port, int flags)
{
  struct addrinfo hints;
  int rv;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       // define a família do protocolo para IPv4 (`AF_INET`)
  hints.ai_socktype = SOCK_STREAM; // define o tipo de socket como TCP (`SOCK_STREAM`)
  hints.ai_flags = flags;          // recebe as flags passadas como parâmetro para a função

  // Caso aconteça algum erro, a função imprime uma mensagem de erro, libera a memória
  // alocada para a estrutura `servinfo` e encerra o programa
  if ((rv = getaddrinfo(hostname, port, &hints, servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    freeaddrinfo(*servinfo);
    exit(1);
  }
}

// Cria um socket e estabelece uma conexão com um servidor remoto
int get_socket_file_descriptor(char *hostname, char *port)
{
  int sockfd;
  struct addrinfo *servinfo, *p;
  char s[INET_ADDRSTRLEN];

  setup_addrinfo(&servinfo, hostname, port, 0);

  // Iteração sobre a lista de resultados retornados por `getaddrinfo`, tentando
  // criar um socket e conectar-se ao primeiro resultado possível
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    // Se ocorrer um erro ao criar o socket, a função imprime
    // uma mensagem de erro e continua para a próxima entrada
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("socket");
      continue;
    }

    // Se a conexão falhar, o socket é fechado e uma mensagem de erro é impressa.
    // Caso contrário, o loop é interrompido e a conexão é considerada bem-sucedida
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("connection");
      continue;
    }

    break;
  }

  // Caso aconteça algum erro, a função imprime uma mensagem de erro, libera a memória
  // alocada para a estrutura `servinfo` e encerra o programa
  if (p == NULL)
  {
    fprintf(stderr, "failed to connect\n");
    freeaddrinfo(servinfo);
    exit(1);
  }

  // Converter o endereço IP do servidor remoto em uma representação legível por humanos
  // e imprime uma mensagem indicando o host ao qual está conectado. Em seguida, a memória
  // alocada para a estrutura `servinfo` é liberada e o descritor do socket é retornado
  inet_ntop(p->ai_family, &(((struct sockaddr_in *)p->ai_addr)->sin_addr), s, sizeof(s));
  printf("connecting to %s\n", s);

  freeaddrinfo(servinfo);

  return sockfd;
}

// Cria um socket e o associa a uma porta local para aguardar conexões de entrada
int get_listener_socket_file_descriptor(char *port)
{
  int sockfd;
  struct addrinfo *servinfo, *p;
  char s[INET_ADDRSTRLEN];
  int yes = 1;

  // Chamada para configurar as informações de endereço necessárias, com a flag `AI_PASSIVE`
  // indicando que o endereço é usado para um socket de escuta
  setup_addrinfo(&servinfo, NULL, port, AI_PASSIVE);

  // Iteração sobre a lista de resultados retornados por `getaddrinfo`, tentando criar um
  // socket e vinculá-lo ao primeiro resultado possível
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    // O socket é criado usando a função `socket`, passando a família, tipo e protocolo
    // obtidos da estrutura `addrinfo`
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      perror("socket");
      continue;
    }

    // A função `setsockopt` define a opção `SO_REUSEADDR` no socket, permitindo que a
    // porta seja reutilizada após a aplicação ser encerrada
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
      perror("setsockopt");
      exit(1);
    }

    // A função bind é chamada para associar o socket à porta e ao endereço fornecidos
    // na estrutura `addrinfo`
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(sockfd);
      perror("connection");
      continue;
    }

    break;
  }

  // Se nenhum resultado possível for encontrado ou ocorrer um erro ao vincular o socket,
  // a função imprime uma mensagem de erro, libera a memória alocada para a estrutura
  // `servinfo` e encerra o programa
  if (p == NULL)
  {
    fprintf(stderr, "%s\n", "server: failed to bind");
    exit(1);
  }

  // Converter o endereço IP associado ao socket em uma representação legível por humanos
  // e imprime uma mensagem indicando o endereço e a porta em que está ouvindo. Em seguida,
  // a memória alocada para a estrutura `servinfo` é liberada e o descritor do socket é retornado
  inet_ntop(p->ai_family, &(((struct sockaddr_in *)p->ai_addr)->sin_addr), s, sizeof(s));
  printf("listening on %s\n", s);

  freeaddrinfo(servinfo);

  return sockfd;
}