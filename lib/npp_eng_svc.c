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
    ASYNC services' engine module
    nodepp.org

-------------------------------------------------------------------------- */


#include "npp.h"


/* globals */

char        G_service[NPP_SVC_NAME_LEN+1];
int         G_error_code=OK;
int         G_svc_si=0;
int         G_ASYNCId=-1;
char        G_req_queue_name[256]="";
char        G_res_queue_name[256]="";
mqd_t       G_queue_req={0};                /* request queue */
mqd_t       G_queue_res={0};                /* response queue */
int         G_async_req_data_size=ASYNC_REQ_MSG_SIZE-sizeof(async_req_hdr_t); /* how many bytes are left for data */
int         G_async_res_data_size=ASYNC_RES_MSG_SIZE-sizeof(async_res_hdr_t)-sizeof(int)*4; /* how many bytes are left for data */
int         G_usersRequireActivation=0;
async_req_t G_svc_req;
async_res_t G_svc_res;
#ifdef NPP_OUT_CHECK_REALLOC
char        *G_svc_out_data=NULL;
#endif
char        *G_svc_p_content=NULL;
npp_connection_t G_connections[NPP_MAX_CONNECTIONS+1]={0};  /* request details */
eng_session_data_t G_sessions[NPP_MAX_SESSIONS+1]={0};      /* sessions -- they start from 1 */
app_session_data_t G_app_session_data[NPP_MAX_SESSIONS+1]={0}; /* app session data, using the same index (si) */

/* counters */

npp_counters_t G_cnts_today={0};            /* today's counters */
npp_counters_t G_cnts_yesterday={0};        /* yesterday's counters */
npp_counters_t G_cnts_day_before={0};       /* day before's counters */

int         G_days_up=0;                    /* web server's days up */
int         G_connections_cnt=0;            /* number of open connections */
int         G_connections_hwm=0;            /* highest number of open connections (high water mark) */
int         G_sessions_cnt=0;               /* number of active user sessions */
int         G_sessions_hwm=0;               /* highest number of active user sessions (high water mark) */
int         G_blacklist_cnt=0;              /* G_blacklist length */
char        G_last_modified[32]="";         /* response header field with server's start time */


/* locals */

static char *M_pidfile;                     /* pid file name */
static char *M_async_shm=NULL;
#ifdef NPP_OUT_CHECK_REALLOC
static unsigned M_out_data_allocated;
#endif


/* prototypes */

static void sigdisp(int sig);
static void clean_up(void);



/* --------------------------------------------------------------------------
   main
-------------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    char config[512];

    /* library init ------------------------------------------------------ */

    if ( !npp_lib_init() )
        return EXIT_FAILURE;

    /* read the config file or set defaults ------------------------------ */

    char exec_name[256];
    npp_get_exec_name(exec_name, argv[0]);

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

    if ( !npp_read_param_int("logLevel", &G_logLevel) )
        G_logLevel = 3;   /* info */

#ifdef NPP_DEBUG
        G_logLevel = 4;   /* debug */
#endif

    if ( !npp_read_param_int("logToStdout", &G_logToStdout) )
        G_logToStdout = 0;

    if ( !npp_read_param_int("ASYNCId", &G_ASYNCId) )
        G_ASYNCId = -1;

    if ( !npp_read_param_int("callHTTPTimeout", &G_callHTTPTimeout) )
        G_callHTTPTimeout = CALL_HTTP_DEFAULT_TIMEOUT;

#ifdef NPP_MYSQL
    npp_read_param_str("dbHost", G_dbHost);
    npp_read_param_int("dbPort", &G_dbPort);
    npp_read_param_str("dbName", G_dbName);
    npp_read_param_str("dbUser", G_dbUser);
    npp_read_param_str("dbPassword", G_dbPassword);
#endif  /* NPP_MYSQL */

#ifdef NPP_USERS
    npp_read_param_int("usersRequireActivation", &G_usersRequireActivation);
#endif  /* NPP_USERS */

    /* start log --------------------------------------------------------- */

    char logprefix[64];

    sprintf(logprefix, "s_%d", G_pid);

    if ( !npp_log_start(logprefix, G_test, FALSE) )
        return EXIT_FAILURE;

    /* pid file ---------------------------------------------------------- */

    if ( !(M_pidfile=npp_lib_create_pid_file(logprefix)) )
        return EXIT_FAILURE;

    /* fill the M_random_numbers up */

    npp_lib_init_random_numbers();

    /* handle signals ---------------------------------------------------- */

    signal(SIGINT,  sigdisp);   /* Ctrl-C */
    signal(SIGTERM, sigdisp);
