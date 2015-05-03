// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "alloc.h"
#include "read-command.c"

#include <error.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


#define QueueSize 1000
#define ListSize 100
#define ArraySize 1000
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

typedef struct GraphNode{
    command_t command;
    struct GraphNode ** before;
    pid_t pid;
}GraphNode;

GraphNode* initGraphNode(){
    GraphNode* new = checked_malloc(sizeof(GraphNode));
    new->pid = -1;
    return new;
}

typedef struct{
    GraphNode* array[QueueSize];
    int front;
    int rear;
}QueueGraphNode;

typedef struct{
    QueueGraphNode *no_dependencies;
    QueueGraphNode *dependencies;
}DependencyGraph;

QueueGraphNode* initQueue(){
    QueueGraphNode* new = checked_malloc(sizeof(QueueGraphNode));
    new->front = 0;
    new->rear = -1;
    return new;
}

void QueueInsert(QueueGraphNode *queue, GraphNode *node){
    queue->rear += 1;
    queue->array[queue->rear] = node;
}

GraphNode* QueuePopFront(QueueGraphNode *queue){
    if (queue->rear-queue->front < 0)
        error(1, 0, "Cannot pop item in a empty queue");
    
    int popItem = queue->front;
    queue->front += 1;
    return queue->array[popItem];
}

int QueueLen(QueueGraphNode *queue){
    return (queue->rear-queue->front+1);
}

int writeList(command_t command, char** wl, int listPos){
    switch (command->type) {
        case SIMPLE_COMMAND:
            if (command->output != NULL){
                wl[listPos] = command->output;
                listPos++;
            }
            break;
        case SEQUENCE_COMMAND:
            listPos = writeList(command->u.command[0], wl, listPos);
            if (command->u.command[1]!= NULL)
                listPos = writeList(command->u.command[1], wl, listPos);
            break;
        case PIPE_COMMAND:
        case AND_COMMAND:
        case OR_COMMAND:
            listPos = writeList(command->u.command[0], wl, listPos);
            listPos = writeList(command->u.command[1], wl, listPos);
            break;
        case SUBSHELL_COMMAND:
            listPos = writeList(command->u.subshell_command, wl, listPos);
            break;
        default:
            break;
    }
    
    
    wl[listPos] = NULL;
    return listPos;
}

int readList(command_t command, char** rl, int listPos){
    switch (command->type) {
        case SIMPLE_COMMAND:
            if (command->input != NULL)
            {
                rl[listPos] = command->output;
                listPos++;
            }
            int i = 1;
            while (command->u.word[i] != NULL){
                if (command->u.word[i][2] == '\0')
                    continue;
                rl[listPos] = command->u.word[i];
                i++;
            }
            break;
        case SEQUENCE_COMMAND:
            listPos = readList(command->u.command[0], rl, listPos);
            if (command->u.word[1] != NULL)
                listPos = readList(command->u.command[1], rl, listPos);
            break;
        case PIPE_COMMAND:
        case AND_COMMAND:
        case OR_COMMAND:
            listPos = readList(command->u.command[0], rl, listPos);
            listPos = readList(command->u.command[1], rl, listPos);
            break;
        case SUBSHELL_COMMAND:
            listPos = readList(command->u.subshell_command, rl, listPos);
            break;
        default:
            break;
    }

    rl[listPos] = NULL;
    return listPos;
}

int Dependency(char** child, char** parent){
    int i= 0, j=0;
    while (parent[i] != NULL){
        while (child[j] != NULL){
            if (parent[i] == child[j])
                return 1;
            j++;
        }
        i++;
    }
    return 0;
}

DependencyGraph* createGraph(command_stream_t command_stream){
    //Initialize DependencyGraph
    DependencyGraph *new = checked_malloc(sizeof(DependencyGraph));
    new->no_dependencies = initQueue();
    new->dependencies = initQueue();
    
    
    GraphNode** nodes = checked_malloc(ArraySize*sizeof(GraphNode*));
    char*** writeLists = checked_malloc(ArraySize*sizeof(char**));
    char*** readLists = checked_malloc(ArraySize*sizeof(char**));
    int readListPos = 0;
    int writeListPos = 0;

    int i = 0;//i is index for command trees
    commandNode_t currentNode = command_stream->head;
    
    do{
        //for each command tree
        nodes[i]= initGraphNode();
        nodes[i]->command = currentNode->command;
        
        //WriteList and ReadList for each commandNode
        readListPos = 0;
        writeListPos = 0;
        readList(currentNode->command, readLists[i], readListPos);
        writeList(currentNode->command, writeLists[i], writeListPos);
        
        int j, k=0;
        for (j=i-1; j>=0; j--){
            //determine dependency
            if (Dependency(readLists[j], writeLists[i])){//WAR
                if (k==0)//assign memory
                    nodes[i]->before = checked_malloc(ArraySize*sizeof(GraphNode*));
                
                nodes[i]->before[k] = nodes[j];
                k++;
                continue;
            }

            if (Dependency(writeLists[j], readLists[i])){//RAW
                if (k==0)//assign memory
                    nodes[i]->before = checked_malloc(ArraySize*sizeof(GraphNode*));
                
                nodes[i]->before[k] = nodes[j];
                k++;
                continue;
            }
            
            if (Dependency(writeLists[j], writeLists[i])){//WAW
                if (k==0)//assign memory
                    nodes[i]->before = checked_malloc(ArraySize*sizeof(GraphNode*));
                
                nodes[i]->before[k] = nodes[j];
                k++;
            }


        }
        
        //add the graphNode to dependency queue
        if (nodes[i]->before == NULL){
            QueueInsert(new->no_dependencies, nodes[i]);
        }else{
            QueueInsert(new->dependencies, nodes[i]);
        }
        
        i++;
        currentNode = currentNode->next;
    }while (currentNode != NULL);//exit after traverse the last command tree
    
    
    return new;
}

void executeNoDependencies(QueueGraphNode *no_dependencies){
    int i;
    for (i=0; i< QueueLen(no_dependencies); i++)
    {
        GraphNode *node = QueuePopFront(no_dependencies);
        
        pid_t pid = fork();
        if (pid == 0){
            execute_command(node->command, true);
            exit(0);
        }else{
            node->pid = pid;
        }
    }
    return;
}

void executeDependencies(QueueGraphNode *dependencies){
    int i;
    for (i=0; i< QueueLen(dependencies); i++)
    {
        GraphNode *node = QueuePopFront(dependencies);
        
        if (node->before == NULL)
            error(1,0,"Non-dependent graphNode in dependencies list");
        
        int status;
        int j=0;
        while (node->before[j] != NULL){
            waitpid(node->before[j]->pid, &status, 0);
        }
        
        pid_t pid =fork();
        if (pid == 0){
            execute_command(node->command, true);
            exit(0);
        }else{
            node->pid = pid;
        }
    }
    return;
}

int executeGraph(DependencyGraph *graph){
    executeNoDependencies(graph->no_dependencies);
    executeDependencies(graph->dependencies);
    return 0;
}


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
