/*----------------------------------------------------------------------
     ,'""`.      ,'""`.      ,'""`.      ,'""`.      ,'""`.
    / _  _ \    / _  _ \    / _  _ \    / _  _ \    / _  _ \
    |(@)(@)|    |(@)(@)|    |(@)(@)|    |(@)(@)|    |(@)(@)|
    )  __  (    )  __  (    )  __  (    )  __  (    )  __  (
   /,'))((`.\  /,'))((`.\  /,'))((`.\  /,'))((`.\  /,'))((`.\
  (( ((  )) ))(( ((  )) ))(( ((  )) ))(( ((  )) ))(( ((  )) ))
   `\ `)(' /'  `\ `)(' /'  `\ `)(' /'  `\ `)(' /'  `\ `)(' /'

----------------------------------------------------------------------*/

//OSX compile with:
//gcc cmd.c extras/*.c -o cmd

//Linux compile with:
//gcc cmd.c extras/*.c -lm -lpthread -o cmd

int  system2(const char * command, int * infp, int * outfp);
void i_hate_canonical_input_processing(void);
void make_stdin_cannonical_again();

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
//todo: make this into its own class in a separate file
#define  OSC_BUFFER_SIZE 512
#define  OSC_VALUES_BUFFER_SIZE 64
#define  OSC_SEND_PORT   7003
#define  OSC_RECV_PORT   7002

#include <stdlib.h> //system()
#include <unistd.h> //sleep()
#include <stdio.h> //sleep()
#include <fcntl.h> //open close
#include <signal.h> //kill
#include <sys/types.h>
#include <unistd.h> //fork()
#include <sys/wait.h> //wait();
#include <pthread.h>
#include "extras/Network.h"
#include "extras/OSC.h"

#define PROGRAM_QUIT 0
#define PROGRAM_OP   1
#define PROGRAM_OP2  2
#define PROGRAM_SQ   3


int process_stdin  = -1;
int process_stdout = -1;
int current_pid    = -1;
int current_program = PROGRAM_QUIT;

void*           main_recv_thread_run_loop(void* SELF);
Network*        net;
oscValue_t*     osc_values_buffer;
char*           osc_recv_buffer;
pthread_t       osc_recv_thread;
//pthread_mutex_t osc_mutex;

/*--------------------------------------------------------------------*/
void q()
{
  if(current_pid < 0) return;
  
  FILE* in = fdopen(process_stdin, "w");
  if(in != NULL)
  {
    fprintf(in, "q");
    fflush(in);

    int i;
    fprintf(stderr, "q-ing PID %i ... ", current_pid);

    for(i=0; i<1000; i++)
      {
        if(current_pid < 0)
          break;
        usleep(1000);
      }

    if(i == 1000)
      {
        fprintf(stderr, "killing PID %i ... ", current_pid);
        kill(current_pid, SIGKILL);
        for(i=0; i<1000; i++)
          {
            if(current_pid < 0)
              break;
            usleep(1000);
          }
      }
  }
}

/*--------------------------------------------------------------------*/
int main_init_osc_communication(void* SELF)
{
  //osc_send_buffer = calloc(1, OSC_BUFFER_SIZE);
  //if(!osc_send_buffer) return 0;
  osc_recv_buffer = calloc(1, OSC_BUFFER_SIZE);
  if(!osc_recv_buffer) return 0;
  osc_values_buffer = calloc(sizeof(*osc_values_buffer), OSC_VALUES_BUFFER_SIZE);
  if(!osc_values_buffer) return 0;

  net  = net_new();
  if(!net) return 0;
  if(!net_udp_connect(net, OSC_RECV_PORT))
    return 0;
    
  int error = pthread_create(&osc_recv_thread, NULL, main_recv_thread_run_loop, SELF);
  if(error != 0)
    return 0;

  return 1;
}