#ifndef _WIN32
    signal(SIGQUIT, sigdisp);   /* Ctrl-\ */
    signal(SIGTSTP, sigdisp);   /* Ctrl-Z */

    signal(SIGPIPE, SIG_IGN);   /* ignore SIGPIPE */
#endif

    /* USERS library init ------------------------------------------------ */

#ifdef NPP_USERS
    libusr_init();
#endif

    /* init dummy G_connections structure ----------------------------------------- */

    COPY(G_connections[0].host, NPP_APP_DOMAIN, NPP_MAX_HOST_LEN);
    COPY(G_connections[0].app_name, NPP_APP_NAME, NPP_APP_NAME_LEN);

    if ( !(G_connections[0].in_data = (char*)malloc(G_async_req_data_size)) )
    {
        ERR("malloc for G_connections[0].in_data failed");
        return EXIT_FAILURE;
    }

    G_connections[0].in_data_allocated = G_async_req_data_size;

#ifdef NPP_OUT_CHECK_REALLOC

    if ( !(G_svc_out_data = (char*)malloc(NPP_OUT_BUFSIZE)) )
    {
        ERR("malloc for G_svc_out_data failed");
        return EXIT_FAILURE;
    }

    M_out_data_allocated = NPP_OUT_BUFSIZE;

#endif  /* NPP_OUT_CHECK_REALLOC */

    /* load snippets ----------------------------------------------------- */

    if ( !npp_lib_read_snippets("", "snippets", TRUE, NULL) )
    {
        ERR("npp_lib_read_snippets() failed");
        return EXIT_FAILURE;
    }

#ifdef NPP_MULTI_HOST   /* side gigs */

    int i;

    for ( i=1; i<G_hosts_cnt; ++i )
    {
        if ( G_hosts[i].snippets[0] && !npp_lib_read_snippets(G_hosts[i].host, G_hosts[i].snippets, TRUE, NULL) )
        {
            ERR("reading %s's snippets failed", G_hosts[i].host);
            return EXIT_FAILURE;
        }
    }

#endif  /* NPP_MULTI_HOST */

    /* open database ----------------------------------------------------- */

#ifdef NPP_MYSQL

    DBG("Trying npp_open_db...");

    if ( !npp_open_db() )
    {
        ERR("npp_open_db failed");
        clean_up();
        return EXIT_FAILURE;
    }

    ALWAYS("Database connected");

#endif  /* NPP_MYSQL */


    /* open queues ------------------------------------------------------- */

#ifdef NPP_ASYNC_ID
    if ( G_ASYNCId > -1 )
    {
        sprintf(G_req_queue_name, "%s_%d__%d", ASYNC_REQ_QUEUE, NPP_ASYNC_ID, G_ASYNCId);
        sprintf(G_res_queue_name, "%s_%d__%d", ASYNC_RES_QUEUE, NPP_ASYNC_ID, G_ASYNCId);
    }
    else
    {
        sprintf(G_req_queue_name, "%s_%d", ASYNC_REQ_QUEUE, NPP_ASYNC_ID);
        sprintf(G_res_queue_name, "%s_%d", ASYNC_RES_QUEUE, NPP_ASYNC_ID);
    }
#else
    if ( G_ASYNCId > -1 )
    {
        sprintf(G_req_queue_name, "%s__%d", ASYNC_REQ_QUEUE, G_ASYNCId);
        sprintf(G_res_queue_name, "%s__%d", ASYNC_RES_QUEUE, G_ASYNCId);
    }
    else
    {
        strcpy(G_req_queue_name, ASYNC_REQ_QUEUE);
        strcpy(G_res_queue_name, ASYNC_RES_QUEUE);
    }
