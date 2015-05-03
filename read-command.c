// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <stdio.h>
#include <error.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

//Implement stack
#define stackSize 1024
#define stringSize 1000

int lineNum = 1;

typedef struct stackCommand
{
    command_t array[stackSize];
    int top;
}stackCommand, *stackCommand_t;

void stackCommandPush (stackCommand_t st, command_t item)
{
    st->top++;
    st->array[st->top] = item;
}

int stackCommandEmpty(stackCommand_t st)
{
    if (st->top == -1)
        return 1;
    else
        return 0;
}

stackCommand_t stackCommandInit ()
{
    stackCommand_t new = checked_malloc(sizeof(stackCommand));
    new->top = -1;
    return new;
}

command_t stackCommandPop(stackCommand_t st)
{
    if (stackCommandEmpty(st))
    {
        error(1,0, "No item in stack, line: %d", lineNum);
    }
    command_t item = st->array[st->top];
    st->top--;
    return item;
}

command_t stackCommandTop (stackCommand_t st)
{
    return st->array[st->top];
}

// Several states when reading char by char
enum read_state
{
    NORMAL,
    LOOK_FOR_COMMAND,   //right after |, ||, &&
    LOOK_FOR_AND, //after &
    LOOK_FOR_OR, //after |
    COMMENT,    //after #
    START_COMMAND   //after ; /n
};


typedef struct commandNode
{
    struct command *command;
    struct commandNode *next;
}commandNode, *commandNode_t;

commandNode_t initCommandNode()
{
    commandNode_t new = checked_malloc(sizeof(commandNode));
    new->command = NULL;
    new->next = NULL;
    return new;
}

struct command_stream
{
    struct commandNode *head;
    struct commandNode *tail;
    struct commandNode *cursor;
};

command_stream_t initCommandStream()
{
    command_stream_t new = checked_malloc(sizeof(struct command_stream));
    new->head = NULL;
    new->tail = NULL;
    new->cursor = NULL;
    return new;
}

command_t initCommand ()
{
    command_t c = checked_malloc(sizeof(struct command));
    c->type = SIMPLE_COMMAND;
    c->status = -1;
    c->input = NULL;
    c->output = NULL;
    c->u.word = checked_malloc(stringSize*sizeof(char*));
    return c;
}

command_t initSequence()
{
    command_t c = checked_malloc(sizeof(struct command));
    c->type = SEQUENCE_COMMAND;
    c->status = -1;
    c->input = NULL;
    c->output = NULL;
    c->u.command[0] = NULL;
    c->u.command[1] = NULL;
    return c;
}

command_t initAnd ()
{
    command_t c = checked_malloc(sizeof(struct command));
    c->type = AND_COMMAND;
    c->status = -1;
    c->input = NULL;
    c->output = NULL;
    c->u.command[0] = NULL;
    c->u.command[1] = NULL;
    return c;
}

command_t initOr ()
{
    command_t c = checked_malloc(sizeof(struct command));
    c->type = OR_COMMAND;
    c->status = -1;
    c->input = NULL;
    c->output = NULL;
    c->u.command[0] = NULL;
    c->u.command[1] = NULL;
    return c;
}

command_t initPipe ()
{
    command_t c = checked_malloc(sizeof(struct command));
    c->type = PIPE_COMMAND;
    c->status = -1;
    c->input = NULL;
    c->output = NULL;
    c->u.command[0] = NULL;
    c->u.command[1] = NULL;
    return c;
}

command_t initSubshell ()
{
    command_t c = checked_malloc(sizeof(struct command));
    c->type = SUBSHELL_COMMAND;
    c->status = -1;
    c->input = NULL;
    c->output = NULL;
    c->u.subshell_command=NULL;
    return c;
}

int isNormalChar(char c)
{
    char *special = "!%+,-./:@^_ \t><";
    int result = isalnum(c);
    int i;
    for (i=0; i< (int)strlen(special); i++){
        result = result || (c==special[i]);
    }
    return result;
}

void addCharToString (char c, char* s)
{
    int len = (int)strlen(s);
    s[len] = c;
    s[len+1] = '\0';
}

