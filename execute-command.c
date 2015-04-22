// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}

void execute_simple_command(command_t c)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        char** arg = c->u.word;
        if (c->output != NULL)
        {
            int fd = open(c->output, O_CREAT | O_TRUNC | O_WRONLY, 0644);
            if (fd < 0)
                error(1, 0, "error in creating new output file");
            
            dup2(fd, 1);
            close(fd);
        }
        if (c->input != NULL)
        {
            int fd = open(c->input, O_RDONLY);
            if (fd < 0)
                error(1, 0, "can't find input file");
            dup2(fd, 0);
            close(fd);
        }
        execvp(arg[0], arg);
        exit(c->status);
    }
    else if (pid > 0)
    {
        int status;
        if(waitpid(pid, &status, 0)== -1)
            error(1, 0, "Child process doesn't stop");
        c->status = WEXITSTATUS(status);
    }
    else
        error(1, 0, "fork error");
}

void execute_and_command(command_t c)
{
    execute_command(c->u.command[0], 0);
    if (c->u.command[0]->status == 0)
    {
        execute_command(c->u.command[1], 0);
        c->status = c->u.command[1]->status;
    }
    else
        c->status = c->u.command[0]->status;
}

void execute_or_command(command_t c)
{
    execute_command(c->u.command[0], 0);
    if (c->u.command[0]->status != 0)
    {
        execute_command(c->u.command[1], 0);
        c->status = c->u.command[1]->status;
    }
    else
        c->status = c->u.command[0]->status;
}

void execute_subshell_command(command_t c)
{
    execute_command(c->u.subshell_command, 0);
    c->status = c->u.subshell_command->status;
}

void execute_sequence_command(command_t c)
{
    execute_command(c->u.command[0], 0);
    execute_command(c->u.command[1], 0);
    c->status = c->u.command[1]->status;
}

void execute_pipe_command(command_t c)
{
    int fd[2];
    pipe(fd);
    int firstPid = fork();
    
    if (firstPid == 0)
    {
        close(fd[1]);
        dup2(fd[0], 0);
        execute_command(c->u.command[1], 0);
        close(fd[0]);
        exit(c->u.command[1]->status);
    }
    else if (firstPid > 0)
    {
        int secondPid = fork();
        if (secondPid == 0)
        {
            close(fd[0]);
            dup2(fd[1], 1);
            execute_command(c->u.command[0], 0);
            close(fd[1]);
            exit(c->u.command[0]->status);
        }
        else if (secondPid > 0)
        {
            close(fd[0]);
            close(fd[1]);
            int status;
            int returnedPid = waitpid(-1, &status, 0);
            if (returnedPid == secondPid)
            {
                waitpid(firstPid, &status, 0);
                c->status = WEXITSTATUS(status);
                return;
            }
            else if (returnedPid == firstPid)
            {
                waitpid(secondPid, &status, 0);
                c->status = WEXITSTATUS(status);
                return;
            }
            else
                error(1, 0, "forking error");
        }
        else
            error(1, 0, "forking error");
    }
    else
        error(1, 0 ,"forking error");
}

void
execute_command (command_t c, bool time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    switch (c->type) {
        case SIMPLE_COMMAND:
            execute_simple_command(c);
            break;
        case AND_COMMAND:
            execute_and_command(c);
            break;
        case OR_COMMAND:
            execute_or_command(c);
            break;
        case SEQUENCE_COMMAND:
            execute_sequence_command(c);
            break;
        case PIPE_COMMAND:
            execute_pipe_command(c);
            break;
        case SUBSHELL_COMMAND:
            execute_subshell_command(c);
            break;
        default:
            error(1, 0, "No command type");
    }
    
}