#endif  /* NPP_ASYNC_ID */

    G_queue_req = mq_open(G_req_queue_name, O_RDONLY, NULL, NULL);

    if ( G_queue_req < 0 )
    {
        ERR("mq_open for req failed, errno = %d (%s)", errno, strerror(errno));
        clean_up();
        return EXIT_FAILURE;
    }

    INF("mq_open of %s OK", G_req_queue_name);

    G_queue_res = mq_open(G_res_queue_name, O_WRONLY, NULL, NULL);

    if ( G_queue_res < 0 )
    {
        ERR("mq_open for res failed, errno = %d (%s)", errno, strerror(errno));
        clean_up();
        return EXIT_FAILURE;
    }

    INF("mq_open of %s OK", G_res_queue_name);

    /* ------------------------------------------------------------------- */

    if ( !npp_svc_init() )
    {
        ERR("npp_svc_init failed");
        clean_up();
        return EXIT_FAILURE;
    }

    /* ------------------------------------------------------------------- */

    npp_sort_messages();

    G_initialized = 1;

    int prev_day = G_ptm->tm_mday;

    INF("\nWaiting...\n");

    while (1)
    {
        G_call_http_req_cnt = 0;
        G_call_http_elapsed = 0;
        G_call_http_average = 0;

        if ( mq_receive(G_queue_req, (char*)&G_svc_req, ASYNC_REQ_MSG_SIZE, NULL) != -1 )
        {
            npp_update_time_globals();

            /* start new log file every day */

            if ( G_ptm->tm_mday != prev_day )
            {
                npp_log_finish();

                if ( !npp_log_start(logprefix, G_test, FALSE) )
                {
                    clean_up();
                    return EXIT_FAILURE;
                }

                prev_day = G_ptm->tm_mday;

                npp_lib_init_random_numbers();

                if ( !npp_lib_read_snippets("", "snippets", FALSE, NULL) )
                {
                    ERR("npp_lib_read_snippets() failed");
                    clean_up();
                    return EXIT_FAILURE;
                }
            }

            DBG_T("Message received");

            if ( G_logLevel > LOG_INF )
                DBG_T("ci=%d, service [%s], call_id=%u", G_svc_req.hdr.ci, G_svc_req.hdr.service, G_svc_req.hdr.call_id);
            else
                INF_T("%s called (call_id=%u)", G_svc_req.hdr.service, G_svc_req.hdr.call_id);

            memset(&G_svc_res, 0, ASYNC_RES_MSG_SIZE);

            G_svc_res.ai = G_svc_req.hdr.ai;
            G_svc_res.ci = G_svc_req.hdr.ci;
            strcpy(G_service, G_svc_req.hdr.service);

            /* request details */

//            G_connections[0].secure = G_svc_req.hdr.secure;
            strcpy(G_connections[0].ip, G_svc_req.hdr.ip);
            strcpy(G_connections[0].method, G_svc_req.hdr.method);
//            G_connections[0].post = G_svc_req.hdr.post;
            strcpy(G_connections[0].uri, G_svc_req.hdr.uri);
            strcpy(G_connections[0].resource, G_svc_req.hdr.resource);
#if NPP_RESOURCE_LEVELS > 1
            strcpy(G_connections[0].req1, G_svc_req.hdr.req1);
#if NPP_RESOURCE_LEVELS > 2
            strcpy(G_connections[0].req2, G_svc_req.hdr.req2);
#if NPP_RESOURCE_LEVELS > 3
            strcpy(G_connections[0].req3, G_svc_req.hdr.req3);
#if NPP_RESOURCE_LEVELS > 4
            strcpy(G_connections[0].req4, G_svc_req.hdr.req4);
#if NPP_RESOURCE_LEVELS > 5
            strcpy(G_connections[0].req5, G_svc_req.hdr.req5);
#endif  /* NPP_RESOURCE_LEVELS > 5 */
#endif  /* NPP_RESOURCE_LEVELS > 4 */
#endif  /* NPP_RESOURCE_LEVELS > 3 */
#endif  /* NPP_RESOURCE_LEVELS > 2 */
#endif  /* NPP_RESOURCE_LEVELS > 1 */
            strcpy(G_connections[0].id, G_svc_req.hdr.id);
            strcpy(G_connections[0].uagent, G_svc_req.hdr.uagent);
            G_connections[0].ua_type = G_svc_req.hdr.ua_type;
            strcpy(G_connections[0].referer, G_svc_req.hdr.referer);
            G_connections[0].clen = G_svc_req.hdr.clen;
            strcpy(G_connections[0].in_cookie, G_svc_req.hdr.in_cookie);
            strcpy(G_connections[0].host, G_svc_req.hdr.host);
#ifdef NPP_MULTI_HOST
            strcpy(G_connections[0].host_normalized, G_svc_req.hdr.host_normalized);
            G_connections[0].host_id = G_svc_req.hdr.host_id;
#endif
            strcpy(G_connections[0].app_name, G_svc_req.hdr.app_name);
            strcpy(G_connections[0].lang, G_svc_req.hdr.lang);
            G_connections[0].in_ctype = G_svc_req.hdr.in_ctype;
            strcpy(G_connections[0].boundary, G_svc_req.hdr.boundary);
            G_connections[0].status = G_svc_req.hdr.status;
            strcpy(G_connections[0].cust_headers, G_svc_req.hdr.cust_headers);
            G_connections[0].cust_headers_len = G_svc_req.hdr.cust_headers_len;
            G_connections[0].ctype = G_svc_req.hdr.ctype;
            strcpy(G_connections[0].ctypestr, G_svc_req.hdr.ctypestr);
            strcpy(G_connections[0].cdisp, G_svc_req.hdr.cdisp);
            strcpy(G_connections[0].cookie_out_a, G_svc_req.hdr.cookie_out_a);
            strcpy(G_connections[0].cookie_out_a_exp, G_svc_req.hdr.cookie_out_a_exp);
            strcpy(G_connections[0].cookie_out_l, G_svc_req.hdr.cookie_out_l);
            strcpy(G_connections[0].cookie_out_l_exp, G_svc_req.hdr.cookie_out_l_exp);
            strcpy(G_connections[0].location, G_svc_req.hdr.location);
            G_svc_si = G_svc_req.hdr.si;    /* original si */
//            G_connections[0].bot = G_svc_req.hdr.bot;
//            G_connections[0].dont_cache = G_svc_req.hdr.dont_cache;
//            G_connections[0].keep_content = G_svc_req.hdr.keep_content;
            G_connections[0].flags = G_svc_req.hdr.flags;

            /* For POST, the payload can be in the data space of the message,
               or -- if it's bigger -- in the shared memory */

//            if ( G_svc_req.hdr.post )
            if ( NPP_CONN_IS_PAYLOAD(G_svc_req.hdr.flags) )
            {
//                if ( G_svc_req.hdr.payload_location == ASYNC_PAYLOAD_MSG )
                if ( !NPP_ASYNC_IS_PAYLOAD_IN_SHM(G_svc_req.hdr.async_flags) )
                {
                    memcpy(G_connections[0].in_data, G_svc_req.data, G_svc_req.hdr.clen+1);
                }
                else    /* ASYNC_PAYLOAD_SHM */
                {
                    if ( G_connections[0].in_data_allocated < G_svc_req.hdr.clen+1 )
                    {
                        char *tmp = (char*)realloc(G_connections[0].in_data, G_svc_req.hdr.clen+1);
                        if ( !tmp )
                        {
                            ERR("Couldn't realloc in_data, tried %u bytes", G_svc_req.hdr.clen+1);
                            continue;
                        }
                        G_connections[0].in_data = tmp;
                        G_connections[0].in_data_allocated = G_svc_req.hdr.clen+1;
                        INF("Reallocated in_data, new size = %u bytes", G_svc_req.hdr.clen+1);
                    }

                    if ( !M_async_shm )
                    {
                        if ( (M_async_shm=npp_lib_shm_create(NPP_MAX_PAYLOAD_SIZE, 0)) == NULL )
                        {
                            ERR("Couldn't attach to SHM");
                            continue;
                        }
                    }

                    memcpy(G_connections[0].in_data, M_async_shm, G_svc_req.hdr.clen+1);

                    /* mark it as free */
                    M_async_shm[NPP_MAX_PAYLOAD_SIZE-1] = 0;
                }
            }

            /* session */

            memcpy(&G_sessions[1], &G_svc_req.hdr.eng_session_data, sizeof(eng_session_data_t));
#ifdef NPP_ASYNC_INCLUDE_SESSION_DATA
            memcpy(&G_app_session_data[1], &G_svc_req.hdr.app_session_data, sizeof(app_session_data_t));
#endif
            if ( G_sessions[1].sesid[0] )
                G_connections[0].si = 1;    /* user session present */
            else
            {
                G_connections[0].si = 0;    /* no session */
                strcpy(G_sessions[0].lang, G_connections[0].lang);  /* for npp_message and npp_lib_get_string */
            }

            /* globals */

            memcpy(&G_cnts_today, &G_svc_req.hdr.cnts_today, sizeof(npp_counters_t));
            memcpy(&G_cnts_yesterday, &G_svc_req.hdr.cnts_yesterday, sizeof(npp_counters_t));
            memcpy(&G_cnts_day_before, &G_svc_req.hdr.cnts_day_before, sizeof(npp_counters_t));

            G_days_up = G_svc_req.hdr.days_up;
            G_connections_cnt = G_svc_req.hdr.connections_cnt;
            G_connections_hwm = G_svc_req.hdr.connections_hwm;
            G_sessions_cnt = G_svc_req.hdr.sessions_cnt;
            G_sessions_hwm = G_svc_req.hdr.sessions_hwm;

            G_blacklist_cnt = G_svc_req.hdr.blacklist_cnt;

            strcpy(G_last_modified, G_svc_req.hdr.last_modified);

            /* ----------------------------------------------------------- */

            DBG("Processing...");

            /* response data */

#ifdef NPP_OUT_CHECK_REALLOC
            G_svc_p_content = G_svc_out_data;
#else
            G_svc_p_content = G_svc_res.data;
#endif
            /* ----------------------------------------------------------- */

            npp_svc_main(0);

            /* ----------------------------------------------------------- */

            if ( NPP_ASYNC_IS_WANT_RESPONSE(G_svc_req.hdr.async_flags) )
            {
                DBG_T("Sending response...");

                G_svc_res.hdr.err_code = G_error_code;

                G_svc_res.hdr.status = G_connections[0].status;
                strcpy(G_svc_res.hdr.cust_headers, G_connections[0].cust_headers);
                G_svc_res.hdr.cust_headers_len = G_connections[0].cust_headers_len;
                G_svc_res.hdr.ctype = G_connections[0].ctype;
                strcpy(G_svc_res.hdr.ctypestr, G_connections[0].ctypestr);
                strcpy(G_svc_res.hdr.cdisp, G_connections[0].cdisp);
                strcpy(G_svc_res.hdr.cookie_out_a, G_connections[0].cookie_out_a);
                strcpy(G_svc_res.hdr.cookie_out_a_exp, G_connections[0].cookie_out_a_exp);
                strcpy(G_svc_res.hdr.cookie_out_l, G_connections[0].cookie_out_l);
                strcpy(G_svc_res.hdr.cookie_out_l_exp, G_connections[0].cookie_out_l_exp);
                strcpy(G_svc_res.hdr.location, G_connections[0].location);
//                G_svc_res.hdr.dont_cache = G_connections[0].dont_cache;
//                G_svc_res.hdr.keep_content = G_connections[0].keep_content;

                G_svc_res.hdr.call_http_status = G_call_http_status;
                G_svc_res.hdr.call_http_req_cnt = G_call_http_req_cnt;  /* only for this async call */
                G_svc_res.hdr.call_http_elapsed = G_call_http_elapsed;  /* only for this async call */

                G_svc_res.hdr.flags = G_connections[0].flags;

                /* user session */

                memcpy(&G_svc_res.hdr.eng_session_data, &G_sessions[1], sizeof(eng_session_data_t));
#ifdef NPP_ASYNC_INCLUDE_SESSION_DATA
                memcpy(&G_svc_res.hdr.app_session_data, &G_app_session_data[1], sizeof(app_session_data_t));
#endif
                /* data */

                async_res_data_t resd;   /* different struct for more data */
                unsigned data_len, chunk_num=0, data_sent;
#ifdef NPP_OUT_CHECK_REALLOC
                data_len = G_svc_p_content - G_svc_out_data;
#else
                data_len = G_svc_p_content - G_svc_res.data;
#endif
                DBG("data_len = %u", data_len);

                G_svc_res.chunk = ASYNC_CHUNK_FIRST;

                G_async_res_data_size = ASYNC_RES_MSG_SIZE-sizeof(async_res_hdr_t)-sizeof(int)*4;

                if ( data_len <= G_async_res_data_size )
                {
                    G_svc_res.len = data_len;
#ifdef NPP_OUT_CHECK_REALLOC
                    memcpy(G_svc_res.data, G_svc_out_data, G_svc_res.len);
#endif
                    G_svc_res.chunk |= ASYNC_CHUNK_LAST;
                }
#ifdef NPP_OUT_CHECK_REALLOC
                else    /* we'll need more than one chunk */
                {
                    /* 0-th chunk */

                    G_svc_res.len = G_async_res_data_size;
                    memcpy(G_svc_res.data, G_svc_out_data, G_svc_res.len);

                    /* prepare the new struct for chunks > 0 */

                    resd.ai = G_svc_req.hdr.ai;
                    resd.ci = G_svc_req.hdr.ci;

                    G_async_res_data_size = ASYNC_RES_MSG_SIZE-sizeof(int)*4;
                }

                /* send first chunk (G_svc_res) */

                DBG("Sending 0-th chunk, chunk data length = %d", G_svc_res.len);

                if ( mq_send(G_queue_res, (char*)&G_svc_res, ASYNC_RES_MSG_SIZE, 0) != 0 )
                    ERR("mq_send failed, errno = %d (%s)", errno, strerror(errno));

                data_sent = G_svc_res.len;

                DBG("data_sent = %u", data_sent);

                /* next chunks if required (resd) */

                while ( data_sent < data_len )
                {
                    resd.chunk = ++chunk_num;

                    if ( data_len-data_sent <= G_async_res_data_size )   /* last chunk */
                    {
                        DBG("data_len-data_sent = %d, last chunk...", data_len-data_sent);
                        resd.len = data_len - data_sent;
                        resd.chunk |= ASYNC_CHUNK_LAST;
                    }
                    else
                    {
                        resd.len = G_async_res_data_size;
                    }

                    memcpy(resd.data, G_svc_out_data+data_sent, resd.len);

                    DBG("Sending %u-th chunk, chunk data length = %d", chunk_num, resd.len);

                    if ( mq_send(G_queue_res, (char*)&resd, ASYNC_RES_MSG_SIZE, 0) != 0 )
                        ERR("mq_send failed, errno = %d (%s)", errno, strerror(errno));

                    data_sent += resd.len;

                    DBG("data_sent = %u", data_sent);
                }

#endif  /* NPP_OUT_CHECK_REALLOC */

                DBG("Sent\n");
            }
            else
            {
                DBG("Response not required\n");
            }

            /* ----------------------------------------------------------- */

            npp_log_flush();
        }
    }

    clean_up();

    return EXIT_SUCCESS;
}


