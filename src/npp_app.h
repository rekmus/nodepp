/* --------------------------------------------------------------------------
   Node++ Web App
   Jurek Muszynski
   nodeplusplus.org
-----------------------------------------------------------------------------
   Hello World Sample Node++ Web Application
-------------------------------------------------------------------------- */

#ifndef NPP_APP_H
#define NPP_APP_H


#define APP_WEBSITE                     "Node++ Hello World"
#define APP_DOMAIN                      "example.com"
#define APP_DESCRIPTION                 "Hello World Sample Node++ Web Application"
#define APP_VERSION                     "1.0"


#define HTTP2
#define DUMP


/* app user session */

typedef struct {
    char dummy;     // replace with your own struct members
} ausession_t;


#endif  /* NPP_APP_H */
