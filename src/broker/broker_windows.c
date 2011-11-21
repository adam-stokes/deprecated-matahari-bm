#include <stdio.h>
#include <wchar.h>

#include <windows.h>

#include "broker.h"
#include "broker_os.h"
#include "broker_federation.h"


#define CMD_LINE_LENGTH  32768

#define QPID_BROKER      L"qpidd"
#define QPID_ROUTE       L"qpid-route"


static int start_process(const wchar_t *proc, wchar_t *cmdline)
{
    STARTUPINFO startupinfo;
    PROCESS_INFORMATION procinfo;
    BOOL result;

    GetStartupInfo(&startupinfo);

    result = CreateProcess(proc, cmdline,
                           NULL, NULL, FALSE, 0, NULL, NULL,
                           &startupinfo, &procinfo);

    CloseHandle(procinfo.hProcess);
    CloseHandle(procinfo.hThread);

    return result == FALSE;
}

int broker_os_start_broker(char * const args[])
{
    int ret;
    wchar_t cmdline[CMD_LINE_LENGTH];
    int i = 0;
    const char *arg;

    for (arg = *args; arg; arg++) {
        int c = snwprintf(cmdline + i, CMD_LINE_LENGTH - i, L"%s ", arg);
        if (c < 0 || (i += c) >= (CMD_LINE_LENGTH - 1)) {
            return -1;
        }
    }

    ret = start_process(QPID_BROKER, cmdline);

    broker_federation_configure();

    return ret ? -1 : 0;
}

int broker_os_add_qpid_route_link(const char *local, const char *remote)
{
    wchar_t cmd[1024];

    snwprintf(cmd, sizeof(cmd) / sizeof(*cmd), 
              L"%ls link add %s untrusted/untrusted@%s PLAIN", QPID_ROUTE,
              local, remote);

    if (start_process(QPID_ROUTE, cmd)) {
        return -1;
    }

    return 0;
}



int broker_os_add_qpid_route(const struct mh_qpid_route *route)
{
    wchar_t cmd[1024];

    if (route->aggregate && route->srclocal) {
        snwprintf(cmd, sizeof(cmd) / sizeof(*cmd), 
                  L"%ls --src-local route add %s %s %s %s", QPID_ROUTE,
                  route->dest, route->src, route->exchange, route->route_key);
    } else if (route->aggregate) {
        snwprintf(cmd, sizeof(cmd) / sizeof(*cmd),
                  L"%ls route add %s %s %s %s", QPID_ROUTE,
                  route->dest, route->src, route->exchange, route->route_key);
    } else {
        snwprintf(cmd, sizeof(cmd) / sizeof(*cmd),
                  L"%ls --timeout=5 dynamic add %s %s %s", QPID_ROUTE,
                  route->dest, route->src, route->exchange);
    }
 
    if (start_process(QPID_ROUTE, cmd)) {
        return -1;
    }

    return 0;
}