/* --------------------------------------------------------------------------
   Start new anonymous user session

   It's passed back to the main app process (npp_app)
   which really then starts a new session.

   It is optimistically assumed here that in the meantime NPP_MAX_SESSIONS
   won't be reached in any of the npp_* processes.
-------------------------------------------------------------------------- */
int npp_eng_session_start(int ci, const char *sesid)
{
    char new_sesid[NPP_SESSID_LEN+1];

    DBG("npp_eng_session_start");

    if ( G_sessions_cnt == NPP_MAX_SESSIONS )
    {
        WAR("User sessions exhausted");
        return ERR_SERVER_TOOBUSY;
    }

    ++G_sessions_cnt;

    G_connections[ci].si = 1;

    npp_random(new_sesid, NPP_SESSID_LEN);

    INF("Starting new session, sesid [%s]", new_sesid);

    strcpy(SESSION.sesid, new_sesid);
    strcpy(SESSION.ip, G_connections[ci].ip);
    strcpy(SESSION.uagent, G_connections[ci].uagent);
    strcpy(SESSION.referer, G_connections[ci].referer);
    strcpy(SESSION.lang, G_connections[ci].lang);
    SESSION.formats = G_connections[ci].formats;

    /* set 'as' cookie */

    strcpy(G_connections[ci].cookie_out_a, new_sesid);

    DBG("%d user session(s)", G_sessions_cnt);

    if ( G_sessions_cnt > G_sessions_hwm )
        G_sessions_hwm = G_sessions_cnt;

    return OK;
}


