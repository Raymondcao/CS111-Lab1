// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
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
                error("error in creating new output file", 0);
            
            dup2(fd, 1);
            close(fd);
        }
        if (c->input != NULL)
        {
            int fd = open(c->input, O_RDONLY);
            if (fd < 0)
                error("can't find input file", 0);
            dup2(fd, 0);
            close(fd);
        }
        execvp(arg[0], arg);
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        c->status = WEXITSTATUS(status);
    }
    else
        error("fork error", 0);
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
        close(fd[0]);
        dup2(fd[0], 0);
        execute_command(c->u.command[1], 0);
    }
    else
    {
        int secondPid = fork();
        if (secondPid == 0)
        {
            close(fd[0]);
            dup2(fd[1], 1);
            execute_command(c->u.command[0], 0);
        }
        else
        {
            close(fd[0]);
            close(fd[1]);
            int status;
            int returnedPid = waitpid(-1, &status, 0);
            if (returnedPid == secondPid)
                waitpid(firstPid, &status, 0);
            if (returnedPid == firstPid)
                waitpid(secondPid, &status, 0);
        }
    }
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
            
            break;
        case OR_COMMAND:
            
            break;
        case SEQUENCE_COMMAND:
            
            break;
        case PIPE_COMMAND:
            
            break;
        case SUBSHELL_COMMAND:
            
            break;
        default:
            error("No command type", 0);
    }
    
}
