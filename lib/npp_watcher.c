/* --------------------------------------------------------------------------

    MIT License

    Copyright (c) 2020 Jurek Muszynski

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


int         G_httpPort;


static char M_watcherStopCmd[256];
static char M_watcherStartCmd[256];
static int  M_watcherWait;
static int  M_watcherLogRestart;


/* --------------------------------------------------------------------------
   Restart
-------------------------------------------------------------------------- */
static void restart()
{
    if ( M_watcherLogRestart > 0 )
    {
        G_logLevel = M_watcherLogRestart;
        log_start("watcher", FALSE);
    }

    ALWAYS_T("Restarting...");

    INF_T("Stopping...");
    INF_T(M_watcherStopCmd);

    if ( system(M_watcherStopCmd) != EXIT_SUCCESS )
        WAR("Couldn't execute %s", M_watcherStopCmd);

    npp_update_time_globals();

    INF_T("Waiting %d second(s)...", M_watcherWait);
    sleep(M_watcherWait);

    npp_update_time_globals();

    INF_T("Starting...");
    INF_T(M_watcherStartCmd);

    if ( system(M_watcherStartCmd) != EXIT_SUCCESS )
        WAR("Couldn't execute %s", M_watcherStartCmd);

#ifdef APP_ADMIN_EMAIL
    if ( strlen(APP_ADMIN_EMAIL) )
    {
        char message[1024];
        strcpy(message, "Node++ Watcher had to restart web server.");
        npp_email(APP_ADMIN_EMAIL, "Node++ restart", message);
    }
#endif
}


/* --------------------------------------------------------------------------
   main
-------------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    char config[256];

    /* library init ------------------------------------------------------ */

    if ( !npp_lib_init() )
		return EXIT_FAILURE;

    npp_sort_messages();

    G_initialized = 1;

    /* read the config file or set defaults ------------------------------ */

    if ( G_appdir[0] )
    {
        sprintf(config, "%s/bin/npp.conf", G_appdir);
        if ( !npp_read_conf(config) )   /* no config file there */
        {
            strcpy(config, "npp.conf");
            npp_read_conf(config);
        }
    }
    else    /* no NPP_DIR -- try current dir */
    {
        strcpy(config, "npp.conf");
        npp_read_conf(config);
    }

    /* ------------------------------------------------------------------- */

    if ( !npp_read_param_int("watcherLogLevel", &G_logLevel) )
        G_logLevel = 0;   /* don't create log file */

    if ( !npp_read_param_int("watcherLogToStdout", &G_logToStdout) )
        G_logToStdout = 0;

    if ( !npp_read_param_int("httpPort", &G_httpPort) )
        G_httpPort = 80;

    if ( !npp_read_param_str("watcherStopCmd", M_watcherStopCmd) )
        strcpy(M_watcherStopCmd, STOP_COMMAND);

    if ( !npp_read_param_str("watcherStartCmd", M_watcherStartCmd) )
        strcpy(M_watcherStartCmd, START_COMMAND);

    if ( !npp_read_param_int("watcherWait", &M_watcherWait) )
        M_watcherWait = 10;

    if ( !npp_read_param_int("watcherLogRestart", &M_watcherLogRestart) )
        M_watcherLogRestart = 3;

    /* start log --------------------------------------------------------- */

    if ( !log_start("watcher", FALSE) )
		return EXIT_FAILURE;

    /* ------------------------------------------------------------------- */

    INF_T("Trying to connect...");

    G_RESTTimeout = 60000;   /* 60 seconds */

    char url[1024];

    sprintf(url, "127.0.0.1:%d", G_httpPort);

    REST_HEADER_SET("User-Agent", "Node++ Watcher Bot");

    if ( !CALL_REST_HTTP(NULL, NULL, "GET", url, 0) )
    {
        npp_update_time_globals();
        ERR_T("Couldn't connect");
        restart();
    }

    /* ------------------------------------------------------------------- */

    npp_update_time_globals();

    INF_T("npp_watcher ended");

    log_finish();

    return EXIT_SUCCESS;
}
