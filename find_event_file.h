#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

/**
 * Tenta encontrar o diretorio de um teclado conectado
 * \return diretorio do teclado alocado na memoria, ou NULL se nenhum foi
 *         detectado
 */
char *get_keyboard_event_file(void);