void resetString(char* s)
{
    s[0]= '\0';
    return;
}

command_t stringToCommand (char* s)
{
    //s cannot be empty string
    command_t cmd = initCommand();
    int i, wordCount=0, k=0;
    int input = -1, output = -1;
    cmd->u.word[wordCount] = checked_malloc(stringSize*sizeof(char));
    
    for (i=0;i< (int)strlen(s);i++)
    {
        switch (s[i])
        {
            case ' ':
                if (cmd->u.word[wordCount][0] != '\0')
                {
                    wordCount++;
                    cmd->u.word[wordCount] = checked_malloc(stringSize*sizeof(char));
                    output = -1;
                    input = -1;
                    k = 0;
                }
                break;
            case '>':
                cmd->output =  checked_malloc(stringSize * sizeof(char));
                if (output != -1)
                {
                    //return error
                    error(1,0, "multiple > in a command, line: %d", lineNum);
                }
                if (cmd->u.word[wordCount][0] != '\0')
                {
                    wordCount++;
                    cmd->u.word[wordCount] = checked_malloc(stringSize*sizeof(char));
                    k = 0;
                }
                input = -1;
                output++;
                break;
            case '<':
                cmd->input = checked_malloc(stringSize*sizeof(char));
                if (input != -1)
                {
                    //return error
                    error(1,0, "multiple < in a command, line: %d", lineNum);
                }
                if (cmd->u.word[wordCount][0] != '\0')
                {
                    wordCount++;
                    cmd->u.word[wordCount] = checked_malloc(stringSize*sizeof(char));
                    k = 0;
                }
                output = -1;
                input++;
                break;
            default:
                if (input == -1 && output == -1)
                {
                    cmd->u.word[wordCount][k] = s[i];
                    k++;
                }
                else
                {
                    if (input >= 0 && output == -1)
                    {
                        cmd->input[input] = s[i];
                        input++;
                    }
                    else if (input == -1 && output >= 0)
                    {
                        cmd->output[output] = s[i];
                        output++;
                    }
                }
                break;
        }
    }
    
    if (cmd->input != NULL && cmd->input[0]=='\0')
        error(1,0, "multiple < in a command, line: %d", lineNum);
    
    if (cmd->output != NULL && cmd->output[0]=='\0')
        error(1,0, "multiple < in a command, line: %d", lineNum);
    
    if (cmd->u.word[0][0]=='\0')
        error(1,0, "no word detected in command, line: %d", lineNum);
    
    if (cmd->u.word[wordCount][0] == '\0')
        cmd->u.word[wordCount] = NULL;
    else
        cmd->u.word[wordCount+1] = NULL;
    return cmd;
}


int precedenceOfCmdType (enum command_type type)
{
    if (type == AND_COMMAND || type == OR_COMMAND)
        return 2;
    else if (type == PIPE_COMMAND)
        return 3;
    else if (type == SEQUENCE_COMMAND)
        return 1;
    else if (type == SUBSHELL_COMMAND)
        return 0;
    else
        return -1;
}

