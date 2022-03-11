/* --------------------------------------------------------------------------
   Node++ Web App
   Jurek Muszynski
   nodepp.org
-----------------------------------------------------------------------------
   Hello World Sample Node++ Web Application
-------------------------------------------------------------------------- */

#ifndef NPP_APP_H
#define NPP_APP_H


#define NPP_APP_NAME                    "Sample Node++ App"
#define NPP_APP_DOMAIN                  "example.com"
#define NPP_APP_DESCRIPTION             "Sample Node++ Web Application"


/* List of additional C/C++ modules to compile. They have to be one-liners */

#define NPP_APP_MODULES                 ""
#define NPP_SVC_MODULES                 NPP_APP_MODULES


#define NPP_DEBUG


/* app session data */
/* accessible via SESSION_DATA macro */

typedef struct {
    char dummy;     // replace with your own struct members
} app_session_data_t;


#endif  /* NPP_APP_H */
