#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "commands.h"
#include "built_in.h"

#define SERVER_PATH "tpf_unix_sock.server"
#define CLIENT_PATH "tpf_unix_sock.client"

static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};



static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }
  return -1; // Not found
}


int evaluate_command(int n_commands, struct single_command(*commands)[512]); 

void *multithreading(void *p_argv){
  
  char **parse_com = ((char **)p_argv);
  int ti = 0;

  for(int i=0; parse_com[i] != NULL; i++ ){
     ti++;
  } 

  struct single_command single_commands[512] = {ti, parse_com};

 // struct single_command* com1 = (struct single_command*)commandpart;
 // single_commands[0] = *com1;
  int client_sock;
  int rc,len;
  struct sockaddr_un server_sockaddr;
  struct sockaddr_un client_sockaddr;
  char buf[256];

  memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
  memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));

  //socket creation
  client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
  if(client_sock == -1){
    printf("CLIENT_SOCKET ERROR\n");
    exit(1);
  }

  //close(STDIN_FILENO);
  
  client_sockaddr.sun_family = AF_UNIX;
  strcpy(client_sockaddr.sun_path, CLIENT_PATH);
  len = sizeof(client_sockaddr);

/*  int stdo = dup(STDOUT_FILENO);
  rc = dup2(client_sock,STDOUT_FILENO);
  if(rc == -1){
      printf("dup2 failed\n");
      exit(1);
  }

  evaluate_command(1, &single_commands);
  close(STDOUT_FILENO);
  dup2(stdo,STDOUT_FILENO);
*/

  server_sockaddr.sun_family = AF_UNIX;
  strcpy(server_sockaddr.sun_path, SERVER_PATH);
  rc = connect(client_sock, (struct sockaddr *) &server_sockaddr, len);
  if(rc == -1){
     printf("CONNECTION ERROR\n");
     close(client_sock);
     exit(1);
  }
  printf("connection success");

  sleep(3);
  
  dup2(client_sock, 1);
  close(client_sock);

  evaluate_command(1, &single_commands);

  sleep(3);
  close(client_sock);
  pthread_exit(NULL);
}


/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  int child_pid;
  int status;
  int local=0;
  int server_sock, client_sock, len, rc;
  struct sockaddr_un server_sockaddr;
  struct sockaddr_un client_sockaddr;
  int backlog = 10;
  long t;
  void *p_status;
  
  struct single_command* com = (*commands); 
  assert(com->argv != 0);

  if (n_commands == 1) { //non-pipe
  //  struct single_command* com = (*commands);
   // assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }
    } else if (strcmp(com->argv[0], "") == 0) {
      return 0;
    } else if (strcmp(com->argv[0], "exit") == 0) {
      return 1;
    } else if (pathresolution(com->argv) == 1 ){  //path resolution
      child_pid=fork();
      if (child_pid==0)
      {
        execv(com->argv[0],com->argv);
        fprintf(stderr, "%s : command not found\n", com->argv[0]);
        exit(1);
      }
      else
      {
        wait(&status);
        return 0;
      }
    } else {
      fprintf(stderr,"%s : command not found\n",com->argv[0]);
      return -1;
    }
  } else if(n_commands>1){   //pipe exists

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(server_sock == -1){
      printf("SERVER_SOCKET ERROR\n");
      exit(1);
    }
    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, SERVER_PATH);
    len = sizeof(server_sockaddr);
    unlink(SERVER_PATH);

    //bind- socket addressing
    rc = bind(server_sock, (struct sockaddr *) &server_sockaddr, len);
    if(rc == -1){
      printf("SERVER_BIND ERROR\n");
      close(server_sock);
      exit(1);
    }
    printf("bind success\n");
    //server listen -  socket queue creation 
    rc == listen(server_sock, backlog);
    if(rc == -1){
      printf("SERVEr_LISTEN ERROR\n");
      close(server_sock);
      exit(1);
    }
    printf("binding & listening");
     
    //client thread making
    pthread_t threads[5];
    int status;

    /*int check;
    long t;
    struct single_command single_commands[512];
    struct single_command *com1 = (*commands);
    struct single_command *com2 = &(*commands)[1];
    single_commands[0] = *com2;*/

    rc = pthread_create(&threads[0], NULL, multithreading, (void*)com->argv);
    
    if(rc == -1){
      printf("PTHREAD CREATION ERROR \n");
      exit(1);
    }
    printf("creating thread error");
      
    /*block until all thread complete*/
   // pthread_join(cl_thread, NULL);
     
    while(1){
 
       client_sock = accept(server_sock, (struct sockaddr*)&client_sockaddr, &len);
       if(client_sock == -1){
           printf("CLIENT ACCEPT ERROR");
          // close(server_sock);
         //  close(client_sock);
           exit(1);
       }
       printf("accept success");

       pid_t child_pid;
       int child_status;
       
       child_pid =fork();

       if(child_pid<0){
           printf("fork error");
          exit(1);
       }else if(child_pid == 0){
          dup2(client_sock, STDIN_FILENO);
          printf("duped passed\n");   	
          close(client_sock);
   	  execv((com+1)->argv[0], (com+1)->argv);
          fprintf(stderr, "%s : command not found\n", (com+1)->argv[0]);
       }else{
           close(client_sock);
           wait(&child_status);
           return 0;
       }
       pthread_join(threads[0], (void**)&status);
    }
    close(server_sock);
    close(client_sock);
  
    return 0;
  }
  return 0;
}

      /* len = sizeof(client_sockaddr);
       rc = getpeername(client_sock, (struct sockaddr*)&client_sockaddr, &len);
       if(rc == -1){
           printf("GETPEERNAME ERROR\n");
           close(server_sock);
           close(client_sock);
           exit(1);
       }

       int fd;
       close(STDOUT_FILENO);
       fd = dup(STDIN_FILENO);
       dup2(client_sock,STDIN_FILENO);
       
       evaluate_command(1, &single_commands);
       close(STDIN_FILENO);
       dup2(fd, STDIN_FILENO);
    }
    close(server_sock);
    close(client_sock);
  }
  return 0;
}*/



void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }
    free(argv);
  }
  memset((*commands), 0, sizeof(struct single_command) * n_commands);
  //pthread_exit(NULL);
}
