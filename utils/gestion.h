#ifndef GESTION_H
#define GESTION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Déclaration de la fonction gestion_cmd
void gestion_cmd(char *input, char **arg, char **cmd);

int fsh(char *cmd, char *arg, char *input, char *chemin, int dernier_exit,int ret);

#endif // GESTION_H