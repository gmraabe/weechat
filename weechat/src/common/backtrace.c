/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* backtrace.c: display backtrace after a segfault */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define __USE_GNU
#include <dlfcn.h>

#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif

#include "weechat.h"
#include "backtrace.h"


/*
 * weechat_backtrace_addr2line: display function name and line with a backtrace address
 */

void
weechat_backtrace_addr2line (int number, void *address, char *symbol)
{
    int rc;
    Dl_info info;
    const void *addr;
    FILE *output;
    char cmd_line[1024];
    char line[1024], *ptr_line, *pos;
    char function_name[1024];
    int file_line;
    
    rc = dladdr (address, &info);
    if ((rc == 0) || !info.dli_fname || !info.dli_fname[0])
    {
        fprintf (stderr, "%03d  %s\n", number, symbol);
        return;
    }
    
    addr = address;
    if (info.dli_fbase >= (const void *) 0x40000000)
        addr = (const char *) addr - (unsigned int) info.dli_fbase;
    
    snprintf (cmd_line, sizeof (cmd_line),
              "addr2line --functions --demangle -e %s %p",
              info.dli_fname, addr);
    output = popen (cmd_line, "r");
    if (!output)
    {
        fprintf (stderr, "%03d  %s\n", number, symbol);
        return;
    }
    function_name[0] = '\0';
    file_line = 0;
    while (!feof (output))
    {
        ptr_line = fgets (line, sizeof (line) - 1, output);
        if (ptr_line && ptr_line[0])
        {
            pos = strchr (ptr_line, '\n');
            if (pos)
                pos[0] = '\0';
            if (strchr (ptr_line, ':'))
            {
                file_line = 1;
                fprintf (stderr, "%03d  %s%s%s%s\n",
                         number,
                         ptr_line,
                         (function_name[0]) ? " [function " : "",
                         function_name,
                         (function_name[0]) ? "]" : "");
                function_name[0] = '\0';
            }
            else
            {
                if (function_name[0])
                    fprintf (stderr, "%03d  %s", number, function_name);
                snprintf (function_name, sizeof (function_name),
                          "%s", ptr_line);
            }
        }
    }
    if (function_name[0])
        fprintf (stderr, "%03d  %s\n", number, function_name);
    pclose (output);
}

/*
 * weechat_backtrace: display backtrace (called when a SIGSEGV is received)
 */

void
weechat_backtrace ()
{
#ifdef HAVE_BACKTRACE
    void *trace[BACKTRACE_MAX];
    int trace_size, i;
    char **symbols;
    
    trace_size = backtrace (trace, BACKTRACE_MAX);
    symbols = backtrace_symbols (trace, trace_size);
    
    for (i = 0; i < trace_size; i++)
    {
        weechat_backtrace_addr2line (i + 1, trace[i], symbols[i]);
    }
#else
    fprintf (stderr,
             "  No backtrace info (no debug info available or no backtrace possible "
             "on your system).\n");
#endif
}