/*--------------------------------------------------------------------*/
void*  main_recv_thread_run_loop(void* SELF /*NULL*/)
{
  char senders_address[16];
  char *osc_address, *osc_type_tag;
  
  for(;;)
  {
    int num_valid_bytes = net_udp_receive(net, osc_recv_buffer, OSC_BUFFER_SIZE, senders_address);
    if(num_valid_bytes < 0)
      continue; //return NULL ?
    int num_osc_values = oscParse(osc_recv_buffer, num_valid_bytes, &osc_address, &osc_type_tag, osc_values_buffer, OSC_VALUES_BUFFER_SIZE);
  
    uint32_t address_hash = oscHash((unsigned char*)osc_address);
    if(address_hash == 193346984) // '/sq'
      {
        if(current_program != PROGRAM_SQ)
          {
            q();
            if(current_pid > 0) continue;
            current_program = PROGRAM_SQ;
            current_pid = system2("sq", &process_stdin, &process_stdout);
            fprintf(stderr, "sq ... ");
          }
        }
    else if(address_hash == 193346357) // '/op'
      {
        if(current_program != PROGRAM_OP)
          {
            q();
            if(current_pid > 0) continue;
            current_program = PROGRAM_OP;
            current_pid = system2("op", &process_stdin, &process_stdout);
            fprintf(stderr, "started op new PID %i ... \r\n", current_pid);
          }
      }
    else if(address_hash == 2085462503) // '/op2'
      {
        if(current_program != PROGRAM_OP2)
          {
            q();
            if(current_pid > 0) continue;
            current_program = PROGRAM_OP2;
            current_pid = system2("op2", &process_stdin, &process_stdout);
            fprintf(stderr, "op2 ... ");
          }
      }
    else if(address_hash == 101684563) // '/quit'
      {
        if(current_program != PROGRAM_QUIT)
          {
            q();
            if(current_pid > 0) continue;
            //current_pid = -1;
            current_program = PROGRAM_QUIT;
            fprintf(stderr, "/quit\r\n");
          }
      }
  }
}

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
int system2(const char * command, int * infp, int * outfp)
{
    int p_stdin[2];
    int p_stdout[2];
    int pid;

    if (pipe(p_stdin) == -1)
        return -1;

    if (pipe(p_stdout) == -1) {
        close(p_stdin[0]);
        close(p_stdin[1]);
        return -1;
    }

    pid = fork();

    if (pid < 0) {
        close(p_stdin[0]);
        close(p_stdin[1]);
        close(p_stdout[0]);
        close(p_stdout[1]);
        return pid;
    } else if (pid == 0) {
        close(p_stdin[1]);
        dup2(p_stdin[0], 0);
        close(p_stdout[0]);
        dup2(p_stdout[1], 1);
        //dup2(open("/dev/null", O_RDONLY), 2);
        // Close all other descriptors for the safety sake.
        for (int i = 3; i < 4096; ++i)
            close(i);

        setsid();
        execl("/bin/sh", "sh", "-c", command, NULL);
        _exit(1);
    }

    close(p_stdin[0]);
    close(p_stdout[1]);

    if (infp == NULL) {
        close(p_stdin[1]);
    } else {
        *infp = p_stdin[1];
    }

    if (outfp == NULL) {
        close(p_stdout[0]);
    } else {
        *outfp = p_stdout[0];
    }

    return pid;
}


/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
void sig_handler(int signo)
{
  if (signo == SIGCHLD)
    {
      process_stdin  = -1;
      process_stdout = -1;
      current_pid    = -1;
      current_program = PROGRAM_QUIT;
      fprintf(stderr, "terminated\r\n");
    }

  //happens when calling fflush on process that has terminated
  if (signo == SIGPIPE)
    fprintf(stderr, "received SIGPIPE\r\n");
}

/*--------------------------------------------------------------------*/
void sigaction_handler(int signo, siginfo_t *info, void *_ucontext)
{
  //ucontext_t* ucontext = *_ucontext;
  if (signo == SIGCHLD)
    {
      process_stdin  = -1;
      process_stdout = -1;
      current_pid    = -1;
      current_program = PROGRAM_QUIT;
      fprintf(stderr, "errno: %i\tcode: %i\tstatus:%i\r\n", info->si_errno, info->si_code, info->si_status);
    }
}

/*--------------------------------------------------------------------*/
void init_signal_handlers()
{
  //if(signal(SIGCHLD, sig_handler) == SIG_ERR)
    // fprintf(stderr, "cannot handle SIGCHLD\r\n");
  if(signal(SIGPIPE, sig_handler) == SIG_ERR)
     fprintf(stderr, "cannot handle SIGPIPE\r\n");
  
  struct sigaction new_action;
  new_action.sa_sigaction = sigaction_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = SA_SIGINFO | SA_NOCLDSTOP | SA_NOCLDSTOP;
  
  sigaction (SIGCHLD, &new_action, NULL);
}
/*--------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
  init_signal_handlers();
  if(!main_init_osc_communication(NULL))
    {perror("unable to init OSC communication"); return -1;}
  
  fprintf(stderr, "Send OSC messages '/sq', '/op' or '/op2' on port %i to run any of those programs. Send '/quit' to quit them. Press q to quit this program\r\n", OSC_RECV_PORT);
  
  i_hate_canonical_input_processing();
  
  for(;;)
    if(getchar() == 'q')
      break;
  

  q();
  make_stdin_cannonical_again();
  
  return 0;
}

/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
struct termios old_terminal_attributes;

void i_hate_canonical_input_processing(void)
{
  int error;
  struct termios new_terminal_attributes;
  
  int fd = fcntl(STDIN_FILENO,  F_DUPFD, 0);
  
  error = tcgetattr(fd, &(old_terminal_attributes));
  if(error == -1) {  fprintf(stderr, "Error getting serial terminal attributes\r\n"); return;}
  
  new_terminal_attributes = old_terminal_attributes;
  
  cfmakeraw(&new_terminal_attributes);
  
  error = tcsetattr(fd, TCSANOW, &new_terminal_attributes);
  if(error == -1) {  fprintf(stderr,  "Error setting serial attributes\r\n"); return; }
}

/*--------------------------------------------------------------------*/
void make_stdin_cannonical_again()
{
  int fd = fcntl(STDIN_FILENO,  F_DUPFD, 0);
  
  if (tcsetattr(fd, TCSANOW, &old_terminal_attributes) == -1)
    fprintf(stderr,  "Error setting serial attributes\r\n");
}
