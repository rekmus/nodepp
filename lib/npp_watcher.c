/* --------------------------------------------------------------------------

    MIT License

    Copyright (c) 2020-2022 Jurek Muszynski (rekmus)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

-----------------------------------------------------------------------------

    Node++ Web App Engine
    Restart Node++ app if dead
    nodepp.org

-------------------------------------------------------------------------- */


#include "npp.h"


#define STOP_COMMAND        "$NPP_DIR/bin/nppstop"
#define START_COMMAND       "$NPP_DIR/bin/nppstart"


int         G_httpPort=80;


static char M_watcherStopCmd[256];
static char M_watcherStartCmd[256];
static int  M_watcherWait;
static int  M_watcherLogRestart;


/* --------------------------------------------------------------------------
   Restart
-------------------------------------------------------------------------- */
static void restart()
{
    ALWAYS_T("Restarting...");

    INF_T("Stopping...");
    INF_T(M_watcherStopCmd);

    if ( system(M_watcherStopCmd) != EXIT_SUCCESS )
        WAR("Couldn't execute %s", M_watcherStopCmd);

    npp_update_time_globals();

    INF_T("Waiting %d second(s)...", M_watcherWait);
#ifdef _WIN32
    Sleep(M_watcherWait*1000);
#else   /* Linux */
    sleep(M_watcherWait);
#endif

    npp_update_time_globals();

    INF_T("Starting...");
    INF_T(M_watcherStartCmd);

    if ( system(M_watcherStartCmd) != EXIT_SUCCESS )
        WAR("Couldn't execute %s", M_watcherStartCmd);

#ifdef NPP_ADMIN_EMAIL
    if ( strlen(NPP_ADMIN_EMAIL) )
    {
        char message[1024];
        strcpy(message, "Node++ Watcher had to restart web server.");
        npp_email(NPP_ADMIN_EMAIL, "Node++ restart", message);
    }
#endif
}


/* --------------------------------------------------------------------------
   main
-------------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    /* library init ------------------------------------------------------ */

    if ( !npp_lib_init(FALSE, NULL) )
        return EXIT_FAILURE;

    /* ------------------------------------------------------------------- */

    if ( !npp_read_param_int("watcherLogLevel", &G_logLevel) )
        G_logLevel = 0;   /* don't create log file */

    if ( !npp_read_param_int("watcherLogToStdout", &G_logToStdout) )
        G_logToStdout = 0;

    if ( !npp_read_param_str("watcherStopCmd", M_watcherStopCmd) )
        strcpy(M_watcherStopCmd, STOP_COMMAND);

    if ( !npp_read_param_str("watcherStartCmd", M_watcherStartCmd) )
        strcpy(M_watcherStartCmd, START_COMMAND);

    if ( !npp_read_param_int("watcherWait", &M_watcherWait) )
        M_watcherWait = 10;

    if ( !npp_read_param_int("watcherLogRestart", &M_watcherLogRestart) )
        M_watcherLogRestart = 3;

    /* start log --------------------------------------------------------- */

    if ( G_logLevel && !npp_log_start("watcher", FALSE, FALSE) )
        return EXIT_FAILURE;

    /* ------------------------------------------------------------------- */

    INF_T("Trying to connect...");

    G_callHTTPTimeout = 60000;   /* 60 seconds */

    char url[1024];

    sprintf(url, "127.0.0.1:%d", G_httpPort);

    CALL_HTTP_HEADER_SET("User-Agent", "Node++ Watcher Bot");

    if ( !CALL_HTTP(NULL, NULL, "GET", url, FALSE) )
    {
        npp_update_time_globals();

        if ( M_watcherLogRestart > G_logLevel )
        {
            int old_level = G_logLevel;
            G_logLevel = M_watcherLogRestart;
            if ( old_level < 1 )
                npp_log_start("watcher", FALSE, FALSE);
        }

        ERR_T("Couldn't connect");

        restart();
    }

    /* ------------------------------------------------------------------- */

    npp_update_time_globals();

    INF_T("npp_watcher ended");

    npp_lib_done();

    return EXIT_SUCCESS;
}
