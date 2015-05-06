// UCLA CS 111 Lab 1 command internals
#define QueueSize 1000

enum command_type
  {
    AND_COMMAND,         // A && B
    SEQUENCE_COMMAND,    // A ; B
    OR_COMMAND,          // A || B
    PIPE_COMMAND,        // A | B
    SIMPLE_COMMAND,      // a simple command
    SUBSHELL_COMMAND,    // ( A )
  };

// Data associated with a command.
struct command
{
  enum command_type type;

  // Exit status, or -1 if not known (e.g., because it has not exited yet).
  int status;

  // I/O redirections, or null if none.
  char *input;
  char *output;

  union
  {
    // for AND_COMMAND, SEQUENCE_COMMAND, OR_COMMAND, PIPE_COMMAND:
    struct command *command[2];

    // for SIMPLE_COMMAND:
    char **word;

    // for SUBSHELL_COMMAND:
    struct command *subshell_command;
  } u;
};

struct command_stream
{
    struct commandNode *head;
    struct commandNode *tail;
    struct commandNode *cursor;
};

struct commandNode
{
    struct command *command;
    struct commandNode *next;
};

struct DependencyGraph
{
    QueueGraphNode_t no_dependencies;
    QueueGraphNode_t dependencies;
};

struct QueueGraphNode{
    GraphNode_t array[QueueSize];
    int front;
    int rear;
};

struct GraphNode{
    command_t command;
    struct GraphNode ** before;
    pid_t pid;
};
