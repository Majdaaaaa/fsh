#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include "../../utils/commande.h"
#include "../../utils/gestion.h"
#include "../../utils/for.h"
#define ARG_MAX 512

void freeCmdStruct(commandeStruct *cmd);

void freeCmdSimple(cmd_simple *cmd)
{
    if (cmd == NULL)
        return;

    if (cmd->args != NULL)
    {
        for (char **arg = cmd->args; *arg; ++arg)
        {
            if (*arg != NULL)
            {
                free(*arg);
                *arg= NULL;
            }
        }
        free(cmd->args);
        cmd->args = NULL;
    }
    free(cmd);
}

void free_redirection(cmd_redirection *cmd)
{
    if (!cmd)
    {
        return;
    }
    else
    {
        if (cmd->cmd != NULL)
        {
            freeCmdSimple(cmd->cmd);
        }
        free(cmd);
    }
}

void free_pipe(cmd_pipe *cmd)
{
    for (int i = 0; i < cmd->nbCommandes; i++)
    {
        if (cmd->commandes[i] != NULL)
        {
            freeCmdSimple(cmd->commandes[i]);
            cmd->commandes[i] = NULL;
        }
    }
    free(cmd);
}

void free_for(cmdFor *cmdFor)
{
    if (cmdFor != NULL)
    {
        if (cmdFor->rep != NULL)
        {
            free(cmdFor->rep);
            cmdFor->rep = NULL; // ! jsp
        }
        if (cmdFor->variable != NULL)
        {
            free(cmdFor->variable);
            cmdFor->variable = NULL;
        }
        if (cmdFor->op != NULL)
        {
            int i = 0;
            while (cmdFor->op[i] != NULL)
            {
                free(cmdFor->op[i]);
                cmdFor->op[i]= NULL;
                i++;
            }
            free(cmdFor->op);
            cmdFor->op = NULL;
        }
        if (cmdFor->cmd != NULL)
        {
            int i = 0;
            while (cmdFor->cmd[i] != NULL)
            {
                freeCmdStruct(cmdFor->cmd[i]);
                cmdFor->cmd[i] = NULL;
                i++;
            }
            cmdFor->cmd[i] = NULL;
            free(cmdFor->cmd);
            cmdFor->cmd = NULL;
        }
        free(cmdFor);
        cmdFor = NULL;
    }
    return;
}

void freeCmdStruct(commandeStruct *cmd)
{
    if (cmd != NULL)
    {
        if (cmd->cmdSimple != NULL)
        {
            freeCmdSimple(cmd->cmdSimple);
            cmd->cmdSimple = NULL;
        }

        if (cmd->pipe != NULL)
        {
            free_pipe(cmd->pipe);
            cmd->pipe = NULL;
        }

        if (cmd->cmdFor != NULL)
        {
            free_for(cmd->cmdFor);
            cmd->cmdFor = NULL;
        }
        free(cmd);
        cmd= NULL;
    }
}
