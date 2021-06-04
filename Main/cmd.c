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
//gcc cmd.c extras/*.c -o cmd

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
#include <sys/types.h>
#include <unistd.h> //fork()
#include <pthread.h>
#include "extras/Network.h"
#include "extras/OSC.h"

int process_stdin = -1;
int process_stdout = -1;

void*           main_recv_thread_run_loop(void* SELF);
Network*        net;
oscValue_t*     osc_values_buffer;
char*           osc_recv_buffer;
pthread_t       osc_recv_thread;

/*--------------------------------------------------------------------*/
void q()
{
  FILE* in = fdopen(process_stdin, "w");
  if(in != NULL)
  {
    fprintf(in, "q");
    fclose(in);
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
        q();
        system2("sq", &process_stdin, &process_stdout);
        fprintf(stderr, "sq\r\n");
      }
    else if(address_hash == 193346357) // '/op'
      {
        q();
        system2("op", &process_stdin, &process_stdout);
        fprintf(stderr, "op\r\n");
      }
    else if(address_hash == 2085462503) // '/op2'
      {
        q();
        system2("op2", &process_stdin, &process_stdout);
        fprintf(stderr, "op2\r\n");
      }
    else if(address_hash == 101684563) // '/quit'
      {
        q();
        fprintf(stderr, "/quit\r\n");
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
        dup2(open("/dev/null", O_RDONLY), 2);
        /// Close all other descriptors for the safety sake.
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
int main(int argc, char* argv[])
{
  if(!main_init_osc_communication(NULL))
    {perror("unable to init OSC communication"); return -1;}
  
  fprintf(stderr, "Send OSC messages '/sq', '/op' or './op2' on port %i to run any of those programs. Send '/quit' to quit them. Press q to quit this program\r\n", OSC_RECV_PORT);
  
  i_hate_canonical_input_processing();
  
  for(;;)
    if(getchar() == 'q')
      break;
  
 out:
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