void stackPushToStack(stackCommand_t stackCmd, stackCommand_t stackOper, command_t command)
{
    // DOES NOT APPLY TO commands contain '('
    if (command->type ==SIMPLE_COMMAND)
    {
        stackCommandPush(stackCmd, command);
    }
    
    else if (stackCommandEmpty(stackOper))
    {
        stackCommandPush(stackOper, command);
    }
    
    else if (command->type != SUBSHELL_COMMAND)
    {
        int operatorPriority = precedenceOfCmdType(command->type);
        while (operatorPriority <= precedenceOfCmdType(stackCommandTop(stackOper)->type) && !stackCommandEmpty(stackOper))
        {
            command_t popOperator = stackCommandPop(stackOper);
            
            if (stackCmd->top < 1)
            {
                error(1,0, "error in commands1, line: %d", lineNum);
                return;
            }
            
            command_t popCmdR = stackCommandPop(stackCmd);
            command_t popCmdL = stackCommandPop(stackCmd);
            
            popOperator->u.command[1] = popCmdR;
            popOperator->u.command[0] = popCmdL;
            stackCommandPush(stackCmd, popOperator);
            if (stackCommandEmpty(stackOper))
                break;
        }
        stackCommandPush(stackOper, command);
    }
    
    else if (command->type == SUBSHELL_COMMAND)
    {
        if (stackOper->top ==0 &&stackCommandTop(stackOper)->type != SUBSHELL_COMMAND)
        {
            error(1,0, "error in commands2, line: %d", lineNum);
            return;
        }
        if (stackCommandEmpty(stackOper))
        {
            error(1,0, "error in commands3, line: %d", lineNum);
            return;
        }
        
        while (stackCommandTop(stackOper)->type != SUBSHELL_COMMAND)
        {
            command_t popOperator = stackCommandPop(stackOper);
            if (stackOper->top == 0 && stackCommandTop(stackOper)->type != SUBSHELL_COMMAND)
            {
                error(1,0, "error in commands4, line: %d", lineNum);
                return;
            }
            
            if (stackCmd->top < 1)
            {
                error(1,0, "error in commands5, line: %d", lineNum);
                return;
            }
            
            command_t popCmdR = stackCommandPop(stackCmd);
            command_t popCmdL = stackCommandPop(stackCmd);
            
            popOperator->u.command[1] = popCmdR;
            popOperator->u.command[0] = popCmdL;
            stackCommandPush(stackCmd, popOperator);
        }
        command->u.subshell_command = stackCommandPop(stackCmd);
        stackCommandPush(stackCmd, command);
    }
    else
    {
        error(1,0, "error in commands, line: %d", lineNum);
    }
}

