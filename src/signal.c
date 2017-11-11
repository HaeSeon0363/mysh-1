#include "signal.h"

void catch_sigint(int)
{
  signal(SIGINT, (void*)sig_handler);
}

void catch_sigtstp(int);
{
  signal(SIGTSTP,(void*)sig_handler);
}
