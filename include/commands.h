#ifndef MYSH_COMMANDS_H_
#define MYSH_COMMANDS_H_

struct single_command
{
  int argc;
  char** argv;
};

struct background{
  int pid;
  int flag;
  char *argv;
}bg;

int evaluate_command(int n_commands, struct single_command (*commands)[512]);

void *background_thread(void *argv);
int exe_commands(struct single_command *com);

void free_commands(int n_commands, struct single_command (*commands)[512]);

#endif // MYSH_COMMANDS_H_
