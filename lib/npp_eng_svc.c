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
    ASYNC services' engine module
    nodepp.org

-------------------------------------------------------------------------- */


#include "npp.h"


/* globals */

char        G_service[NPP_SVC_NAME_LEN+1];
int         G_error_code=OK;
int         G_svc_si=0;
int         G_ASYNCId=-1;
int         G_ASYNCSvcProcesses=0;
int         G_ASYNCDefTimeout=NPP_ASYNC_DEF_TIMEOUT;
#ifdef NPP_ASYNC
char        G_req_queue_name[256]="";
char        G_res_queue_name[256]="";
mqd_t       G_queue_req={0};                /* request queue */
mqd_t       G_queue_res={0};                /* response queue */
int         G_async_req_data_size=NPP_ASYNC_REQ_MSG_SIZE-sizeof(async_req_hdr_t); /* how many bytes are left for data */
int         G_async_res_data_size=NPP_ASYNC_RES_MSG_SIZE-sizeof(async_res_hdr_t)-sizeof(int)*4; /* how many bytes are left for data */
#endif  /* NPP_ASYNC */
int         G_usersRequireActivation=0;
async_req_t G_svc_req;
async_res_t G_svc_res;
#ifdef NPP_OUT_CHECK_REALLOC
char        *G_svc_out_data=NULL;
#endif
char        *G_svc_p_content=NULL;
npp_connection_t G_connections[NPP_MAX_CONNECTIONS+2]={0};  /* request details */
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
#ifdef NPP_ASYNC    /* suppress warning */
static char *M_async_shm=NULL;
#endif  /* NPP_ASYNC */
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
    /* library init ------------------------------------------------------ */

    if ( !npp_lib_init(FALSE, NULL) )
        return EXIT_FAILURE;

    /* start log --------------------------------------------------------- */

    char logprefix[64];

    sprintf(logprefix, "s_%d", G_pid);

    if ( !npp_log_start(logprefix, G_test, FALSE) )
    {
        npp_lib_done();
        return EXIT_FAILURE;
    }

    /* pid file ---------------------------------------------------------- */

    if ( !(M_pidfile=npp_lib_create_pid_file(logprefix)) )
    {
        npp_lib_done();
        return EXIT_FAILURE;
    }

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

#ifdef NPP_ASYNC

    if ( !(G_connections[0].in_data = (char*)malloc(G_async_req_data_size)) )
    {
        ERR("malloc for G_connections[0].in_data failed");
        npp_lib_done();
        return EXIT_FAILURE;
    }

    G_connections[0].in_data_allocated = G_async_req_data_size;

#ifdef NPP_OUT_CHECK_REALLOC

    if ( !(G_svc_out_data = (char*)malloc(NPP_OUT_BUFSIZE)) )
    {
        ERR("malloc for G_svc_out_data failed");
        npp_lib_done();
        return EXIT_FAILURE;
    }

    M_out_data_allocated = NPP_OUT_BUFSIZE;

#endif  /* NPP_OUT_CHECK_REALLOC */


    /* load snippets ----------------------------------------------------- */

    if ( !npp_lib_read_snippets("", 0, "snippets", TRUE, NULL) )
    {
        ERR("npp_lib_read_snippets() failed");
        npp_lib_done();
        return EXIT_FAILURE;
    }

#ifdef NPP_MULTI_HOST   /* side gigs */

    int i;

    for ( i=1; i<G_hosts_cnt; ++i )
    {
        if ( G_hosts[i].snippets[0] && !npp_lib_read_snippets(G_hosts[i].host, i, G_hosts[i].snippets, TRUE, NULL) )
        {
            ERR("reading %s's snippets failed", G_hosts[i].host);
            npp_lib_done();
            return EXIT_FAILURE;
        }
    }

#endif  /* NPP_MULTI_HOST */

    qsort(&G_snippets, G_snippets_cnt, sizeof(G_snippets[0]), lib_compare_snippets);


    /* open queues ------------------------------------------------------- */

