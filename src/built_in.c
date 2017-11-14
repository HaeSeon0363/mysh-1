#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>
#include "built_in.h"
#include "commands.h"

int do_cd(int argc, char** argv) {
  if (!validate_cd_argv(argc, argv))
    return -1;

  if (chdir(argv[1]) == -1)
    return -1;

  return 0;
}

int do_pwd(int argc, char** argv) {
  if (!validate_pwd_argv(argc, argv))
    return -1;

  char curdir[PATH_MAX];

  if (getcwd(curdir, PATH_MAX) == NULL)
    return -1;

  printf("%s\n", curdir);

  return 0;
}

int do_fg(int argc, char** argv) {
  if (!validate_fg_argv(argc, argv))
    return -1;
 
  int status, chstatus;
  
  chstatus = waitpid(0,&status,WNOHANG);
  if(chstatus==0){
    fprintf(stderr,"%d running %s\n", bg.pid,bg.argv);
    waitpid(bg.pid,&status,0);
  }
  else if (chstatus > 0){
    fprintf(stderr,"%d done %s\n", bg.pid,bg.argv);
  }
  else return 0;

  return 0;
}

int validate_cd_argv(int argc, char** argv) {
  if (argc != 2) return 0;
  if (strcmp(argv[0], "cd") != 0) return 0;

  struct stat buf;
  stat(argv[1], &buf);

  if (!S_ISDIR(buf.st_mode)) return 0;

  return 1;
}

int validate_pwd_argv(int argc, char** argv) {
  if (argc != 1) return 0;
  if (strcmp(argv[0], "pwd") != 0) return 0;

  return 1;
}

int validate_fg_argv(int argc, char** argv) {
  if (argc != 1) return 0;
  if (strcmp(argv[0], "fg") != 0) return 0;
  
  return 1;
}

int pathresolution(char** argv)
{

  char *path[5] = {"/usr/local/bin/","/usr/bin/","/bin/","/usr/sbin/","/sbin/"};

  int i;

  if (access(argv[0],X_OK) == 0 ) return 1;
  for (i=0;i<5;i++)
  {
    char *token;
    token=malloc(strlen(path[i])+strlen(argv[0]));
    strcpy(token,path[i]);
    strcat(token,argv[0]);
    if (access(token,X_OK) ==0)
    {
      strcpy(argv[0],token);
      return 1;
    }
  }
  return 0;

}
