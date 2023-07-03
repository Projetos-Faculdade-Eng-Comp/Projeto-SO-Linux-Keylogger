#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "networking.h"

#define PORT "3491"
#define BACKLOG 15
#define BUFFER_SIZE 1000

int loop = 1;           // Variável de controle para manter o loop do servidor
int uppercase_mode = 0; // 0 para minúsculo, 1 para maiúsculo

// Manipulador de sinal para SIGINT (Ctrl+C)
void sigint_handler(int sig)
{
  loop = 0; // Define loop como falso para encerrar o servidor
}

// Manipulador de sinal para SIGCHLD (quando um processo filho termina)
void sigchld_handler(int sig)
{
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
  // "Reap" (espera) todos os processos filhos que terminaram
}

// Função para imprimir mensagem de uso correto do programa e sair
void print_usage_and_quit(char *application_name);

int main(int argc, char *argv[])
{
  int sockfd, new_fd; // Descritores de arquivo para o socket de escuta e novas conexões
  struct sigaction sa;
  struct sockaddr_storage their_addr; // Estrutura para armazenar informações do cliente
  socklen_t sin_size;
  char s[INET_ADDRSTRLEN];  // Buffer para armazenar o endereço IP do cliente
  char buffer[BUFFER_SIZE]; // Buffer para armazenar os dados recebidos
  ssize_t bytes_received;
  int i;
  FILE *fp; // Ponteiro de arquivo para o arquivo de saída, se fornecido

  int file = 0, option = 0;
  // Processar argumentos de linha de comando para a opção de arquivo de saída
  while ((option = getopt(argc, argv, "sf:")) != -1)
  {
    switch (option)
    {
    case 's':
      // Redirecionar saída padrão e saída de erro para /dev/null
      freopen("/dev/null", "w", stdout);
      freopen("/dev/null", "w", stderr);
      break;
    case 'f':
      // Indicar que a opção de arquivo foi fornecida e abrir o arquivo no modo de acrescentar (append)
      file = 1;
      if ((fp = fopen(optarg, "a")) == NULL)
      {
        perror("opening");
        exit(1);
      }
      break;
    default:
      print_usage_and_quit(argv[0]);
    }
  }

  // Obtém o descritor de arquivo do socket do servidor
  sockfd = get_listener_socket_file_descriptor(PORT);

  // Configura o servidor para ouvir por conexões
  if (listen(sockfd, BACKLOG) == -1)
  {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // Reapar os processos filhos
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1)
  {
    perror("sigaction");
    exit(1);
  }

  printf("%s\n", "server: waiting for connections");

  // Loop principal do servidor
  while (loop)
  {
    sin_size = sizeof(their_addr);
    // Aceitar uma nova conexão
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1)
    {
      perror("accept");
      continue;
    }

    // Converte o endereço IP do cliente para uma string legível
    inet_ntop(their_addr.ss_family, &(((struct sockaddr_in *)&their_addr)->sin_addr), s, sizeof(s));
    printf("server: got connection from %s\n", s);

    // Criar um processo filho para lidar com a nova conexão
    if (!fork())
    {
      close(sockfd); // Fechar o socket de escuta no processo filho

      // Receber dados da nova conexão
      bytes_received = recv(new_fd, buffer, sizeof(buffer), 0);

      if (bytes_received < 0)
      {
        perror("recv");
        return 1;
      }

      // Processar os dados recebidos
      while (bytes_received > 0)
      {
        for (i = 0; i < bytes_received; ++i)
        {
          if (buffer[i] != '\n' && buffer[i] != '\0')
          {
            if (strcmp(&buffer[i], "SPACE") == 0)
            {
              if (file)
              {
                fprintf(fp, " "); // Escreve um espaço no arquivo
                fflush(fp);       // Atualiza o arquivo
              }
              else
              {
                putchar(' '); // Imprime um espaço na saída padrão
                fflush(stdout);
              }
              i += 4; // Pula os próximos 4 caracteres após "SPACE"
            }
            else if (strcmp(&buffer[i], "ENTER") == 0 || strcmp(&buffer[i], "RIGHTENTER") == 0)
            {
              if (file)
              {
                fprintf(fp, "\n"); // Escreve uma nova linha no arquivo
                fflush(fp);        // Atualiza o arquivo
              }
              else
              {
                putchar('\n'); // Imprime uma nova linha na saída padrão
                fflush(stdout);
              }
              i += strlen(&buffer[i]) - 1; // Pula os caracteres restantes após "ENTER" ou "RIGHTENTER"
            }
            else if (strcmp(&buffer[i], "BACKSPACE") == 0)
            {
              if (file)
              {
                // Volta um caractere no arquivo
                fseek(fp, -1, SEEK_CUR);
                // Trunca o arquivo na posição atual
                ftruncate(fileno(fp), ftell(fp));
                fflush(fp); // Atualiza o arquivo
              }
              else
              {
                // Volta um caractere na saída padrão
                putchar('\b');
                putchar(' ');
                putchar('\b');
                fflush(stdout);
              }
              i += strlen(&buffer[i]) - 1; // Pula os caracteres restantes após "BACKSPACE"
            }
            else if (strcmp(&buffer[i], "CAPSLOCK") == 0)
            {
              uppercase_mode = !uppercase_mode; // Alternar entre maiúsculo e minúsculo
              i += 7;
            }
            else
            {
              // Converter para maiúsculo ou minúsculo conforme a configuração
              if (uppercase_mode)
              {
                buffer[i] = toupper(buffer[i]); // Transformar em maiúsculo
              }
              else
              {
                buffer[i] = tolower(buffer[i]); // Transformar em minúsculo
              }

              if (file)
              {
                fprintf(fp, "%c", buffer[i]); // Escrever no arquivo de saída
                fflush(fp);                   // Atualiza o arquivo
              }
              else
              {
                printf("%c", buffer[i]); // Imprimir na saída padrão
                fflush(stdout);
              }
            }
          }
        }
        bytes_received = recv(new_fd, buffer, sizeof(buffer), 0);
      }

      close(new_fd); // Fecha o descritor de arquivo do cliente
      exit(0);       // Encerra o processo filho
    }
  }

  close(new_fd); // Fecha o descritor de arquivo do cliente

  if (file)
    fclose(fp); // Fecha o arquivo, se estiver aberto

  return 0;
}

// Imprime mensagem de uso correto do programa e encerra
void print_usage_and_quit(char *application_name)
{
  printf("Usage: %s [-s] filename\n", application_name);
  exit(1);
}
