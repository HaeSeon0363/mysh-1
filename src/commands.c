#include <stdio.h>
#include <signal.h>
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
#include <arpa/inet.h>
#include "commands.h"
#include "built_in.h"

#define FILE_SERVER "/tmp/test_server"

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

//int evaluate_command(int n_commands, struct single_command(*commands)[512]); 

void *multithreading(void *p_argv){
  int client_sock;
  int client_addr_size;
  int rc;
  struct sockaddr_un server_addr;
  struct single_command* first_com = (struct single_command *) p_argv;
  //struct single_command*  second_com = (struct single_command *) p_argv;
 
  //socket creation
  client_sock = socket(PF_FILE, SOCK_STREAM, 0);
  if(client_sock == -1){
    printf("CLIENT_SOCKET ERROR\n");
    exit(1);
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path, FILE_SERVER);
    
  //connection
  do
  {
    rc = connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (rc == -1) continue;
    if(fork()==0){
    printf("CONNECTION SUCCESS\n");
    dup2(client_sock,1);
    close(client_sock);
    exe_commands(first_com);
    //close(client_sock);
    exit(0);  
    }
    close(client_sock);
    wait(NULL);
    pthread_exit(0);
  }while(1);
 //close(client_sock);
  pthread_exit(0);
  return 0;
}

void *background_thread(void *argv){
  int status;
  do{
     if(waitpid(0,&status,WNOHANG)>0){
        fprintf(stderr,"%d finished %s\n", bg.pid, bg.argv);
        pthread_exit(0);
     }
  }while(1);
}


/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  int child_pid,pid;
  int status;
  int local=0;
  int server_sock, client_sock, client_addr_size, rc;
  int background;
  struct sockaddr_un server_addr;
  struct sockaddr_un client_addr;
  int backlog = 10;
  //struct single_command* com = (*commands); 
  // assert(com->argv != 0);

  if (n_commands == 1) { //non-pipe
    struct single_command* com = (*commands);
    return exe_commands(com);
  } else if(n_commands>1){   //pipe exists
  struct single_command* first_com = (*commands);
  struct single_command* second_com = (*commands+1); 
  pthread_t p_thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create (&p_thread,&attr,multithreading,(void *)first_com);
  if(access(FILE_SERVER,F_OK)==0){
     unlink(FILE_SERVER);
  }
  // printf("good file server\n");

  server_sock = socket(PF_FILE, SOCK_STREAM, 0);
  if(server_sock == -1){
    printf("SERVER_SOCKET ERROR\n");
    exit(1);
  }

  memset(&server_addr,0,sizeof(server_addr));
  server_addr.sun_family = AF_UNIX;
  strcpy(server_addr.sun_path,FILE_SERVER);

  if (bind(server_sock,(struct sockaddr*)&server_addr,sizeof(server_addr))==-1){
      printf("BIND ERROR\n");
      exit(1);
  }

    
  if (listen(server_sock,backlog)==-1){
     printf("LISTEN ERROR\n");
        //continue;
  }
  client_addr_size = sizeof(client_addr);
  do{
        client_sock = accept(server_sock,(struct sockaddr*)&client_addr,&client_addr_size);
        
     if (client_sock == -1){
       printf("CLIENT_SOCKET ERROR\n");
       continue;
     }
     break;
  } while(1);

  if(fork()==0){ 
     dup2(client_sock,0);
     close(client_sock);
      // close(server_sock);
     exe_commands(second_com);
        //close(client_sock);
        //close(server_sock);
     exit(1);
  }
  close(client_sock);
  wait(NULL);
 }
 return 0;
}

int exe_commands (struct single_command *com)
{
  int child_pid,status,i,bgpid;
  assert(com->argc != 0);
  
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
    if(strcmp(com->argv[com->argc-1],"&") == 0){
       bg.flag = 1;
       com->argv[com->argc-1]=NULL;
       com->argc = com->argc-1;
    } 
    if(bg.flag == 1){
       bg.argv=malloc(3);
       for (i=0;i<com->argc;i++){
         bg.argv = realloc(bg.argv,strlen(com->argv[i])+strlen(bg.argv));
         strcat(bg.argv,com->argv[i]);
         strcat(bg.argv," ");
       }
    }
  
    child_pid=fork();
    if (child_pid==0){
      child_pid=getpid();
      execv(com->argv[0],com->argv);
      fprintf(stderr, "%s : command not found\n", com->argv[0]);
      exit(1);
    }else{
      if(bg.flag == 1){
         pthread_t bg_thread;
         pthread_attr_t attr;
         pthread_attr_init(&attr);
         bg.pid = child_pid;
         bg.flag = 0;
         pthread_create(&bg_thread,&attr,background_thread, (void *)bg.argv); 
      }
      else wait(&status);
    }
    return 0;
  }  else {
    fprintf(stderr,"%s : command not found\n",com->argv[0]);
    return -1;
  }
  return 0;
}  

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
//  pthread_exit(NULL);
}
