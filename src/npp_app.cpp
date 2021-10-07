/* --------------------------------------------------------------------------
   Node++ Web App
   Jurek Muszynski
   nodepp.org
-----------------------------------------------------------------------------
   Hello World Sample Node++ Web Application
-------------------------------------------------------------------------- */


#include "npp.h"


/* --------------------------------------------------------------------------
   Output HTML & page header
-------------------------------------------------------------------------- */
static void header(int ci)
{
    OUT("<!DOCTYPE html>");
    OUT("<html>");
    OUT("<head>");
    OUT("<meta charset=\"UTF-8\">");
    OUT("<title>%s</title>", NPP_APP_NAME);
    if ( REQ_MOB )
        OUT("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    OUT("</head>");

    OUT("<body>");

    if ( REQ_DSK )  /* for desktop only */
    {
        OUT("<style>");
        OUT("body{margin-left:25px;}");
        OUT("</style>");
    }

    /* -------------------------------------------------------------------------- */
    /* first row -- logo */

    OUT("<div>");

    if ( REQ("") )
        OUT("<h1>%s</h1>", NPP_APP_NAME);
    else
        OUT("<h1><a href=\"/\">%s</a></h1>", NPP_APP_NAME);

    OUT("</div>");  /* end of first row */

    /* -------------------------------------------------------------------------- */
    /* second row -- main menu */

    OUT("<div style=\"margin-bottom:2.5em;\">");

    char lnk_home[256]="<a href=\"/\">Home</a>";
    char lnk_welcome[256]="<a href=\"/welcome\">Welcome</a>";
    char lnk_performance[256]="<a href=\"/performance\">Performance</a>";

    if ( REQ("") )
        strcpy(lnk_home, "<b>Home</b>");
    else if ( REQ("welcome") )
        strcpy(lnk_welcome, "<b>Welcome</b>");
    else if ( REQ("performance") )
        strcpy(lnk_performance, "<b>Performance</b>");

    /* display menu */

    OUT(lnk_home);
    OUT(" | ");
    OUT(lnk_welcome);
    OUT(" | ");
    OUT(lnk_performance);

    OUT("</div>");  /* end of second row */
}


/* --------------------------------------------------------------------------
   Output footer; body & html tags close here
-------------------------------------------------------------------------- */
static void footer(int ci)
{
    OUT("</body></html>");
}


/* --------------------------------------------------------------------------
   Render landing page
-------------------------------------------------------------------------- */
void render_landing(int ci)
{
    header(ci);

    OUT("<h2>Welcome to my web app!</h2>");

    /* show client type */

    if ( REQ_DSK )
        OUT("<p>You're on desktop.</p>");
    else if ( REQ_TAB )
        OUT("<p>You're on tablet.</p>");
    else  /* REQ_MOB */
        OUT("<p>You're on the phone.</p>");

    /* show some info */

    OUT("<p>You can see how a <a href=\"/welcome\">simple form</a> works.</p>");

    OUT("<p>You can also estimate <a href=\"/performance\">your server's performance</a>.</p>");

    OUT("<p>You can modify this app in <b style=\"font-family:monospace;\">src/npp_app.cpp</b>.</p>");

    footer(ci);
}


/* --------------------------------------------------------------------------
   Render page
-------------------------------------------------------------------------- */
void render_welcome(int ci)
{
    header(ci);

    /* display form */

    OUT("<p>Say something about yourself:</p>");

    OUT("<form action=\"/welcome\" style=\"width:250px;text-align:right;margin-bottom:2em;\">");
    OUT("<p>Name: <input name=\"name\" autofocus></p>");
    OUT("<p>Age: <input name=\"age\" type=\"number\"></p>");
    OUT("<p><input type=\"submit\" value=\"Submit\"></p>");
    OUT("</form>");

    /* try to retrieve query string values */

    QSVAL name;   /* string value */

    if ( QS("name", name) )    /* if present, bid welcome */
        OUT("<p>Welcome <b>%s</b>, my dear friend!</p>", name);  /* QS sanitizes strings by default */

    int age;    /* integer */

    if ( QSI("age", &age) )   /* if present, say something nice */
        OUT("<p>It's good to see you in a good health, despite being already %d years old!</p>", age);

    footer(ci);
}


/* --------------------------------------------------------------------------
   Render page
-------------------------------------------------------------------------- */
void render_performance(int ci)
{
    header(ci);

static int    requests;
static double elapsed_all;
static double average;

    ++requests;

    double elapsed = npp_elapsed(&G_connections[ci].proc_start);

    elapsed_all += elapsed;

    average = elapsed_all / requests;

    long long requests_daily = 86400000 / average;

    OUT("<p>This request took %0.3lf ms to process.</p>", elapsed);
    OUT("<p>Based on %d request(s), average rendering time is %0.3lf ms.</p>", requests, average);

    char formatted[256];
    npp_lib_fmt_int(ci, formatted, requests_daily);  /* format number depending on user agent language */

    OUT("<p>So this server could handle up to <b>%s</b> requests per day.</p>", formatted);

    OUT("<p>Refresh this page a couple of times to obtain more accurate estimation.</p>");

    footer(ci);
}




/* --------------------------------------------------------------------------------
   Called after parsing HTTP request header
   ------------------------------
   This is the main entry point for a request
   ------------------------------
   Response status will be 200 by default
   Use RES_STATUS() if you want to change it
-------------------------------------------------------------------------------- */
void npp_app_main(int ci)
{
    if ( REQ("") )  // landing page
    {
        render_landing(ci);
    }
    else if ( REQ("welcome") )
    {
        render_welcome(ci);
    }
    else if ( REQ("performance") )
    {
        render_performance(ci);
    }
    else  // page not found
    {
        RES_STATUS(404);
    }
}


/* --------------------------------------------------------------------------------
   Called when application starts
   ------------------------------
   Return true if everything OK
   ------------------------------
   Returning false will stop booting process,
   npp_app_done() will be called and application will be terminated
-------------------------------------------------------------------------------- */
bool npp_app_init(int argc, char *argv[])
{
    return true;
}


/* --------------------------------------------------------------------------------
   Called when new anonymous session is starting
   ------------------------------
   Return true if everything OK
   ------------------------------
   Returning false will cause the session to be closed
   and npp_app_session_done() will be called
   Response status will be set to 500
-------------------------------------------------------------------------------- */
bool npp_app_session_init(int ci)
{
    return true;
}


#ifdef NPP_USERS
/* --------------------------------------------------------------------------------
   ******* Only for USERS *******
   ------------------------------
   Called after successful authentication (using password or cookie)
   when user session is upgraded from anonymous to logged in
   ------------------------------
   Return true if everything OK
   ------------------------------
   Returning false will cause the session to be downgraded back to anonymous
   and npp_app_user_logout() will be called
-------------------------------------------------------------------------------- */
bool npp_app_user_login(int ci)
{
    return true;
}


/* --------------------------------------------------------------------------------
   ******* Only for USERS *******
   ------------------------------
   Called when downgrading logged in user session to anonymous
   Application session data (SESSION_DATA) will be zero-ed as well,
   unless NPP_KEEP_SESSION_DATA_ON_LOGOUT is defined
-------------------------------------------------------------------------------- */
void npp_app_user_logout(int ci)
{
}
#endif  /* NPP_USERS */


/* --------------------------------------------------------------------------------
   Called when closing anonymous session
   After calling this the session memory will be zero-ed
-------------------------------------------------------------------------------- */
void npp_app_session_done(int ci)
{
}


/* --------------------------------------------------------------------------------
   Called when application shuts down
-------------------------------------------------------------------------------- */
void npp_app_done()
{
}
