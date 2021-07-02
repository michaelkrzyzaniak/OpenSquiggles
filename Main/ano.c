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
//gcc ano.c ../Robot_Communication_Framework/*.c -framework CoreMidi -framework Carbon -O2 -o ano

//Linux compile with:
//sudo apt-get install libasound2-dev
//gcc ano.c ../Robot_Communication_Framework/*.c -lasound -o ano

#define __OPC_VERSION__ "0.3"


/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
/*--------------------------------------------------------------------*/
#include <stdio.h>

#include "../Robot_Communication_Framework/Robot_Communication.h"

Robot* robot;

/*--------------------------------------------------------------------*/
void message_recd_from_robot(void* ignored_self, char* message, robot_arg_t args[], int num_args)
{
}
        
/*--------------------------------------------------------------------*/
int main(void)
{
  robot = robot_new(message_recd_from_robot, NULL);
  robot_send_message(robot, robot_cmd_all_notes_off);
  robot_send_message(robot, robot_cmd_all_notes_off);
  robot_send_message(robot, robot_cmd_all_notes_off);
  robot_send_message(robot, robot_cmd_eye_yes, 3, 60);
  robot = robot_destroy(robot);

  return 0;
}