#ifdef NPP_ASYNC_ID
    if ( G_ASYNCId > -1 )
    {
        sprintf(G_req_queue_name, "%s_%d__%d", NPP_ASYNC_REQ_QUEUE, NPP_ASYNC_ID, G_ASYNCId);
        sprintf(G_res_queue_name, "%s_%d__%d", NPP_ASYNC_RES_QUEUE, NPP_ASYNC_ID, G_ASYNCId);
    }
    else
    {
        sprintf(G_req_queue_name, "%s_%d", NPP_ASYNC_REQ_QUEUE, NPP_ASYNC_ID);
        sprintf(G_res_queue_name, "%s_%d", NPP_ASYNC_RES_QUEUE, NPP_ASYNC_ID);
    }
#else
    if ( G_ASYNCId > -1 )
    {
        sprintf(G_req_queue_name, "%s__%d", NPP_ASYNC_REQ_QUEUE, G_ASYNCId);
        sprintf(G_res_queue_name, "%s__%d", NPP_ASYNC_RES_QUEUE, G_ASYNCId);
    }
    else
    {
        strcpy(G_req_queue_name, NPP_ASYNC_REQ_QUEUE);
        strcpy(G_res_queue_name, NPP_ASYNC_RES_QUEUE);
    }
#endif  /* NPP_ASYNC_ID */

    DBG("Opening G_req_queue_name [%s]", G_req_queue_name);

    G_queue_req = mq_open(G_req_queue_name, O_RDONLY, NULL, NULL);

    if ( G_queue_req < 0 )
    {
        ERR("mq_open for req failed, errno = %d (%s)", errno, strerror(errno));
        npp_lib_done();
        return EXIT_FAILURE;
    }

    INF("mq_open of %s OK", G_req_queue_name);

    DBG("Opening G_res_queue_name [%s]", G_res_queue_name);

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

    INF("Sorting messages...");

    npp_sort_messages();


    INF("Sorting strings...");

    lib_sort_strings();

    /* ------------------------------------------------------------------- */

    G_initialized = 1;

    int prev_day = G_ptm->tm_mday;

    INF("\nWaiting...\n");

    while ( TRUE )
    {
        G_call_http_req_cnt = 0;
        G_call_http_elapsed = 0;
        G_call_http_average = 0;

        if ( mq_receive(G_queue_req, (char*)&G_svc_req, NPP_ASYNC_REQ_MSG_SIZE, NULL) != -1 )
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

                if ( !npp_lib_read_snippets("", 0, "snippets", FALSE, NULL) )
                {
                    ERR("npp_lib_read_snippets() failed");
                    clean_up();
                    return EXIT_FAILURE;
                }

#ifdef NPP_MULTI_HOST   /* side gigs */

                for ( i=1; i<G_hosts_cnt; ++i )
                {
                    if ( G_hosts[i].snippets[0] && !npp_lib_read_snippets(G_hosts[i].host, i, G_hosts[i].snippets, TRUE, NULL) )
                    {
                        ERR("reading %s's snippets failed", G_hosts[i].host);
                        npp_lib_done();
                        return EXIT_FAILURE;
                    }
                }

#endif  /* NPP_MULTI_HOST */

                qsort(&G_snippets, G_snippets_cnt, sizeof(G_snippets[0]), lib_compare_snippets);
            }

            DBG_T("Message received");

            if ( G_logLevel > LOG_INF )
                DBG_T("ci=%d, service [%s], call_id=%u", G_svc_req.hdr.ci, G_svc_req.hdr.service, G_svc_req.hdr.call_id);
            else
                INF_T("%s called (call_id=%u)", G_svc_req.hdr.service, G_svc_req.hdr.call_id);

            memset(&G_svc_res, 0, NPP_ASYNC_RES_MSG_SIZE);

            G_svc_res.ai = G_svc_req.hdr.ai;
            G_svc_res.ci = G_svc_req.hdr.ci;
            strcpy(G_service, G_svc_req.hdr.service);

            /* request details */

            strcpy(G_connections[0].ip, G_svc_req.hdr.ip);
            strcpy(G_connections[0].method, G_svc_req.hdr.method);
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
            G_connections[0].out_ctype = G_svc_req.hdr.out_ctype;
            strcpy(G_connections[0].ctypestr, G_svc_req.hdr.ctypestr);
            strcpy(G_connections[0].cdisp, G_svc_req.hdr.cdisp);
            strcpy(G_connections[0].cookie_out_a, G_svc_req.hdr.cookie_out_a);
            strcpy(G_connections[0].cookie_out_a_exp, G_svc_req.hdr.cookie_out_a_exp);
            strcpy(G_connections[0].cookie_out_l, G_svc_req.hdr.cookie_out_l);
            strcpy(G_connections[0].cookie_out_l_exp, G_svc_req.hdr.cookie_out_l_exp);
            strcpy(G_connections[0].location, G_svc_req.hdr.location);
            G_svc_si = G_svc_req.hdr.si;    /* original si */
            G_connections[0].flags = G_svc_req.hdr.flags;

            /* For POST, the payload can be in the data space of the message,
               or -- if it's bigger -- in the shared memory */

            if ( NPP_CONN_IS_PAYLOAD(G_svc_req.hdr.flags) && G_svc_req.hdr.clen > 0 )
            {
                if ( !NPP_ASYNC_IS_PAYLOAD_IN_SHM(G_svc_req.hdr.async_flags) )
                {
                    memcpy(G_connections[0].in_data, G_svc_req.data, G_svc_req.hdr.clen+1);
                }
                else    /* shared memory */
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
            if ( G_sessions[1].sessid[0] )
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

            /* response data */

#ifdef NPP_OUT_CHECK_REALLOC
            G_svc_p_content = G_svc_out_data;
#else
            G_svc_p_content = G_svc_res.data;
#endif
            /* ----------------------------------------------------------- */

            DBG("Calling npp_svc_main...");

            npp_svc_main(0);

            /* ----------------------------------------------------------- */

            if ( NPP_ASYNC_IS_WANT_RESPONSE(G_svc_req.hdr.async_flags) )
            {
                DBG_T("Sending response...");

                G_svc_res.hdr.err_code = G_error_code;

                G_svc_res.hdr.status = G_connections[0].status;
                strcpy(G_svc_res.hdr.cust_headers, G_connections[0].cust_headers);
                G_svc_res.hdr.cust_headers_len = G_connections[0].cust_headers_len;
                G_svc_res.hdr.out_ctype = G_connections[0].out_ctype;
                strcpy(G_svc_res.hdr.ctypestr, G_connections[0].ctypestr);
                strcpy(G_svc_res.hdr.cdisp, G_connections[0].cdisp);
                strcpy(G_svc_res.hdr.cookie_out_a, G_connections[0].cookie_out_a);
                strcpy(G_svc_res.hdr.cookie_out_a_exp, G_connections[0].cookie_out_a_exp);
                strcpy(G_svc_res.hdr.cookie_out_l, G_connections[0].cookie_out_l);
                strcpy(G_svc_res.hdr.cookie_out_l_exp, G_connections[0].cookie_out_l_exp);
                strcpy(G_svc_res.hdr.location, G_connections[0].location);

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

                unsigned data_len, chunk_num=0, data_sent;

#ifdef NPP_OUT_CHECK_REALLOC

                async_res_data_t resd;   /* different struct for more data */

                data_len = G_svc_p_content - G_svc_out_data;

#else
                data_len = G_svc_p_content - G_svc_res.data;
#endif

                DBG("data_len = %u", data_len);

                G_svc_res.chunk = ASYNC_CHUNK_FIRST;

                G_async_res_data_size = NPP_ASYNC_RES_MSG_SIZE-sizeof(async_res_hdr_t)-sizeof(int)*4;

                if ( data_len <= (unsigned)G_async_res_data_size )
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

                    G_async_res_data_size = NPP_ASYNC_RES_MSG_SIZE-sizeof(int)*4;
                }

                /* send first chunk (G_svc_res) */

                DBG("Sending 0-th chunk, chunk data length = %d", G_svc_res.len);

                if ( mq_send(G_queue_res, (char*)&G_svc_res, NPP_ASYNC_RES_MSG_SIZE, 0) != 0 )
                    ERR("mq_send failed, errno = %d (%s)", errno, strerror(errno));

                data_sent = G_svc_res.len;

                DBG("data_sent = %u", data_sent);

                /* next chunks if required (resd) */

                while ( data_sent < data_len )
                {
                    resd.chunk = ++chunk_num;

                    if ( data_len-data_sent <= (unsigned)G_async_res_data_size )   /* last chunk */
                    {
                        DDBG("data_len-data_sent = %u, last chunk...", data_len-data_sent);
                        resd.len = data_len - data_sent;
                        resd.chunk |= ASYNC_CHUNK_LAST;
                    }
                    else
                    {
                        resd.len = G_async_res_data_size;
                    }

                    memcpy(resd.data, G_svc_out_data+data_sent, resd.len);

                    DBG("Sending %u-th chunk, chunk data length = %d", chunk_num, resd.len);

                    if ( mq_send(G_queue_res, (char*)&resd, NPP_ASYNC_RES_MSG_SIZE, 0) != 0 )
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

#endif  /* NPP_ASYNC */

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
int npp_eng_session_start(int ci, const char *sessid)
{
    char new_sessid[NPP_SESSID_LEN+1];

    DBG("npp_eng_session_start");

    if ( G_sessions_cnt == NPP_MAX_SESSIONS )
    {
        WAR("User sessions exhausted");
        return ERR_SERVER_TOOBUSY;
    }

    ++G_sessions_cnt;

    G_connections[ci].si = 1;

    strcpy(new_sessid, npp_random(NPP_SESSID_LEN));

#ifdef NPP_DEBUG
    INF("Starting new session, sessid [%s]", new_sessid);
#else
    INF("Starting new session");
#endif

    strcpy(SESSION.sessid, new_sessid);
    strcpy(SESSION.ip, G_connections[ci].ip);
    strcpy(SESSION.uagent, G_connections[ci].uagent);
    strcpy(SESSION.referer, G_connections[ci].referer);
    strcpy(SESSION.lang, G_connections[ci].lang);
    SESSION.formats = G_connections[ci].formats;

    /* set 'as' cookie */

    strcpy(G_connections[ci].cookie_out_a, new_sessid);

    DBG("%d session(s)", G_sessions_cnt);

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
#ifdef NPP_ASYNC
    size_t available = G_async_res_data_size - (G_svc_p_content - G_svc_res.data);

    if ( strlen(str) < available )  /* the whole string will fit */
    {
        G_svc_p_content = stpcpy(G_svc_p_content, str);
    }
    else    /* let's write only what we can. WARNING: no UTF-8 checking is done here! */
    {
        G_svc_p_content = stpncpy(G_svc_p_content, str, available-1);
        *G_svc_p_content = EOS;
    }
#endif  /* NPP_ASYNC */
}


/* --------------------------------------------------------------------------
   Write string to output buffer with buffer resizing if necessary
-------------------------------------------------------------------------- */
void npp_svc_out_check_realloc(const char *str)
{
    if ( strlen(str) < M_out_data_allocated - (unsigned)(G_svc_p_content-G_svc_out_data) )    /* the whole string will fit */
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
        remove(M_pidfile);
    }

#ifdef NPP_ASYNC

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

#endif  /* NPP_ASYNC */

    npp_lib_done();
}
