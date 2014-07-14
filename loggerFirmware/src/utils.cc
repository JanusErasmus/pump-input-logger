#include <cyg/kernel/kapi.h>

#include <string.h>
#include <ctype.h>

union u_conv
{
    cyg_uint8 b[4];
    cyg_uint8 s[2];
    cyg_uint32 l;
} __attribute__((__packed__));

void util_parse_params(char *str,char *argv[],int &argc,char delim1,char delim2)
{

    int max_args = argc;
    char * cmdl = str;
    cyg_bool done = false;
    argc = 0;
    char delim = delim1;
    while ( !done )
    {
        /* Strip Leading Whitespce */
        while ( isspace(*cmdl) )
        {
            if ( *cmdl )
            {
                cmdl++;
            }
            else
            {
                done = true;
                break;
            }
        }
        /* Now we are at an arg */
        if ( !done && *cmdl )
        {
            argv[argc] = cmdl;
            argc++;
            if (argc >= max_args)
            {
                done =true;
                break;
            }
        }
        /* Go to the next delim */
        while ( delim != *cmdl )
        {
            if ( *cmdl )
            {
                cmdl++;
            }
            else
            {
                done = true;
                break;
            }
        }
        if ( *cmdl )
        {
            *cmdl = 0;
            cmdl++;
        }
        else
        {
            done = true;
        }
        if (argc)
        {
            delim = delim2;
        }
    }
}


