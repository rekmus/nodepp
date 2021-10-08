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

    /* -------------------------------------------------------------------------- */
    /* style */

    OUT("<style>");

    if ( REQ_DSK )
        OUT("body{margin-left:25px;margin-bottom:40px;}");   /* for desktop only */

    OUT("code{font-size:1.1em;}");
    OUT(".m{font-family:monospace;font-size:1.1em;}");
    OUT(".mt{margin-top:40px;}");

    OUT("</style>");

    /* -------------------------------------------------------------------------- */
    /* first row -- logo */

    OUT("<div>");

static char logo[]="<img src=\"/nodepp.jpg\" alt=\"Logo\" width=\"250\" height=\"62\">";

    if ( REQ("") )
        OUT("<h1>%s</h1>", logo);
    else
        OUT("<h1><a href=\"/\">%s</a></h1>", logo);

    OUT("</div>");  /* end of first row */

    /* -------------------------------------------------------------------------- */
    /* second row -- main menu */

    OUT("<div style=\"margin-bottom:2.5em;\">");

    char lnk_home[256]="<a href=\"/\">Home</a>";
    char lnk_welcome[256]="<a href=\"/welcome\">Welcome</a>";
    char lnk_snippets[256]="<a href=\"/snippets\">Snippets</a>";
    char lnk_performance[256]="<a href=\"/performance\">Performance</a>";

    if ( REQ("") )
        strcpy(lnk_home, "<b>Home</b>");
    else if ( REQ("welcome") )
        strcpy(lnk_welcome, "<b>Welcome</b>");
    else if ( REQ("snippets") )
        strcpy(lnk_snippets, "<b>Snippets</b>");
    else if ( REQ("performance") )
        strcpy(lnk_performance, "<b>Performance</b>");

    /* display menu */

    OUT(lnk_home);
    OUT(" | ");
    OUT(lnk_welcome);
    OUT(" | ");
    OUT(lnk_snippets);
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

    OUT("<h2>Congratulations!</h2>");

    OUT("<p>You've managed to compile and run Node++ web application! I hope you will like the performance of the compiled code.</p>");

    /* show client type */

    if ( REQ_DSK )
        OUT("<p>You're on desktop, right?</p>");
    else if ( REQ_TAB )
        OUT("<p>You're on tablet, right?</p>");
    else  /* REQ_MOB */
        OUT("<p>You're on the phone, right?</p>");

    /* show some info */

    OUT("<p>You can see how a <a href=\"/welcome\">simple form</a> works.</p>");

    OUT("<p>You can use <a href=\"/snippets\">HTML and Markdown snippets</a> for static content.</p>");

    OUT("<p>You can also estimate <a href=\"/performance\">your server's performance</a>.</p>");

    OUT("<p>You can modify this app in <b class=m>src/npp_app.cpp</b>. The entry point is <code>npp_app_main()</code>. Have fun!</p>");

    /* --------------------------------------------------- */

    OUT("<p class=mt><i>To see the source code look at <code>render_landing()</code>.</i></p>");

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
    int   age;    /* integer */

    if ( QS("name", name) )    /* if present, bid welcome */
        OUT("<p>Welcome <b>%s</b>, my dear friend!</p>", name);  /* QS sanitizes strings by default */

    if ( QSI("age", &age) )   /* if present, say something nice */
        OUT("<p>It's good to see you in a good health, despite being already %d years old!</p>", age);

    /* --------------------------------------------------- */

    if ( QS("name", NULL) )    /* show this only if there's a name in query string */
        OUT("<p class=mt><i>To see the source code look at <code>render_welcome()</code>.</i></p>");

    footer(ci);
}


/* --------------------------------------------------------------------------
   Render page
-------------------------------------------------------------------------- */
void render_snippets(int ci)
{
    header(ci);

    OUT_SNIPPET("sample_snippet.html");

    OUT("<p>&nbsp;</p>");

    OUT_SNIPPET_MD("sample_snippet.md");

    /* --------------------------------------------------- */

    OUT("<p class=mt><i>To see the source code look at <code>render_snippets()</code>.</i></p>");

    footer(ci);
}


/* --------------------------------------------------------------------------
   Render page
-------------------------------------------------------------------------- */
void render_performance(int ci)
{
    header(ci);

    if ( G_cnts_today.req < 2 )
    {
        OUT("<p>Refresh this page to let the server measure the average processing time.</p>");
    }
    else    /* server average is available only after the first request has finished */
    {
        long long requests_daily = 86400000 / G_cnts_today.average;

        OUT("<p>Based on %d requests the average rendering time is %0.3lf ms.</p>", G_cnts_today.req, G_cnts_today.average);

        OUT("<p>It seems that this Node++ application could handle up to <b>%s</b> requests per day on your server.</p>", INT(requests_daily));

        OUT("<p>Refresh this page a couple of times to obtain more accurate estimation.</p>");
    }

    /* --------------------------------------------------- */

    OUT("<p class=mt><i>To see the source code look at <code>render_performance()</code>.</i></p>");

    RES_DONT_CACHE;

    footer(ci);
}




/* --------------------------------------------------------------------------------
   This is the main entry point for a request
   ------------------------------
   Called after parsing HTTP request headers
   ------------------------------
   If required (NPP_REQUIRED_AUTH_LEVEL >= AUTH_LEVEL_ANONYMOUS),
   the session is already created

   If valid ls cookie is present in the request or
   it's over existing connection that has already been authenticated,
   the session is already authenticated
   ------------------------------
   Response status is 200 by default
   Use RES_STATUS() if you want to change it

   Response content type is text/html by default
   Use RES_CONTENT_TYPE() if you want to change it
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
    else if ( REQ("snippets") )
    {
        render_snippets(ci);
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