/* --------------------------------------------------------------------------
   Invalidate active user sessions belonging to user_id
   Called after password change
-------------------------------------------------------------------------- */
void npp_eng_session_downgrade_by_uid(int user_id, int ci)
{
    G_svc_res.hdr.invalidate_uid = user_id;
    G_svc_res.hdr.invalidate_ci = ci;
}


/* --------------------------------------------------------------------------
   Write string to output buffer with buffer overwrite protection
-------------------------------------------------------------------------- */
void npp_svc_out_check(const char *str)
{
    int available = G_async_res_data_size - (G_svc_p_content - G_svc_res.data);

    if ( strlen(str) < available )  /* the whole string will fit */
    {
        G_svc_p_content = stpcpy(G_svc_p_content, str);
    }
    else    /* let's write only what we can. WARNING: no UTF-8 checking is done here! */
    {
        G_svc_p_content = stpncpy(G_svc_p_content, str, available-1);
        *G_svc_p_content = EOS;
    }
}


/* --------------------------------------------------------------------------
   Write string to output buffer with buffer resizing if necessary
-------------------------------------------------------------------------- */
void npp_svc_out_check_realloc(const char *str)
{
    if ( strlen(str) < M_out_data_allocated - (G_svc_p_content - G_svc_out_data) )    /* the whole string will fit */
    {
        G_svc_p_content = stpcpy(G_svc_p_content, str);
    }
    else    /* resize output buffer and try again */
    {
        unsigned used = G_svc_p_content - G_svc_out_data;
        char *tmp = (char*)realloc(G_svc_out_data, M_out_data_allocated*2);
        if ( !tmp )
        {
            ERR("Couldn't reallocate output buffer, tried %u bytes", M_out_data_allocated*2);
            return;
        }
        G_svc_out_data = tmp;
        M_out_data_allocated = M_out_data_allocated * 2;
        G_svc_p_content = G_svc_out_data + used;
        INF("Reallocated output buffer, new size = %u bytes", M_out_data_allocated);
        npp_svc_out_check_realloc(str);     /* call itself! */
    }
}


