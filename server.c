#include "server_helper.h"

void usage()
{
    printf("server.exe -i or\n server.exe <portnumber>\n");
}


// public static main(string[] args)
int main(int argc, char** argv)
{
    CmdArg  cmd;
    if(!process_args(argc, argv, &cmd)){
        usage();
        return -1;   
    };
    if (cmd.is_interactive) //if -i was provided
    {
        run_interactive(&cmd); //we have the address of the cmd here so it doesnt make a copy
    }
    else
    {
        run_listen(&cmd);
    }
    return 0;
}















//TODO1 true/false ()  -DONE
//TODO2 write usage() function
//TODO3 call usage() if process_args failes - DONE
//TODO4 write process_cmd function   -DONE
//TODO5 finsihs the run_interactive functions inside    R Done,
//TODO5.1 implement the isValidRule function to help run_add_cmd (A)
//TODO5.1.1 make the isValidPort and isValidIP function to help isValidRule 


