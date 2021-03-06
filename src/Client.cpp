#include "Client.h"
#include "SDLman.h"
#include "Logman.h"
#include "Config.h"
#include "Graphics.h"
#include "Ships.h"
#include "Chat.h"
#include "Timers.h"
#include "Net.h"
#include "Packets.h"
#include "Connection.h"
#include "Players.h"
#include "Map.h"

#ifdef WIN32
void PrintStackTrace()
{
    fprintf(stderr, "Stack trace not available on Windows.\n");
}

#else

#include <signal.h>
#include <execinfo.h>  // for backtraces
#include <cxxabi.h>    // for c++ name demangling
#include <sys/types.h>
#include <unistd.h>

// Print a demangled stack backtrace of the caller function
// from: http://panthema.net/2008/0901-stacktrace-demangled
// this requires being compiled with -g -rdynamic to work
void PrintStackTrace()
{
    FILE *out = stderr;
    unsigned int max_frames = 127;

    fprintf(out, "\nStack trace (requires compilation with '-g -rdynamic'):\n");

    // storage array for stack trace address data
    void *addrlist[max_frames + 1];

    // retrieve current stack addresses
    int addrlen = backtrace(addrlist, sizeof(addrlist) / sizeof(void *));

    if (addrlen == 0)
    {
        fprintf(out, "  <empty, possibly corrupt>\n");
        return;
    }

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char **symbollist = backtrace_symbols(addrlist, addrlen);

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    char *funcname = (char *)malloc(funcnamesize);

    // iterate over the returned symbol lines.
    for (int i = 0; i < addrlen; i++)
    {
        char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

        // find parentheses and +address offset surrounding the mangled name:
        // ./module(function+0x15c) [0x8048a6d]
        for (char *p = symbollist[i]; *p; ++p)
        {
            if (*p == '(')
                begin_name = p;
            else if (*p == '+')
                begin_offset = p;
            else if (*p == ')' && begin_offset)
            {
                end_offset = p;
                break;
            }
        }

        // stop printing once we reach start_main
        if (strcasestr(symbollist[i], "__libc_start_main") != 0)
            break;

        if (begin_name && begin_offset && end_offset && begin_name < begin_offset)
        {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            // mangled name is now in [begin_name, begin_offset) and caller
            // offset in [begin_offset, end_offset). now apply
            // __cxa_demangle():

            int status;
            char *ret = abi::__cxa_demangle(begin_name, funcname, &funcnamesize, &status);
            if (status == 0)
            {
                funcname = ret;  // use possibly realloc()-ed string
                fprintf(out, "%s+%s in ", funcname, begin_offset);
            }
            else
            {
                // demangling failed.
                fprintf(out, "%s()+%s in ", begin_name, begin_offset);
            }

            fflush(out);

            ////////////////////////
            char **messages = symbollist;
            void **trace = addrlist;
            /* find first occurence of '(' or ' ' in message[i] and assume
             * everything before that is the file name. (Don't go beyond 0 though
             * (string terminator)*/
            i32 p = 0;
            while (messages[i][p] != '(' && messages[i][p] != ' ' && messages[i][p] != 0)
                ++p;

            char syscom[256];
            sprintf(syscom, "addr2line %p -s -e %.*s", trace[i], p, messages[i]);
            // last parameter is the file name of the symbol
            system(syscom);
            /////////////////////////////
        }
        else
        {
            // couldn't parse the line? skip
            // fprintf(out, "  %s\n", symbollist[i]);
        }
    }

    free(funcname);
    free(symbollist);
}

static void SigHandler(int sig)
{
    fprintf(stderr, "\nSEGFAULT, trying to print stack trace.");
    PrintStackTrace();
    signal(sig, SIG_DFL);
    kill(getpid(), sig);
}

#endif  // !WIN32

class SegFaultHandler
{
   public:
    SegFaultHandler()
    {
#ifndef WIN32
        // this requires being compiled with -g -rdynamic to get function names to work
        signal(SIGSEGV, SigHandler);
#endif  // !WIN32
    }
};

Client::Client()
    :  // initialization order matters, add to the end
      segFaultHandler(make_shared<SegFaultHandler>()),
      cfg(make_shared<Config>(*this)),
      log(make_shared<Logman>(*this)),
      packets(make_shared<Packets>(*this)),
      net(make_shared<Net>(*this)),
      sdl(make_shared<SDLman>(*this)),
      graphics(make_shared<Graphics>(*this)),
      timers(make_shared<Timers>(*this)),
      chat(make_shared<Chat>(*this)),
      ships(make_shared<Ships>(*this)),
      connection(make_shared<Connection>(*this)),
      players(make_shared<Players>(*this)),
      map(make_shared<Map>(*this))
{
}

void Client::Start()
{
    sdl->MainLoop();
}