/* --------------------------------------------------------------------------
   Write binary data to output buffer with buffer resizing if necessary
-------------------------------------------------------------------------- */
void npp_svc_out_check_realloc_bin(const char *data, int len)
{
    if ( len < M_out_data_allocated - (G_svc_p_content - G_svc_out_data) )    /* the whole data will fit */
    {
        memcpy(G_svc_p_content, data, len);
        G_svc_p_content += len;
    }
    else    /* resize output buffer and try again */
    {
        unsigned used = G_svc_p_content - G_svc_out_data;
        char *tmp = (char*)realloc(G_svc_out_data, M_out_data_allocated*2);
        if ( !tmp )
        {
            ERR("Couldn't reallocate output buffer, tried %u bytes", M_out_data_allocated*2);
            return;
        }
        G_svc_out_data = tmp;
        M_out_data_allocated = M_out_data_allocated * 2;
        G_svc_p_content = G_svc_out_data + used;
        INF("Reallocated output buffer, new size = %u bytes", M_out_data_allocated);
        npp_svc_out_check_realloc_bin(data, len);       /* call itself! */
    }
}


/* --------------------------------------------------------------------------
   Signal response
-------------------------------------------------------------------------- */
static void sigdisp(int sig)
{
    npp_update_time_globals();
    ALWAYS("");
    ALWAYS_T("Exiting due to receiving signal: %d", sig);
    clean_up();
    exit(1);
}


/* --------------------------------------------------------------------------
   Clean up
-------------------------------------------------------------------------- */
static void clean_up()
{
    ALWAYS("");
    ALWAYS("Cleaning up...\n");
    npp_log_memory();

    npp_svc_done();

    if ( access(M_pidfile, F_OK) != -1 )
    {
        DBG("Removing pid file...");
        char command[1024];
#ifdef _WIN32   /* Windows */
        sprintf(command, "del %s", M_pidfile);
#else
        sprintf(command, "rm %s", M_pidfile);
#endif
        if ( system(command) != EXIT_SUCCESS )
            WAR("Couldn't execute %s", command);
    }

#ifdef NPP_MYSQL
    npp_close_db();
#endif

    if (G_queue_req)
    {
        mq_close(G_queue_req);
        mq_unlink(G_req_queue_name);
    }
    if (G_queue_res)
    {
        mq_close(G_queue_res);
        mq_unlink(G_res_queue_name);
    }

    npp_lib_done();
}