//Result will be saved in the only element in stackCmd
void stackCombine (stackCommand_t stackCmd, stackCommand_t stackOper)
{
    if (stackOper ->top < 0 && stackCmd->top != 0)
    {
        error(1,0, "error in commands6, line: %d", lineNum);
        return;
    }
    
    while (stackOper->top >= 0)
    {
        command_t popOperator = stackCommandPop(stackOper);
        if (stackCmd->top < 1)
        {
            error(1,0, "error in commands7, line: %d", lineNum);
            return;
        }
        
        command_t popCmdR = stackCommandPop(stackCmd);
        command_t popCmdL = stackCommandPop(stackCmd);
        
        popOperator->u.command[1] = popCmdR;
        popOperator->u.command[0] = popCmdL;
        stackCommandPush(stackCmd, popOperator);
    }
    
    if (stackCmd->top != 0 || !stackCommandEmpty(stackOper))
    {
        error(1,0, "error in commands8, line: %d %d, %d", lineNum, stackCmd->top, stackOper->top);
        return;
    }
}
//-p test.sh > test.out 2> test.err

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
    // Init command_stream_t
    command_stream_t commandStream = initCommandStream();
    commandNode_t curNode = initCommandNode();
    
    commandStream->head = curNode;
    commandStream->tail = curNode;
    commandStream->cursor = curNode;
    
    // Get & Separate each command
    enum read_state curState = START_COMMAND;
    enum read_state prevState = START_COMMAND;
    char cget;
    char* buffer = checked_malloc(stringSize*sizeof(char));
    stackCommand_t stackCmd =stackCommandInit();
    stackCommand_t stackOper = stackCommandInit();
    
    int newlineCount = 0;   //number of continuous new lines
    
    do
    {
        cget = get_next_byte(get_next_byte_argument);
        if (cget == EOF)
        {
            if (buffer[0] != '\0')
            {
                command_t curCommand = stringToCommand(buffer);
                stackPushToStack(stackCmd, stackOper,curCommand);
                resetString(buffer);
            }
        }
        else if (cget == '#' && curState != COMMENT)
        {
            prevState = curState;
            curState = COMMENT;
            continue;
        }

        else if (curState == COMMENT)
        {
            if (cget == '\n')
            {
                lineNum++;
                curState = prevState;
                prevState = COMMENT;
                if (curState != LOOK_FOR_COMMAND)
                {
                    newlineCount++;
                    if (newlineCount == 2 && stackCmd->top != -1)
                    {
                        // start a new commandNode
                        stackCombine(stackCmd, stackOper);
                        curNode->command = stackCommandPop(stackCmd);
                        //two stacks are all empty now & can reuse without reset
                        commandNode_t new = initCommandNode();
                        curNode->next = new;
                        curNode= new;
                    }
                }
            }
        }
        
        else if (curState ==LOOK_FOR_AND)
        {
            if (cget != '&')
            {
                //return error
                error(1,0, "error in commands9, line: %d", lineNum);
                return 0;
            }
            else
            {
                prevState = curState;
                curState = LOOK_FOR_COMMAND;
                command_t and = initAnd();
                stackPushToStack(stackCmd,stackOper, and);
            }
        }
        
        else if (curState == LOOK_FOR_OR)
        {
            if (cget == '|')
            {
                prevState = curState;
                curState = LOOK_FOR_COMMAND;
                command_t or = initOr();
                stackPushToStack(stackCmd, stackOper, or);
            }
            else if (isNormalChar(cget) && cget !=' ' && cget != '\t' && cget != '\n')
            {
                prevState = LOOK_FOR_COMMAND;
                curState = NORMAL;

                command_t pipe = initPipe();
                stackPushToStack(stackCmd, stackOper, pipe);
                
                resetString(buffer);
                addCharToString(cget, buffer);
            }
            else if (cget ==' ' || cget =='\t' ||cget == '\n')
            {
                if (cget == '\n')
                    lineNum++;
                prevState = LOOK_FOR_COMMAND;
                curState = LOOK_FOR_COMMAND;
                command_t pipe = initPipe();
                stackPushToStack(stackCmd, stackOper, pipe);
            }
            else
            {
                error(1,0, "error in commands10, line: %d", lineNum);
                return 0;
            }
        }
        
        else if (curState == LOOK_FOR_COMMAND)
        {
            newlineCount = 0;
            if (isNormalChar(cget))
            {
                resetString(buffer);
                addCharToString(cget, buffer);
                prevState = curState;
                curState = NORMAL;
            }
            else if (cget ==' '||cget =='\t' || cget =='\n')
            {
                if (cget == '\n')
                    lineNum++;
                continue;
            }
            else if (cget == '(')
            {
                command_t subshell = initSubshell();
                stackCommandPush(stackOper, subshell);
            }
            else if (cget == ')')
            {
                //CAUTION: need to work
                error(1,0, "error in commands11, line: %d", lineNum);
            }
            else{
                error(1,0, "error in commands12, line: %d", lineNum);
            }
        }
        
        else if (curState == START_COMMAND)
        {
            if ( isNormalChar(cget) && cget != ' ')
            {
                newlineCount = 0;   //reset counter
                resetString(buffer);
                addCharToString(cget, buffer);
                
                prevState = curState;
                curState = NORMAL;
            }
            else if (cget == '\n'|| cget =='\t'|| cget == ' ')
            {
                if (cget == '\n')
                {
                    lineNum++;
                    newlineCount++;
                    if (newlineCount == 2 && stackCmd->top != -1)
                    {
                        // start a new commandNode
                        stackCombine(stackCmd, stackOper);
                        curNode->command = stackCommandPop(stackCmd);
                        //two stacks are all empty now & can reuse without reset
                        commandNode_t new = initCommandNode();
                        curNode->next = new;
                        curNode= new;
                    }
                }
                continue;
            }
            else if (cget == ';')
            {
                error(1,0, "error in commands13, line: %d", lineNum);
            }
            else if (cget == '(' || cget == ')')
            {
                command_t subshell = initSubshell();
                if (cget == '(')
                {
                    stackCommandPush(stackOper, subshell);
                }
                else
                {
                    stackPushToStack(stackCmd, stackOper, subshell);
                }
            }
            else
                error(1,0, "error in commands13, line: %d", lineNum);
        }
        
        else if (curState == NORMAL)
        {
            if (isNormalChar(cget))
            {
                newlineCount = 0;
                addCharToString(cget, buffer);
            }
            else
            {
                command_t curCommand = stringToCommand(buffer);
                
                switch(cget)
                {
                    case '#':
                        prevState = curState;
                        curState = COMMENT;
                        
                        if (buffer[0] != '\0')
                        {
                            command_t curCommand = stringToCommand(buffer);
                            stackPushToStack(stackCmd, stackOper,curCommand);
                            resetString(buffer);
                        }
                        break;
                        
                    case '&':
                        prevState = curState;
                        curState = LOOK_FOR_AND;
                        
                        if (newlineCount != 0)
                            error(1,0, "error in commands17, line: %d", lineNum);
                        
                        if (buffer[0] != '\0')
                        {
                            command_t curCommand = stringToCommand(buffer);
                            stackPushToStack(stackCmd, stackOper,curCommand);
                            resetString(buffer);
                        }
                        break;
                        
                    case '|':
                        if (newlineCount != 0)
                            error(1,0, "error in commands18, line: %d", lineNum);

                        prevState = curState;
                        curState = LOOK_FOR_OR;
                        
                        if (buffer[0] != '\0')
                        {
                            command_t curCommand = stringToCommand(buffer);
                            stackPushToStack(stackCmd, stackOper,curCommand);
                            resetString(buffer);
                        }
                        else
                        {
                            error(1,0, "error in commands14, line: %d", lineNum);
                        }
                        break;
                        
                    case '\n':
                        lineNum++;
                        newlineCount++;
                        if (buffer[0] != '\0')
                        {
                            command_t curCommand = stringToCommand(buffer);
                            stackPushToStack(stackCmd, stackOper,curCommand);
                            resetString(buffer);
                        }
                        prevState = curState;
                        curState = START_COMMAND;
                        
                        if (newlineCount == 2)
                        {
                            stackCombine(stackCmd, stackOper);
                            curNode->command = stackCommandPop(stackCmd);
                            
                            commandNode_t new = initCommandNode();
                            curNode->next = new;
                            curNode = new;
                        }
                        break;
                        
                    case ';':
                        if (newlineCount != 0)
                            error(1,0, "error in commands19, line: %d", lineNum);

                        if (!stackCommandEmpty(stackOper))
                        {
                            if (stackCommandTop(stackOper)->type == SEQUENCE_COMMAND && stackCommandTop(stackOper)->u.command[0]==NULL && stackCommandTop(stackOper)->u.command[1]==NULL)
                            {
                                continue;
                            }
                        }
                        prevState = curState;
                        curState = START_COMMAND;
                        
                        
                        if (buffer[0] != '\0')
                        {
                            command_t curCommand = stringToCommand(buffer);
                            stackPushToStack(stackCmd, stackOper,curCommand);
                            resetString(buffer);
                        }
                        
                        //push ; to oeprator stack
                        command_t sequence = initSequence();
                        stackPushToStack(stackCmd, stackOper, sequence);
                        break;
                    case '(':
                        error(1,0, "error in commands15, line: %d", lineNum);
                        break;
                    case ')':
                        newlineCount = 0;
                        if (buffer[0] != '\0')
                        {
                            stackPushToStack(stackCmd, stackOper,curCommand);
                        }
                        
                        //push ) to operator stack
                        command_t subshell = initSubshell();
                        stackPushToStack(stackCmd, stackOper, subshell);
                        break;
                    default:
                        error(1,0, "error in commands16, line: %d", lineNum);
                }
            }
        }
    }while(cget !=EOF);
    
    //Combine two stacks
    if (stackCmd->top != -1)
    {
        stackCombine(stackCmd, stackOper);
        curNode->command = stackCommandPop(stackCmd);
        commandStream->tail = curNode;
    }
    
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    return commandStream;
}

command_t
read_command_stream (command_stream_t s)
{
    if (s->cursor == NULL)
        return NULL;
    commandNode_t temp = s->cursor;
    s->cursor = s->cursor->next;
    return temp->command;
}
