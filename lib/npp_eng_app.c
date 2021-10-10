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
    Main application engine module
    nodepp.org

-------------------------------------------------------------------------- */


#include "npp.h"


#ifdef NPP_FD_MON_POLL
#ifndef _WIN32
#include <poll.h>
#endif
#endif  /* NPP_FD_MON_POLL */

#ifndef _WIN32
#include <zlib.h>
#endif


/* globals */

/* read from the config file */

int         G_httpPort=80;
int         G_httpsPort=443;
char        G_cipherList[1024]="";
char        G_certFile[256]="";
char        G_certChainFile[256]="";
char        G_keyFile[256]="";
int         G_usersRequireActivation=0;
char        G_blockedIPList[256]="";
char        G_whiteList[256]="";
int         G_ASYNCId=0;
int         G_ASYNCDefTimeout=NPP_ASYNC_DEF_TIMEOUT;

/* end of config params */

int         G_days_up=0;
npp_connection_t G_connections[NPP_MAX_CONNECTIONS+1]={0};
int         G_connections_cnt=0;
int         G_connections_hwm=0;
eng_session_data_t G_sessions[NPP_MAX_SESSIONS+1]={0};
app_session_data_t G_app_session_data[NPP_MAX_SESSIONS+1]={0};
int         G_sessions_cnt=0;
int         G_sessions_hwm=0;
char        G_last_modified[32]="";

/* asynchorous processing */

#ifndef _WIN32
char        G_req_queue_name[256]="";
char        G_res_queue_name[256]="";
mqd_t       G_queue_req={0};                /* request queue */
mqd_t       G_queue_res={0};                /* response queue */
#endif  /* _WIN32 */
int         G_async_req_data_size=ASYNC_REQ_MSG_SIZE-sizeof(async_req_hdr_t); /* how many bytes are left for data */
int         G_async_res_data_size=ASYNC_RES_MSG_SIZE-sizeof(async_res_hdr_t)-sizeof(int)*4; /* how many bytes are left for data */

bool        G_index_present=FALSE;          /* index.html present in res? */

char        G_blacklist[NPP_MAX_BLACKLIST+1][INET_ADDRSTRLEN];
int         G_blacklist_cnt=0;              /* G_blacklist length */

char        G_whitelist[NPP_MAX_WHITELIST+1][INET_ADDRSTRLEN];
int         G_whitelist_cnt=0;              /* G_whitelist length */

/* counters */

npp_counters_t G_cnts_today={0};            /* today's counters */
npp_counters_t G_cnts_yesterday={0};        /* yesterday's counters */
npp_counters_t G_cnts_day_before={0};       /* day before's counters */


/* locals */

http_status_t   M_http_status[]={
        {101, "Switching Protocols"},
        {200, "OK"},
        {201, "Created"},
        {206, "Partial Content"},
        {301, "Moved Permanently"},
        {302, "Found"},
        {303, "See Other"},
        {304, "Not Modified"},
        {400, "Bad Request"},
        {401, "Unauthorized"},
        {403, "Forbidden"},
        {404, "Not Found"},
        {405, "Method Not Allowed\r\nAllow: GET, POST, PUT, DELETE, OPTIONS, HEAD"},
        {413, "Request Entity Too Large"},
        {414, "Request-URI Too Long"},
        {416, "Range Not Satisfiable"},
        {500, "Internal Server Error"},
        {501, "Not Implemented"},
        {503, "Service Unavailable"},
        { -1, ""}
    };


/* authorization levels */

static struct {
    char path[256];
    char level;
    } M_auth_levels[NPP_MAX_RESOURCES] = {
        {"-", EOS}
    };


/* hosts */

#ifdef NPP_MULTI_HOST
static struct {
    char host[NPP_MAX_HOST_LEN+1];
    char res[256];
    char resmin[256];
    char snippets[256];
    bool index_present;
    } M_hosts[NPP_MAX_HOSTS] = {
        {"", "res", "resmin", "snippets", FALSE}
    };

static int      M_hosts_cnt=1;              /* main host always present */
#endif  /* NPP_MULTI_HOST */

static char     *M_pidfile;                 /* pid file name */

#ifdef _WIN32   /* Windows */
static SOCKET   M_listening_fd=0;           /* The socket file descriptor for "listening" socket */
static SOCKET   M_listening_sec_fd=0;       /* The socket file descriptor for secure "listening" socket */
#else
static int      M_listening_fd=0;           /* The socket file descriptor for "listening" socket */
static int      M_listening_sec_fd=0;       /* The socket file descriptor for secure "listening" socket */
#endif  /* _WIN32 */

#ifdef NPP_HTTPS
static SSL_CTX  *M_ssl_ctx;
#endif

#ifdef NPP_HTTPS
#define NPP_LISTENING_FDS   2
#else
#define NPP_LISTENING_FDS   1
#endif

#ifdef NPP_FD_MON_SELECT
static fd_set       M_readfds={0};              /* Socket file descriptors we want to wake up for, using select() */
static fd_set       M_writefds={0};             /* Socket file descriptors we want to wake up for, using select() */
static int          M_highsock=0;               /* Highest #'d file descriptor, needed for select() */
#endif  /* NPP_FD_MON_SELECT */

#ifdef NPP_FD_MON_POLL

static struct pollfd M_pollfds[NPP_MAX_CONNECTIONS+NPP_LISTENING_FDS]={0};
static int          M_pollfds_cnt=0;
static int          M_poll_ci[NPP_MAX_CONNECTIONS+NPP_LISTENING_FDS]={0};

#endif  /* NPP_FD_MON_POLL */

static static_res_t M_stat[NPP_MAX_STATICS]={0};    /* static resources */
static char         M_resp_date[32];                /* response header Date */
static char         M_expires_stat[32];             /* response header for static resources */
static char         M_expires_gen[32];              /* response header for generated resources */
static int          M_static_cnt=-1;                /* highest static resource M_stat index */
static bool         M_popular_favicon=FALSE;        /* popular statics -- don't create sessions for those */
static bool         M_popular_appleicon=FALSE;      /* -''- */
static bool         M_popular_robots=FALSE;         /* -''- */
static bool         M_popular_sw=FALSE;             /* -''- */

#ifdef _WIN32   /* Windows */
WSADATA             wsa;
#endif

static bool         M_shutdown=FALSE;
static int          M_prev_minute;
static int          M_prev_day;
static time_t       M_last_housekeeping=0;

#ifdef NPP_ASYNC
static areq_t       M_areqs[ASYNC_MAX_REQUESTS]={0}; /* async requests */
static unsigned     M_last_call_id=0;               /* counter */
static char         *M_async_shm=NULL;
#endif  /* NPP_ASYNC */


/* prototypes */

static bool housekeeping(void);
#ifdef NPP_HTTP2
static void http2_check_client_preface(int ci);
static void http2_parse_frame(int ci, int bytes);
static void http2_add_frame(int ci, unsigned char type);
#endif  /* NPP_HTTP2 */
static void set_state(int ci, int bytes);
static void set_state_sec(int ci, int bytes);
static void read_conf(void);
static void respond_to_expect(int ci);
static void log_proc_time(int ci);
static void log_request(int ci);
static void close_conn(int ci);
static bool init(int argc, char **argv);
static void build_fd_sets(void);
static void accept_http();
static void accept_https();
static void read_blocked_ips(void);
static bool ip_blocked(const char *addr);
static void read_allowed_ips(void);
static bool ip_allowed(const char *addr);
static int  first_free_stat(void);
static bool read_resources(bool first_scan);
static int  is_static_res(int ci);
static void process_req(int ci);
static unsigned deflate_data(unsigned char *dest, const unsigned char *src, unsigned src_len);
static void gen_response_header(int ci);
static void print_content_type(int ci, char type);
static bool a_usession_ok(int ci);
static void close_old_conn(void);
static void uses_close_timeouted(void);
static void close_uses(int si, int ci);
static void reset_conn(int ci, char new_state);
static int  parse_req(int ci, int len);
static int  set_http_req_val(int ci, const char *label, const char *value);
static bool check_block_ip(int ci, const char *rule, const char *value);
static char *get_http_descr(int status_code);
static void dump_counters(void);
static void clean_up(void);
static void sigdisp(int sig);
static void render_page_msg(int ci, int code);
static bool init_ssl(void);



/* --------------------------------------------------------------------------
   main
-------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    struct sockaddr_in serv_addr;
    unsigned    hit=0;
    int         reuse_addr=1;       /* Used so we can re-bind to our port while a previous connection is still in TIME_WAIT state */
    struct timeval timeout;         /* Timeout for select */
    int         sockets_ready;      /* Number of sockets ready for I/O */
    int         i=0;
    int         bytes=0;
    int         failed_select_cnt=0;
    int         j=0;
#ifdef NPP_DEBUG
    time_t      dbg_last_time0=0;
    time_t      dbg_last_time1=0;
    time_t      dbg_last_time2=0;
    time_t      dbg_last_time3=0;
    time_t      dbg_last_time4=0;
    time_t      dbg_last_time5=0;
    time_t      dbg_last_time6=0;
    time_t      dbg_last_time7=0;
    time_t      dbg_last_time8=0;
    time_t      dbg_last_time9=0;
#endif  /* NPP_DEBUG */

    if ( !init(argc, argv) )
    {
        ERR("init() failed, exiting");
        clean_up();
        return EXIT_FAILURE;
    }

    /* setup the network socket */

    DBG("Trying socket...");

#ifdef _WIN32   /* Windows */

    DBG("Initializing Winsock...");

    if ( WSAStartup(MAKEWORD(2,2), &wsa) != 0 )
    {
        ERR("WSAStartup failed. Error Code = %d", WSAGetLastError());
        clean_up();
        return EXIT_FAILURE;
    }

#endif  /* _WIN32 */

    if ( (M_listening_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        ERR("socket failed, errno = %d (%s)", errno, strerror(errno));
        clean_up();
        return EXIT_FAILURE;
    }

    DBG("M_listening_fd = %d", M_listening_fd);

    /* So that we can re-bind to it without TIME_WAIT problems */
#ifdef _WIN32   /* Windows */
    setsockopt(M_listening_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_addr, sizeof(reuse_addr));
#else
    setsockopt(M_listening_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
#endif 

    /* Set socket to non-blocking */

    npp_lib_setnonblocking(M_listening_fd);

    /* bind socket to a port */

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(G_httpPort);

    DBG("Trying bind to port %d...", G_httpPort);

    if ( bind(M_listening_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0 )
    {
        ERR("bind failed, errno = %d (%s)", errno, strerror(errno));
        clean_up();
        return EXIT_FAILURE;
    }

    /* listen to a port */

    DBG("Trying listen...\n");

    if ( listen(M_listening_fd, SOMAXCONN) < 0 )
    {
        ERR("listen failed, errno = %d (%s)", errno, strerror(errno));
        clean_up();
        return EXIT_FAILURE;
    }

    /* repeat everything for port 443 */

#ifdef NPP_HTTPS

    DBG("Trying socket for secure connections...");

    if ( (M_listening_sec_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        ERR("socket failed, errno = %d (%s)", errno, strerror(errno));
        clean_up();
        return EXIT_FAILURE;
    }

    DBG("M_listening_sec_fd = %d", M_listening_sec_fd);

    /* So that we can re-bind to it without TIME_WAIT problems */
#ifdef _WIN32   /* Windows */
    setsockopt(M_listening_sec_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_addr, sizeof(reuse_addr));
#else
    setsockopt(M_listening_sec_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
#endif

    /* Set socket to non-blocking */

    npp_lib_setnonblocking(M_listening_sec_fd);

    /* bind socket to a port */

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(G_httpsPort);

    DBG("Trying bind to port %d...", G_httpsPort);

    if ( bind(M_listening_sec_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0 )
    {
        ERR("bind failed, errno = %d (%s)", errno, strerror(errno));
        clean_up();
        return EXIT_FAILURE;
    }

    /* listen to a port */

    DBG("Trying listen...\n");

    if ( listen(M_listening_sec_fd, SOMAXCONN) < 0 )
    {
        ERR("listen failed, errno = %d (%s)", errno, strerror(errno));
        clean_up();
        return EXIT_FAILURE;
    }

#endif  /* NPP_HTTPS */

    /* log currently used memory */

    npp_log_memory();

    ALWAYS("\nWaiting for requests...\n");

    log_flush();

    M_prev_minute = G_ptm->tm_min;
    M_prev_day = G_ptm->tm_mday;

    /* main server loop ------------------------------------------------------------------------- */

#ifdef NPP_FD_MON_POLL

    M_pollfds[0].fd = M_listening_fd;
    M_pollfds[0].events = POLLIN;
    M_pollfds_cnt = 1;

#ifdef NPP_HTTPS
    M_pollfds[1].fd = M_listening_sec_fd;
    M_pollfds[1].events = POLLIN;
    M_pollfds_cnt = 2;
#endif

    int pi;     /* poll loop index */

#endif  /* NPP_FD_MON_POLL */

//  for ( ; hit<1000; ++hit )   /* test only */
    for ( ;; )
    {
        npp_update_time_globals();

#ifdef _WIN32   /* Windows */
        strftime(M_resp_date, 32, "%a, %d %b %Y %H:%M:%S GMT", G_ptm);
#else
        strftime(M_resp_date, 32, "%a, %d %b %Y %T GMT", G_ptm);
#endif  /* _WIN32 */

#ifdef NPP_ASYNC
        /* release timeout-ed */

        for ( j=0; j<ASYNC_MAX_REQUESTS; ++j )
        {
            if ( M_areqs[j].state==NPP_ASYNC_STATE_SENT && M_areqs[j].sent < G_now-M_areqs[j].timeout )
            {
                DBG("Async request %d timeout-ed", j);
                G_connections[M_areqs[j].ci].async_err_code = ERR_ASYNC_TIMEOUT;
                G_connections[M_areqs[j].ci].status = 500;
                M_areqs[j].state = NPP_ASYNC_STATE_FREE;
                gen_response_header(M_areqs[j].ci);
            }
        }
#endif

        /* use your favourite fd monitoring */

#ifdef NPP_FD_MON_SELECT
        build_fd_sets();

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        sockets_ready = select(M_highsock+1, &M_readfds, &M_writefds, NULL, &timeout);
#endif  /* NPP_FD_MON_SELECT */

#ifdef NPP_FD_MON_POLL
        sockets_ready = poll(M_pollfds, M_pollfds_cnt, 1000);
#endif  /* NPP_FD_MON_POLL */

#ifdef NPP_FD_MON_EPOLL
        ALWAYS("epoll not implemented yet!");
        clean_up();
        return EXIT_FAILURE;
#endif  /* NPP_FD_MON_EPOLL */

#ifdef _WIN32
        if ( M_shutdown ) break;
#endif
        if ( sockets_ready < 0 )
        {
            ERR_T("select failed, errno = %d (%s)", errno, strerror(errno));
            /* protect from infinite loop */
            if ( failed_select_cnt >= 10 )
            {
                ERR("select failed for the 10-th time, entering emergency reset");
                ALWAYS("Resetting all connections...");
                int k;
                for ( k=0; k<NPP_MAX_CONNECTIONS; ++k )
                    close_conn(k);
                failed_select_cnt = 0;
                ALWAYS("Waiting for 1 second...");
#ifdef _WIN32
                Sleep(1000);
#else
                sleep(1);
#endif
                continue;
            }
            else
            {
                ++failed_select_cnt;
                continue;
            }
        }
        else if ( sockets_ready == 0 )
        {
            /* we have some time now, let's do some housekeeping */

//            DDBG("sockets_ready == 0");

            if ( !housekeeping() )
                return EXIT_FAILURE;
        }
        else    /* sockets_ready > 0 */
        {
#ifdef NPP_DEBUG
            if ( G_now != dbg_last_time0 )   /* only once in a second */
            {
                DBG("    connected = %d", G_connections_cnt);
#ifdef NPP_FD_MON_POLL
                DBG("M_pollfds_cnt = %d", M_pollfds_cnt);
#endif
                DBG("sockets_ready = %d", sockets_ready);
                DBG("");
                dbg_last_time0 = G_now;
            }
#endif  /* NPP_DEBUG */
#ifdef NPP_FD_MON_SELECT
            if ( FD_ISSET(M_listening_fd, &M_readfds) )   /* new http connection */
            {
#endif  /* NPP_FD_MON_SELECT */
#ifdef NPP_FD_MON_POLL
            if ( M_pollfds[0].revents & POLLIN )
            {
                M_pollfds[0].revents = 0;
#endif  /* NPP_FD_MON_POLL */
                accept_http();
                sockets_ready--;
            }
#ifdef NPP_HTTPS
#ifdef NPP_FD_MON_SELECT
            else if ( FD_ISSET(M_listening_sec_fd, &M_readfds) )   /* new https connection */
            {
#endif  /* NPP_FD_MON_SELECT */
#ifdef NPP_FD_MON_POLL
            else if ( M_pollfds[1].revents & POLLIN )
            {
                M_pollfds[1].revents = 0;
#endif  /* NPP_FD_MON_POLL */
                accept_https();
                sockets_ready--;
            }
#endif  /* NPP_HTTPS */
            else    /* existing connections have something going on on them ---------------------------------- */
            {
#ifdef NPP_FD_MON_SELECT
                for ( i=0; sockets_ready>0 && i<NPP_MAX_CONNECTIONS; ++i )
                {
#endif  /* NPP_FD_MON_SELECT */
#ifdef NPP_FD_MON_POLL
                for ( pi=NPP_LISTENING_FDS; sockets_ready>0 && pi<M_pollfds_cnt; ++pi )
                {
                    i = M_poll_ci[pi];   /* set G_connections array index */
#ifdef NPP_DEBUG
                    if ( G_now != dbg_last_time1 )   /* only once in a second */
                    {
                        int l;
                        for ( l=0; l<M_pollfds_cnt; ++l )
                            DBG("ci=%d, pi=%d, M_pollfds[pi].revents = %d", M_poll_ci[l], l, M_pollfds[l].revents);
                        dbg_last_time1 = G_now;
                    }
#endif  /* NPP_DEBUG */
#endif  /* NPP_FD_MON_POLL */
#ifdef NPP_FD_MON_SELECT
                    if ( FD_ISSET(G_connections[i].fd, &M_readfds) )     /* incoming data ready */
                    {
#endif  /* NPP_FD_MON_SELECT */
#ifdef NPP_FD_MON_POLL
                    if ( M_pollfds[pi].revents & POLLIN )
                    {
                        M_pollfds[pi].revents = 0;
#endif  /* NPP_FD_MON_POLL */
#ifdef NPP_DEBUG
                        if ( G_now != dbg_last_time2 )   /* only once in a second */
                        {
                            DBG("ci=%d, fd=%d has incoming data, conn_state = %c", i, G_connections[i].fd, G_connections[i].conn_state);
                            dbg_last_time2 = G_now;
                        }
#endif  /* NPP_DEBUG */
#ifdef NPP_HTTPS
                        if ( NPP_CONN_IS_SECURE(G_connections[i].flags) )   /* HTTPS */
                        {
                            if ( G_connections[i].conn_state != CONN_STATE_READING_DATA )
                            {
                                DDBG("Trying SSL_read from fd=%d (ci=%d)", G_connections[i].fd, i);

                                bytes = SSL_read(G_connections[i].ssl, G_connections[i].in, NPP_IN_BUFSIZE-1);

                                if ( bytes > 1 )
                                    G_connections[i].in[bytes] = EOS;
                                else if ( bytes == 1 )   /* when browser splits the request to prevent BEAST attack */
                                {
                                    bytes = SSL_read(G_connections[i].ssl, G_connections[i].in+1, NPP_IN_BUFSIZE-2) + 1;
                                    if ( bytes > 1 )
                                        G_connections[i].in[bytes] = EOS;
                                }
#ifdef NPP_HTTP2
                                if ( G_connections[i].http_ver[0] == '2' )
                                    http2_parse_frame(i, bytes);
                                else
#endif  /* NPP_HTTP2 */
                                    set_state_sec(i, bytes);
                            }
                            else    /* POST */
                            {
#ifdef NPP_DEBUG
                                DBG("ci=%d, state == CONN_STATE_READING_DATA", i);
                                DBG("Trying SSL_read %u bytes of POST data from fd=%d (ci=%d)", G_connections[i].clen-G_connections[i].was_read, G_connections[i].fd, i);
#endif  /* NPP_DEBUG */
                                bytes = SSL_read(G_connections[i].ssl, G_connections[i].in_data+G_connections[i].was_read, G_connections[i].clen-G_connections[i].was_read);

                                if ( bytes > 0 )
                                    G_connections[i].was_read += bytes;

#ifdef NPP_HTTP2
                                if ( G_connections[i].http_ver[0] == '2' )
                                    http2_parse_frame(i, bytes);
                                else
#endif  /* NPP_HTTP2 */
                                    set_state_sec(i, bytes);
                            }
                        }
                        else    /* HTTP */
#endif  /* NPP_HTTPS */
                        {
                            if ( G_connections[i].conn_state == CONN_STATE_CONNECTED )
                            {
#ifdef NPP_DEBUG
                                DBG("ci=%d, state == CONN_STATE_CONNECTED", i);
                                DBG("Trying read from fd=%d (ci=%d)", G_connections[i].fd, i);
#endif  /* NPP_DEBUG */
                                bytes = recv(G_connections[i].fd, G_connections[i].in, NPP_IN_BUFSIZE-1, 0);

                                if ( bytes > 0 )
                                    G_connections[i].in[bytes] = EOS;
#ifdef NPP_HTTP2
                                if ( G_connections[i].http_ver[0] == '2' && bytes > 0 )
                                    http2_parse_frame(i, bytes);
                                else
#endif  /* NPP_HTTP2 */
                                    set_state(i, bytes);    /* possibly:    CONN_STATE_DISCONNECTED (if error or closed by peer) */
                                                            /*              CONN_STATE_READY_FOR_PARSE */
                            }
#ifdef NPP_HTTP2
                            else if ( G_connections[i].conn_state == CONN_STATE_READY_FOR_CLIENT_PREFACE )
                            {
#ifdef NPP_DEBUG
                                DBG("ci=%d, state == CONN_STATE_READY_FOR_CLIENT_PREFACE", i);
                                DBG("Trying read from fd=%d (ci=%d)", G_connections[i].fd, i);
#endif  /* NPP_DEBUG */
                                bytes = recv(G_connections[i].fd, G_connections[i].in, NPP_IN_BUFSIZE-1, 0);

                                if ( bytes > 0 )
                                {
                                    G_connections[i].in[bytes] = EOS;
                                    http2_check_client_preface(i);   /* hopefully finish upgrade to HTTP/2 */
                                }
                                else
                                    set_state(i, bytes);    /* disconnected */
                            }
#endif  /* NPP_HTTP2 */
                            else if ( G_connections[i].conn_state == CONN_STATE_READING_DATA )   /* POST */
                            {
#ifdef NPP_DEBUG
                                DBG("ci=%d, state == CONN_STATE_READING_DATA", i);
                                DBG("Trying to read %u bytes of POST data from fd=%d (ci=%d)", G_connections[i].clen-G_connections[i].was_read, G_connections[i].fd, i);
#endif  /* NPP_DEBUG */
                                bytes = recv(G_connections[i].fd, G_connections[i].in_data+G_connections[i].was_read, G_connections[i].clen-G_connections[i].was_read, 0);

                                if ( bytes > 0 )
                                    G_connections[i].was_read += bytes;

#ifdef NPP_HTTP2
                                if ( G_connections[i].http_ver[0] == '2' )
                                    http2_parse_frame(i, bytes);
                                else
#endif  /* NPP_HTTP2 */
                                    set_state(i, bytes);    /* possibly:    CONN_STATE_DISCONNECTED (if error or closed by peer) */
                                                            /*              CONN_STATE_READY_FOR_PROCESS */
                            }
                        }

                        sockets_ready--;
                    }
                    /* --------------------------------------------------------------------------------------- */
#ifdef NPP_FD_MON_SELECT
                    else if ( FD_ISSET(G_connections[i].fd, &M_writefds) )        /* ready for outgoing data */
                    {
#endif  /* NPP_FD_MON_SELECT */
#ifdef NPP_FD_MON_POLL
                    else if ( M_pollfds[pi].revents & POLLOUT )
                    {
                        M_pollfds[pi].revents = 0;
#endif  /* NPP_FD_MON_POLL */
#ifdef NPP_DEBUG
                        if ( G_now != dbg_last_time3 )   /* only once in a second */
                        {
                            DBG("ci=%d, fd=%d is ready for outgoing data, conn_state = %c", i, G_connections[i].fd, G_connections[i].conn_state);
                            dbg_last_time3 = G_now;
                        }
#endif  /* NPP_DEBUG */

#ifdef NPP_HTTPS
                        if ( NPP_CONN_IS_SECURE(G_connections[i].flags) )   /* HTTPS */
                        {
                            if ( G_connections[i].conn_state == CONN_STATE_READY_TO_SEND_RESPONSE )
                            {
#ifdef NPP_DEBUG
                                DBG("ci=%d, state == CONN_STATE_READY_TO_SEND_RESPONSE", i);
                                DBG("Trying SSL_write %u bytes to fd=%d (ci=%d)", G_connections[i].out_len, G_connections[i].fd, i);
#endif  /* NPP_DEBUG */
                                bytes = SSL_write(G_connections[i].ssl, G_connections[i].out_start, G_connections[i].out_len);

                                set_state_sec(i, bytes);
                            }
                            else if ( G_connections[i].conn_state == CONN_STATE_SENDING_CONTENT)
                            {
#ifdef NPP_DEBUG
                                DBG("ci=%d, state == CONN_STATE_SENDING_CONTENT", i);
                                DBG("Trying SSL_write %u bytes to fd=%d (ci=%d)", G_connections[i].out_len-G_connections[i].data_sent, G_connections[i].fd, i);
#endif  /* NPP_DEBUG */
                                bytes = SSL_write(G_connections[i].ssl, G_connections[i].out_start, G_connections[i].out_len);

                                set_state_sec(i, bytes);
                            }
                        }
                        else    /* plain HTTP */
#endif  /* NPP_HTTPS */
                        {
                            if ( G_connections[i].conn_state == CONN_STATE_READY_TO_SEND_RESPONSE )
                            {
                                DDBG("ci=%d, state == CONN_STATE_READY_TO_SEND_RESPONSE", i);
#ifdef NPP_HTTP2
                                if ( G_connections[i].http2_upgrade_in_progress )    /* switching protocol in progress; send only 101 header, follow with settings frame */
                                {
                                    bytes = send(G_connections[i].fd, G_connections[i].out_start, G_connections[i].out_hlen, 0);

                                    http2_add_frame(i, HTTP2_FRAME_TYPE_SETTINGS);
#ifdef NPP_DEBUG
                                    DBG("ci=%d, Sending SETTINGS frame", i);
                                    DBG("Trying to write %u bytes to fd=%d (ci=%d)", G_connections[i].http2_bytes_to_send, G_connections[i].fd, i);
#endif  /* NPP_DEBUG */
                                    bytes = send(G_connections[i].fd, G_connections[i].http2_frame_start, G_connections[i].http2_bytes_to_send, 0);

                                    DDBG("ci=%d, changing state to CONN_STATE_READY_FOR_CLIENT_PREFACE", i);
                                    G_connections[i].conn_state = CONN_STATE_READY_FOR_CLIENT_PREFACE;
                                }
                                else    /* header to send */
                                {
                                    if ( G_connections[i].http_ver[0] == '2' )
                                    {
                                        http2_add_frame(i, HTTP2_FRAME_TYPE_HEADERS);

                                        bytes = send(G_connections[i].fd, G_connections[i].http2_frame_start, G_connections[i].http2_bytes_to_send, 0);
                                    }
                                    else    /* HTTP/1 */
                                    {
#endif  /* NPP_HTTP2 */
                                        DDBG("Trying to write %u bytes to fd=%d (ci=%d)", G_connections[i].out_len, G_connections[i].fd, i);

                                        bytes = send(G_connections[i].fd, G_connections[i].out_start, G_connections[i].out_len, 0);
#ifdef NPP_HTTP2
                                    }
#endif
                                    set_state(i, bytes);    /* possibly:    CONN_STATE_DISCONNECTED (if error or closed by peer) */
#ifdef NPP_HTTP2
                                }
#endif  /* NPP_HTTP2 */
                            }
                            else if ( G_connections[i].conn_state == CONN_STATE_SENDING_CONTENT )
                            {
                                DDBG("ci=%d, state == CONN_STATE_SENDING_CONTENT", i);
#ifdef NPP_HTTP2
                                if ( G_connections[i].http_ver[0] == '2' )
                                {
                                    http2_add_frame(i, HTTP2_FRAME_TYPE_DATA);

                                    bytes = send(G_connections[i].fd, G_connections[i].http2_frame_start, G_connections[i].http2_bytes_to_send, 0);
                                }
                                else
                                {
#endif  /* NPP_HTTP2 */
                                    DDBG("Trying to write %u bytes to fd=%d (ci=%d)", G_connections[i].out_len-G_connections[i].data_sent, G_connections[i].fd, i);

                                    bytes = send(G_connections[i].fd, G_connections[i].out_start+G_connections[i].data_sent, G_connections[i].out_len-G_connections[i].data_sent, 0);
#ifdef NPP_HTTP2
                                }
#endif
                                set_state(i, bytes);    /* possibly:    CONN_STATE_DISCONNECTED (if error or closed by peer or !keep_alive) */
                                                        /*              CONN_STATE_SENDING_CONTENT (if data_sent < clen) */
                                                        /*              CONN_STATE_CONNECTED */
                            }
                        }

                        sockets_ready--;
                    }
                    else    /* not IN nor OUT */
                    {
#ifdef NPP_DEBUG
                        DBG("Not IN nor OUT, ci=%d, fd=%d conn_state = %c", i, G_connections[i].fd, G_connections[i].conn_state);
#ifdef NPP_FD_MON_POLL
                        DBG("revents=%d", M_pollfds[pi].revents);
#endif
#endif  /* NPP_DEBUG */

#ifdef NPP_APP_EVERY_SPARE_SECOND
                        npp_app_every_spare_second();
#endif
                    }

                    /* --------------------------------------------------------------------------------------- */
                    /* after reading / writing it may be ready for parsing and processing ... */

                    if ( G_connections[i].conn_state == CONN_STATE_READY_FOR_PARSE )
                    {
                        G_connections[i].status = parse_req(i, bytes);

                        if ( G_connections[i].conn_state != CONN_STATE_READING_DATA )
                        {
                            DDBG("ci=%d, changing state to CONN_STATE_READY_FOR_PROCESS", i);
                            G_connections[i].conn_state = CONN_STATE_READY_FOR_PROCESS;
                        }
                    }

                    /* received Expect: 100-continue before content */

                    if ( G_connections[i].expect100 )
                        respond_to_expect(i);

                    /* ready for processing */

                    if ( G_connections[i].conn_state == CONN_STATE_READY_FOR_PROCESS )
                    {
#ifdef _WIN32
                        clock_gettime_win(&G_connections[i].proc_start);
#else
                        clock_gettime(MONOTONIC_CLOCK_NAME, &G_connections[i].proc_start);
#endif
                        /* update visits counter */
                        if ( !G_connections[i].resource[0] && G_connections[i].status==200 && !NPP_CONN_IS_BOT(G_connections[i].flags) && !G_connections[i].method[0]=='H' && 0==strcmp(G_connections[i].host, NPP_APP_DOMAIN) )
                        {
                            ++G_cnts_today.visits;

                            if ( G_connections[i].ua_type == NPP_UA_TYPE_MOB )
                                ++G_cnts_today.visits_mob;
                            if ( G_connections[i].ua_type == NPP_UA_TYPE_TAB )
                                ++G_cnts_today.visits_tab;
                            else
                                ++G_cnts_today.visits_dsk;
                        }
#ifdef NPP_DEBUG
                        if ( G_connections[i].static_res != NPP_NOT_STATIC )
                            DBG("ci=%d, static resource", i);
#endif
                        if ( !G_connections[i].location[0] && G_connections[i].static_res == NPP_NOT_STATIC )   /* process request */
                            process_req(i);
#ifdef NPP_ASYNC
                        if ( G_connections[i].conn_state != CONN_STATE_WAITING_FOR_ASYNC )
#endif
                        gen_response_header(i);
                    }
                }   /* for on active sockets */
            }   /* some of the existing connections are ready to read/write */
        }   /* some of the sockets are ready to read/write */

#ifdef NPP_DEBUG
        if ( sockets_ready != 0 )
        {
            static time_t last_time=0;   /* prevent log overflow */

            if ( last_time != G_now )
            {
                DBG_T("sockets_ready should be 0 but currently %d", sockets_ready);
                last_time = G_now;
            }
        }
#endif  /* NPP_DEBUG */

        /* async processing -- check on response queue */
#ifdef NPP_ASYNC
        async_res_t res;
#ifdef NPP_DEBUG
        int mq_ret;
        if ( (mq_ret=mq_receive(G_queue_res, (char*)&res, ASYNC_RES_MSG_SIZE, NULL)) != -1 )    /* there's a response in the queue */
#else
        if ( mq_receive(G_queue_res, (char*)&res, ASYNC_RES_MSG_SIZE, NULL) != -1 )    /* there's a response in the queue */
#endif  /* NPP_DEBUG */
        {
#ifdef NPP_DEBUG
            DBG("res.chunk=%d", res.chunk);
            DBG("(unsigned short)res.chunk=%hd", (unsigned short)res.chunk);
#endif  /* NPP_DEBUG */

            unsigned chunk_num = 0;
            chunk_num |= (unsigned short)res.chunk;

            DBG("ASYNC response received, chunk=%u", chunk_num);

            int  res_ai;
            int  res_ci;
            int  res_len;
            char *res_data;

            if ( ASYNC_CHUNK_IS_FIRST(res.chunk) )  /* get all the response's details */
            {
                DBG("ASYNC_CHUNK_IS_FIRST");
                DBG("res.ci=%d", res.ci);
                DBG("res.hdr.err_code = %d", res.hdr.err_code);
                DBG("res.hdr.status = %d", res.hdr.status);
                DBG("res.len = %d", res.len);

                /* error code & status */

                G_connections[res.ci].async_err_code = res.hdr.err_code;
                G_connections[res.ci].status = res.hdr.status;

                /* update user session */

                if ( G_connections[res.ci].si )   /* session had existed before CALL_ASYNC */
                {
                    memcpy(&G_sessions[G_connections[res.ci].si], &res.hdr.eng_session_data, sizeof(eng_session_data_t));
#ifdef NPP_ASYNC_INCLUDE_SESSION_DATA
                    memcpy(&G_app_session_data[G_connections[res.ci].si], &res.hdr.app_session_data, sizeof(app_session_data_t));
#endif
                }
                else if ( res.hdr.eng_session_data.sesid[0] )   /* session has been started in npp_svc */
                {
                    DBG("New session has been started in npp_svc, adding to G_sessions");

                    if ( npp_eng_session_start(res.ci, res.hdr.eng_session_data.sesid) != OK )
                    {
                        ERR("Couldn't start a session after npp_svc had started it. Your memory model may be too low.");
                    }
                    else    /* OK, copy the session details from the response */
                    {
                        memcpy(&G_sessions[G_connections[res.ci].si], &res.hdr.eng_session_data, sizeof(eng_session_data_t));
#ifdef NPP_ASYNC_INCLUDE_SESSION_DATA
                        memcpy(&G_app_session_data[G_connections[res.ci].si], &res.hdr.app_session_data, sizeof(app_session_data_t));
#endif
                    }
                }

                /* password change or user deleted */

                if ( res.hdr.invalidate_uid )
                {
                    npp_eng_session_downgrade_by_uid(res.hdr.invalidate_uid, res.hdr.invalidate_ci);
                }

                /* update connection details */

                strcpy(G_connections[res.ci].cust_headers, res.hdr.cust_headers);
                G_connections[res.ci].cust_headers_len = res.hdr.cust_headers_len;
                G_connections[res.ci].ctype = res.hdr.ctype;
                strcpy(G_connections[res.ci].ctypestr, res.hdr.ctypestr);
                strcpy(G_connections[res.ci].cdisp, res.hdr.cdisp);
                strcpy(G_connections[res.ci].cookie_out_a, res.hdr.cookie_out_a);
                strcpy(G_connections[res.ci].cookie_out_a_exp, res.hdr.cookie_out_a_exp);
                strcpy(G_connections[res.ci].cookie_out_l, res.hdr.cookie_out_l);
                strcpy(G_connections[res.ci].cookie_out_l_exp, res.hdr.cookie_out_l_exp);
                strcpy(G_connections[res.ci].location, res.hdr.location);
//                G_connections[res.ci].dont_cache = res.hdr.dont_cache;
//                G_connections[res.ci].keep_content = res.hdr.keep_content;

                G_connections[res.ci].flags = res.hdr.flags;

                /* update HTTP calls stats */

                if ( res.hdr.call_http_req_cnt > 0 )
                {
                    G_call_http_status = res.hdr.call_http_status;
                    G_call_http_req_cnt += res.hdr.call_http_req_cnt;
                    G_call_http_elapsed += res.hdr.call_http_elapsed;
                    G_call_http_average = G_call_http_elapsed / G_call_http_req_cnt;
                }

                res_ai = res.ai;
                res_ci = res.ci;
                res_len = res.len;
                res_data = res.data;
            }
            else    /* 'data' chunk */
            {
                DBG("'data' chunk");

                async_res_data_t *resd = (async_res_data_t*)&res;

                res_ai = resd->ai;
                res_ci = resd->ci;
                res_len = resd->len;
                res_data = resd->data;
            }

            /* out data */

            if ( res_len > 0 )    /* chunk length */
            {
                DBG("res_len = %d", res_len);
#ifdef NPP_OUT_CHECK_REALLOC
                npp_eng_out_check_realloc_bin(res_ci, res_data, res_len);
#else
                unsigned checked_len = res_len > NPP_OUT_BUFSIZE-NPP_OUT_HEADER_BUFSIZE ? NPP_OUT_BUFSIZE-NPP_OUT_HEADER_BUFSIZE : res_len;
                memcpy(G_connections[res_ci].p_content, res_data, checked_len);
                G_connections[res_ci].p_content += checked_len;
#endif
            }

            if ( ASYNC_CHUNK_IS_LAST(res.chunk) )
            {
                DBG("ASYNC_CHUNK_IS_LAST");

                M_areqs[res_ai].state = NPP_ASYNC_STATE_FREE;

                if ( G_connections[res_ci].location[0] )
                    G_connections[res_ci].status = 303;

                gen_response_header(res_ci);
            }
        }
#ifdef NPP_DEBUG
        else
        {
            static time_t last_time=0;   /* prevent log overflow */

            if ( last_time != G_now )
            {
                int wtf = errno;
                if ( wtf != EAGAIN )
                    ERR("mq_receive failed, errno = %d (%s)", wtf, strerror(wtf));
                last_time = G_now;
            }
        }
#endif  /* NPP_DEBUG */

#endif  /* NPP_ASYNC */

        /* under heavy load there might never be that sockets_ready==0 */
        /* make sure it runs at least every 10 seconds */
#ifdef NPP_DEBUG
//        DBG("M_last_housekeeping = %ld", M_last_housekeeping);
//        DBG("              G_now = %ld", G_now);
#endif
        if ( M_last_housekeeping < G_now-10 )
        {
            INF_T("M_last_housekeeping < G_now-10 ==> run housekeeping");
            if ( !housekeeping() )
                return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}


/* --------------------------------------------------------------------------
   Set values for Expires response headers
-------------------------------------------------------------------------- */
static void set_expiry_dates()
{
    strcpy(M_expires_stat, time_epoch2http(G_now + 3600*24*NPP_EXPIRES_STATICS));
    DBG("New M_expires_stat: %s", M_expires_stat);

    strcpy(M_expires_gen, time_epoch2http(G_now + 3600*24*NPP_EXPIRES_GENERATED));
    DBG("New M_expires_gen: %s", M_expires_gen);
}


/* --------------------------------------------------------------------------
   Close expired sessions etc...
-------------------------------------------------------------------------- */
static bool housekeeping()
{
//    DDBG("housekeeping");

    /* close expired connections */
    if ( G_connections_cnt ) close_old_conn();

    /* close expired anonymous user sessions */
    if ( G_sessions_cnt ) uses_close_timeouted();

#ifdef NPP_DEBUG
#ifndef NPP_DONT_RESCAN_RES
    if ( G_test )   /* kind of developer mode */
    {
        read_resources(FALSE);
    }
#endif  /* NPP_DONT_RESCAN_RES */
#endif  /* NPP_DEBUG */

    if ( G_ptm->tm_min != M_prev_minute )
    {
        DDBG("Once a minute");

        /* close expired authenticated sessions */
#ifdef NPP_USERS
        if ( G_sessions_cnt ) libusr_luses_close_timeouted();
#endif
        /* say something sometimes ... */
        ALWAYS_T("%d open connection(s) | %d session(s)", G_connections_cnt, G_sessions_cnt);

        log_flush();

#ifndef NPP_DONT_RESCAN_RES    /* refresh static resources */
        read_resources(FALSE);
#endif

        /* start new log file every day */

        if ( G_ptm->tm_mday != M_prev_day )
        {
            DDBG("Once a day");

            dump_counters();
            log_finish();
            if ( !log_start("", G_test) )
            {
                clean_up();
                return FALSE;
            }

            set_expiry_dates();

            if ( G_blockedIPList[0] )
                read_blocked_ips();

            if ( G_whiteList[0] )
                read_allowed_ips();

            /* copy & reset counters */
            memcpy(&G_cnts_day_before, &G_cnts_yesterday, sizeof(npp_counters_t));
            memcpy(&G_cnts_yesterday, &G_cnts_today, sizeof(npp_counters_t));
            memset(&G_cnts_today, 0, sizeof(npp_counters_t));
            G_call_http_req_cnt = 0;
            G_call_http_elapsed = 0;
            G_call_http_average = 0;

            /* log currently used memory */
            npp_log_memory();
            ++G_days_up;

            npp_lib_init_random_numbers();

            M_prev_day = G_ptm->tm_mday;
        }

        M_prev_minute = G_ptm->tm_min;
    }

    M_last_housekeeping = G_now;

    return TRUE;
}


#ifdef NPP_HTTP2
/* --------------------------------------------------------------------------
   Verify HTTP/2 client preface
   Finish protocol upgrade
-------------------------------------------------------------------------- */
static void http2_check_client_preface(int ci)
{
    DDBG("http2_check_client_preface ci=%d", ci);

    if ( strcmp(G_connections[ci].in, HTTP2_CLIENT_PREFACE) == 0 )
    {
        DBG("HTTP/2 client preface OK");

#ifdef NPP_DEBUG
        DBG_LINE;
        DBG("ci=%d, Changing HTTP version to 2", ci);
        DBG_LINE;
#endif  /* NPP_DEBUG */

        G_connections[ci].http_ver[0] = '2';
        G_connections[ci].http_ver[1] = EOS;

        /* at this point upgrade process should be over */
        /* we can now start serving normal HTTP/2 response frames */

        G_connections[ci].status = 200;

        G_connections[ci].http2_upgrade_in_progress = FALSE;
//        G_connections[ci].http2_last_stream_id = 1;

        /* generate response header again */

        gen_response_header(ci);
    }
    else
    {
        WAR("Invalid HTTP/2 client preface");
        DBG("End of processing, close_conn\n");
        close_conn(ci);
    }
}


/* --------------------------------------------------------------------------
   Get HTTP/2 frame type
-------------------------------------------------------------------------- */
static char *http2_get_frame_type(unsigned char type)
{
static char desc[64];

    if ( type == HTTP2_FRAME_TYPE_DATA )
        strcpy(desc, "HTTP2_FRAME_TYPE_DATA");
    else if ( type == HTTP2_FRAME_TYPE_HEADERS )
        strcpy(desc, "HTTP2_FRAME_TYPE_HEADERS");
    else if ( type == HTTP2_FRAME_TYPE_PRIORITY )
        strcpy(desc, "HTTP2_FRAME_TYPE_PRIORITY");
    else if ( type == HTTP2_FRAME_TYPE_RST_STREAM )
        strcpy(desc, "HTTP2_FRAME_TYPE_RST_STREAM");
    else if ( type == HTTP2_FRAME_TYPE_SETTINGS )
        strcpy(desc, "HTTP2_FRAME_TYPE_SETTINGS");
    else if ( type == HTTP2_FRAME_TYPE_PUSH_PROMISE )
        strcpy(desc, "HTTP2_FRAME_TYPE_PUSH_PROMISE");
    else if ( type == HTTP2_FRAME_TYPE_PING )
        strcpy(desc, "HTTP2_FRAME_TYPE_PING");
    else if ( type == HTTP2_FRAME_TYPE_GOAWAY )
        strcpy(desc, "HTTP2_FRAME_TYPE_GOAWAY");
    else if ( type == HTTP2_FRAME_TYPE_WINDOW_UPDATE )
        strcpy(desc, "HTTP2_FRAME_TYPE_WINDOW_UPDATE");
    else if ( type == HTTP2_FRAME_TYPE_CONTINUATION )
        strcpy(desc, "HTTP2_FRAME_TYPE_CONTINUATION");
    else
        strcpy(desc, "Unknown frame type");

    return desc;
}


/* --------------------------------------------------------------------------
   Convert NBO 24-bit integer to machine int
-------------------------------------------------------------------------- */
int http2_24nbo_2_machine(const char *nbo_number)
{
    int number;

    memcpy((char*)&number, nbo_number, 3);
    number >> 8;

    if ( G_endianness == ENDIANNESS_LITTLE )
    {
        int32_t tmp = number;
        number = bswap32(tmp);
    }

    DDBG("http2_24nbo_2_machine result = %d", number);

    return number;
}


/* --------------------------------------------------------------------------
   Parse HTTP/2 incoming frame
   bytes has to be > 0
-------------------------------------------------------------------------- */
static void http2_parse_frame(int ci, int bytes)
{
    DDBG("http2_parse_frame ci=%d, bytes=%d", ci, bytes);

//    http2_frame_hdr_t hdr;
    char hdr[HTTP2_FRAME_HDR_LEN]={0};

    memcpy(&hdr, &G_connections[ci].in, HTTP2_FRAME_HDR_LEN);

    int32_t length=0;

    /* we're in the network byte order ------------- */

    memcpy((char*)&length, (char*)&hdr, 3);
    length >> 8;

    /* --------------------------------------------- */

    if ( G_endianness == ENDIANNESS_LITTLE )
    {
        int32_t tmp = length;
        length = bswap32(tmp);
        G_connections[ci].http2_last_stream_id = bswap32((int32_t)*(hdr+5));
    }
    else    /* Big Endian */
    {
        G_connections[ci].http2_last_stream_id = (int32_t)*(hdr+5);
    }

#ifdef NPP_DEBUG
    DBG("IN frame length = %d", length);
    DBG("IN frame type = %s", http2_get_frame_type(hdr[3]));
    DBG("IN frame flags = 0x%02x", hdr[4]);
    DBG("IN frame stream_id = %d", G_connections[ci].http2_last_stream_id);
#endif  /* NPP_DEBUG */
}


/* --------------------------------------------------------------------------
   Create HTTP/2 outgoing frame
-------------------------------------------------------------------------- */
static void http2_add_frame(int ci, unsigned char type)
{
    DDBG("http2_add_frame ci=%d, type [%s]", ci, http2_get_frame_type(type));

    /* frame data */
    /* we're trying to avoid unnecessary copying */
    /* so we don't move the actual data, only headers */

    /* ------------------------------------------------ */
    /* calculate frame payload length */

    int32_t frame_pld_len;

    if ( type == HTTP2_FRAME_TYPE_DATA )
    {
        frame_pld_len = G_connections[ci].out_len - G_connections[ci].out_hlen;
    }
    else if ( type == HTTP2_FRAME_TYPE_HEADERS )
    {
        frame_pld_len = G_connections[ci].out_hlen;
        frame_pld_len += HTTP2_HEADERS_PLD_LEN;
    }
    else if ( type == HTTP2_FRAME_TYPE_SETTINGS )
    {
        frame_pld_len = 0;

        /* non-empty settings frame test ---------- */

/*        http2_SETTINGS_pld_t s={0};

        s.id = HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS;
        s.value = 100;

        memcpy(G_connections[ci].out_data, &s, HTTP2_SETTINGS_PAIR_LEN);

        frame_pld_len = HTTP2_SETTINGS_PAIR_LEN; */

        /* ---------------------------------------- */
    }

    DDBG("frame_pld_len = %d", frame_pld_len);

    /* ------------------------------------------------ */
    /* build frame header */

//    http2_frame_hdr_t hdr={0};
    char hdr[HTTP2_FRAME_HDR_LEN]={0};

    /* length */

    if ( G_endianness == ENDIANNESS_LITTLE )
    {
        int32_t tmp = bswap32(frame_pld_len);
        tmp << 8;
//        memcpy((char*)&hdr.length, (char*)&tmp, 3);
        memcpy(&hdr, (char*)&tmp, 3);
    }
    else    /* Big Endian */
    {
        int32_t tmp = frame_pld_len;
        tmp << 8;
//        memcpy((char*)&hdr.length, (char*)&tmp, 3);
        memcpy(&hdr, (char*)&tmp, 3);
    }

#ifdef NPP_DEBUG
    int test_len = http2_24nbo_2_machine((char*)&hdr);
#endif

    /* type */

//    hdr.type = type;
    hdr[3] = type;

    /* stream id */

    if ( type != HTTP2_FRAME_TYPE_SETTINGS )
    {
/*        if ( G_endianness == ENDIANNESS_LITTLE )
        {
            int32_t tmp = bswap32(G_connections[ci].http2_last_stream_id);
            memcpy(hdr+5, &tmp, 4);
        }
        else     Big Endian */
//            memcpy(hdr+5, &G_connections[ci].http2_last_stream_id, 4);

        int32_t tmp = htonl(G_connections[ci].http2_last_stream_id);
        memcpy(hdr+5, &tmp, 4);
    }

    /* ------------------------------------------------ */
    /* set frame starting point */

    /* beware! */
//    DDBG("        sizeof(hdr) = %d", sizeof(hdr));   /* 3 extra bytes of padding! */
//    DDBG("HTTP2_FRAME_HDR_LEN = %d", HTTP2_FRAME_HDR_LEN);

    if ( type == HTTP2_FRAME_TYPE_DATA )
    {
        G_connections[ci].http2_frame_start = G_connections[ci].out_data - HTTP2_FRAME_HDR_LEN;

//        hdr.flags |= HTTP2_FRAME_FLAG_END_STREAM;
        hdr[4] |= HTTP2_FRAME_FLAG_END_STREAM;
    }
    else if ( type == HTTP2_FRAME_TYPE_HEADERS )
    {
        G_connections[ci].http2_frame_start = G_connections[ci].out_start - HTTP2_FRAME_HDR_LEN - HTTP2_HEADERS_PLD_LEN;

        http2_HEADERS_pld_t HEADERS_pld={0};

//        HEADERS_pld.weight = 1;

        memcpy(G_connections[ci].http2_frame_start+HTTP2_FRAME_HDR_LEN, (char*)&HEADERS_pld, HTTP2_HEADERS_PLD_LEN);

//        hdr.flags |= HTTP2_FRAME_FLAG_END_HEADERS;
        hdr[4] |= HTTP2_FRAME_FLAG_END_HEADERS;
    }
    else if ( type == HTTP2_FRAME_TYPE_SETTINGS )
    {
//        hdr[4] |= HTTP2_FRAME_FLAG_ACK;   // temp!

        G_connections[ci].http2_frame_start = G_connections[ci].out_data - HTTP2_FRAME_HDR_LEN;
    }

    G_connections[ci].http2_bytes_to_send = HTTP2_FRAME_HDR_LEN + frame_pld_len;

    DDBG("http2_bytes_to_send = %u", G_connections[ci].http2_bytes_to_send);

    /* ------------------------------------------------ */
    /* copy frame header before the frame data */

    memcpy(G_connections[ci].http2_frame_start, (char*)&hdr, HTTP2_FRAME_HDR_LEN);

    /* ------------------------------------------------ */

#ifdef NPP_DEBUG
    DBG("OUT frame pld length = %d", frame_pld_len);
    DBG("OUT frame type = %s", http2_get_frame_type(type));
    DBG("OUT frame flags = 0x%02x", hdr[4]);

    if ( type != HTTP2_FRAME_TYPE_SETTINGS )
        DBG("OUT frame stream_id = %d", G_connections[ci].http2_last_stream_id);
    else
        DBG("OUT frame stream_id = 0");
#endif  /* NPP_DEBUG */
}


static int http2_single_octet(unsigned val)
{
    return val < (1 << HTTP2_INT_PREFIX_BITS) - 1;
}


/* --------------------------------------------------------------------------
   Encode an integer for HTTP/2
   Return number of bytes encoded value took
-------------------------------------------------------------------------- */
static int http2_encode_int(char *enc_val, int val)
{
/*    char buf[HTTP2_MAX_ENC_INT_LEN];
    char *p = buf + HTTP2_MAX_ENC_INT_LEN;

    do
    {
        *--p = '0' + val % 10;
    } while ((val /= 10) != 0);

    int len = buf + sizeof(buf) - p;

    DDBG("http2_encode_int: val = %u, len = %d", val, len);

    *enc_val++ = 0x0f;
    *enc_val++ = 0x0d;
    *enc_val++ = (uint8_t)len;

    memcpy(enc_val, p, len); */



//    DDBG("http2_encode_int: val = %d", val);



    int len=1;

    if ( http2_single_octet(val) )
    {
        *enc_val++ |= val;
    }
    else
    {
        val -= (1 << 3) - 1;
        *enc_val++ |= (1 << 3) - 1;

        for ( ; val>=128; val>>=7 )
        {
            *enc_val++ = 0x80 | val;
            ++len;
        }

        *enc_val++ = val;
    }

//    DDBG("http2_encode_int: len = %d", len);

    return len;
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_status(int ci, int status)
{
    DDBG("http2_hdr_status");

    G_connections[ci].p_header += HTTP2_HEADERS_PLD_LEN;

    if ( status == 200 )
        *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_STATUS_200);
    else if ( status == 400 )
        *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_STATUS_400);
    else if ( status == 404 )
        *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_STATUS_404);
    else if ( status == 500 )
        *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_STATUS_500);
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_date(int ci)
{
    DDBG("http2_hdr_date");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_DATE);
    *G_connections[ci].p_header++ = (char)strlen(M_resp_date);
    HOUT(M_resp_date);
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_location(int ci)
{
    DDBG("http2_hdr_location");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_LOCATION);
    *G_connections[ci].p_header++ = (char)strlen(G_connections[ci].location);
    HOUT(G_connections[ci].location);
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_content_type(int ci, const char *val)
{
    DDBG("http2_hdr_content_type");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_CONTENT_TYPE);
    *G_connections[ci].p_header++ = (char)strlen(val);
    HOUT(val);
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_content_disp(int ci, const char *val)
{
    DDBG("http2_hdr_content_disp");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_CONTENT_DISPOSITION);
    *G_connections[ci].p_header++ = (char)strlen(val);
    HOUT(val);
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_content_len(int ci, unsigned val)
{
    DDBG("http2_hdr_content_len, val = %u", val);

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_CONTENT_LENGTH);

//    char enc_val[HTTP2_MAX_ENC_INT_LEN];
//    int bytes = http2_encode_int(enc_val, val);
//    HOUT_BIN(enc_val, bytes);

    int32_t enc_val_int = htonl(val);
    HOUT_BIN(&enc_val_int, 4);
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_cache_ctrl_public(int ci)
{
    DDBG("http2_hdr_cache_ctrl_public");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_CACHE_CONTROL);
    *G_connections[ci].p_header++ = (char)24;
    HOUT("public, max-age=31536000");
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_cache_ctrl_private(int ci)
{
    DDBG("http2_hdr_cache_ctrl_private");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_CACHE_CONTROL);
    *G_connections[ci].p_header++ = (char)55;
    HOUT("private, must-revalidate, no-store, no-cache, max-age=0");
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_expires_statics(int ci)
{
    DDBG("http2_hdr_expires_statics");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_EXPIRES);
    *G_connections[ci].p_header++ = (char)strlen(M_expires_stat);
    HOUT(M_expires_stat);
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_expires_gen(int ci)
{
/*    DDBG("http2_hdr_expires_gen");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_EXPIRES);
    *G_connections[ci].p_header++ = (char)strlen(M_expires_gen);
    HOUT(M_expires_gen); */
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_last_modified(int ci, const char *val)
{
    DDBG("http2_hdr_last_modified");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_LAST_MODIFIED);
    *G_connections[ci].p_header++ = (char)strlen(val);
    HOUT(val);
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_vary(int ci, const char *val)
{
/*    DDBG("http2_hdr_vary");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_VARY);
    *G_connections[ci].p_header++ = (char)strlen(val);
    HOUT(val); */
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_content_lang(int ci, const char *val)
{
    DDBG("http2_hdr_content_lang");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_CONTENT_LANGUAGE);
    *G_connections[ci].p_header++ = (char)strlen(val);
    HOUT(val);
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_content_enc_deflate(int ci)
{
    DDBG("http2_hdr_content_enc_deflate");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_CONTENT_ENCODING);
    *G_connections[ci].p_header++ = (char)7;
    HOUT("deflate");
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_set_cookie(int ci, const char *val)
{
    DDBG("http2_hdr_set_cookie");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_SET_COOKIE);
    *G_connections[ci].p_header++ = (char)strlen(val);
    HOUT(val);
}


/* --------------------------------------------------------------------------
   Add HTTP/2 header
-------------------------------------------------------------------------- */
static void http2_hdr_server(int ci)
{
/*    DDBG("http2_hdr_server");

    *G_connections[ci].p_header++ = (0x80 | HTTP2_HDR_SERVER);
    *G_connections[ci].p_header++ = (char)6;
    HOUT("Node++"); */
}
#endif  /* NPP_HTTP2 */


/* --------------------------------------------------------------------------
   Set new connection state after read or write
-------------------------------------------------------------------------- */
static void set_state(int ci, int bytes)
{
    DDBG("set_state ci=%d, bytes=%d", ci, bytes);

    if ( bytes <= 0 )
    {
        DBG("bytes = %d, errno = %d (%s), disconnecting slot %d\n", bytes, errno, strerror(errno), ci);
        close_conn(ci);
        return;
    }

    /* bytes > 0 */

    if ( G_connections[ci].conn_state == CONN_STATE_CONNECTED )  /* assume the whole header has been read */
    {
        DDBG("ci=%d, changing state to CONN_STATE_READY_FOR_PARSE", ci);
        G_connections[ci].conn_state = CONN_STATE_READY_FOR_PARSE;
    }
    else if ( G_connections[ci].conn_state == CONN_STATE_READING_DATA )  /* it could have been received only partially */
    {
        if ( G_connections[ci].was_read < G_connections[ci].clen )
        {
            DBG("ci=%d, was_read=%u, continue receiving", ci, G_connections[ci].was_read);
        }
        else    /* data received */
        {
            G_connections[ci].in_data[G_connections[ci].was_read] = EOS;

            DBG("POST data received");

            /* ready for processing */

            DDBG("ci=%d, changing state to CONN_STATE_READY_FOR_PROCESS", ci);
            G_connections[ci].conn_state = CONN_STATE_READY_FOR_PROCESS;
        }
    }
    else if ( G_connections[ci].conn_state == CONN_STATE_READY_TO_SEND_RESPONSE )
    {
        if ( G_connections[ci].clen == 0 )   /* no content to send */
        {
            DBG("clen = 0");

            /* assume the whole header has been sent */

            log_request(ci);
#ifdef NPP_HTTP2
            if ( NPP_CONN_IS_KEEP_ALIVE(G_connections[ci].flags) || G_connections[ci].http_ver[0] == '2' )
#else
            if ( NPP_CONN_IS_KEEP_ALIVE(G_connections[ci].flags) )
#endif
            {
                DBG("End of processing, reset_conn\n");
                reset_conn(ci, CONN_STATE_CONNECTED);
            }
            else
            {
                DBG("End of processing, close_conn\n");
                close_conn(ci);
            }
        }
        else    /* there was a content to send */
        {
            G_connections[ci].data_sent += bytes;

            DDBG("ci=%d, data_sent = %u", ci, G_connections[ci].data_sent);

#ifdef NPP_HTTP2
            if ( bytes < G_connections[ci].out_len && G_connections[ci].http_ver[0] != '2' )
#else
            if ( bytes < G_connections[ci].out_len )   /* not all have been sent yet */
#endif  /* NPP_HTTP2 */
            {
                DDBG("ci=%d, changing state to CONN_STATE_SENDING_CONTENT", ci);
                G_connections[ci].conn_state = CONN_STATE_SENDING_CONTENT;
            }
            else    /* the whole content has been sent at once */
            {
                log_request(ci);
#ifdef NPP_HTTP2
                if ( NPP_CONN_IS_KEEP_ALIVE(G_connections[ci].flags) || G_connections[ci].http_ver[0] == '2' )
#else
                if ( NPP_CONN_IS_KEEP_ALIVE(G_connections[ci].flags) )
#endif
                {
                    DBG("End of processing, reset_conn\n");
                    reset_conn(ci, CONN_STATE_CONNECTED);
                }
                else
                {
                    DBG("End of processing, close_conn\n");
                    close_conn(ci);
                }
            }
        }
    }
    else if ( G_connections[ci].conn_state == CONN_STATE_SENDING_CONTENT )
    {
        G_connections[ci].data_sent += bytes;

        if ( G_connections[ci].data_sent < G_connections[ci].out_len )
        {
            DBG("ci=%d, data_sent=%u, continue sending", ci, G_connections[ci].data_sent);
        }
        else    /* all sent */
        {
            log_request(ci);
#ifdef NPP_HTTP2
            if ( NPP_CONN_IS_KEEP_ALIVE(G_connections[ci].flags) || G_connections[ci].http_ver[0] == '2' )
#else
            if ( NPP_CONN_IS_KEEP_ALIVE(G_connections[ci].flags) )
#endif
            {
                DBG("End of processing, reset_conn\n");
                reset_conn(ci, CONN_STATE_CONNECTED);
            }
            else
            {
                DBG("End of processing, close_conn\n");
                close_conn(ci);
            }
        }
    }
}


/* --------------------------------------------------------------------------
   Set new connection state after read or write for secure connections
-------------------------------------------------------------------------- */
static void set_state_sec(int ci, int bytes)
{
    int     e;
    char    ec[256]="";
#ifdef NPP_HTTPS
    e = errno;

    DDBG("set_state_sec ci=%d, bytes=%d", ci, bytes);

    G_connections[ci].ssl_err = SSL_get_error(G_connections[ci].ssl, bytes);

    if ( bytes <= 0 )
    {
        if ( G_connections[ci].ssl_err == SSL_ERROR_SYSCALL )
            sprintf(ec, ", errno = %d (%s)", e, strerror(e));

        DBG("bytes = %d, ssl_err = %d%s", bytes, G_connections[ci].ssl_err, ec);

        if ( G_connections[ci].ssl_err != SSL_ERROR_WANT_READ && G_connections[ci].ssl_err != SSL_ERROR_WANT_WRITE )
        {
            DBG("Closing connection\n");
            close_conn(ci);
        }

#ifdef NPP_FD_MON_POLL
        if ( G_connections[ci].ssl_err == SSL_ERROR_WANT_READ )
            M_pollfds[G_connections[ci].pi].events = POLLIN;
        else if ( G_connections[ci].ssl_err == SSL_ERROR_WANT_WRITE )
            M_pollfds[G_connections[ci].pi].events = POLLOUT;
#endif
        return;
    }

    /* bytes > 0 */

    /* we have no way of knowing if accept finished before reading actual request */
    if ( G_connections[ci].conn_state == CONN_STATE_ACCEPTING || G_connections[ci].conn_state == CONN_STATE_CONNECTED )   /* assume the whole header has been read */
    {
        DDBG("Changing state to CONN_STATE_READY_FOR_PARSE");
        G_connections[ci].conn_state = CONN_STATE_READY_FOR_PARSE;
    }
    else if ( G_connections[ci].conn_state == CONN_STATE_READING_DATA )
    {
        if ( G_connections[ci].was_read < G_connections[ci].clen )
        {
            DBG("Continue receiving");
        }
        else    /* data received */
        {
            G_connections[ci].in_data[G_connections[ci].was_read] = EOS;

            DBG("POST data received");

            /* ready for processing */

            DDBG("Changing state to CONN_STATE_READY_FOR_PROCESS");
            G_connections[ci].conn_state = CONN_STATE_READY_FOR_PROCESS;
        }
    }
    else if ( G_connections[ci].conn_state == CONN_STATE_READY_TO_SEND_RESPONSE )
    {
        if ( G_connections[ci].clen == 0 )   /* no content to send */
        {
            DBG("clen = 0");

            /* assume the whole header has been sent */

            log_request(ci);

            if ( NPP_CONN_IS_KEEP_ALIVE(G_connections[ci].flags) )
            {
                DBG("End of processing, reset_conn\n");
                reset_conn(ci, CONN_STATE_CONNECTED);
            }
            else
            {
                DBG("End of processing, close_conn\n");
                close_conn(ci);
            }
        }
        else    /* there was a content to send */
        {
            G_connections[ci].data_sent += bytes;

            DDBG("ci=%d, data_sent = %u", ci, G_connections[ci].data_sent);

#ifdef NPP_HTTP2
            if ( bytes < G_connections[ci].out_len && G_connections[ci].http_ver[0] != '2' )
#else
            if ( bytes < G_connections[ci].out_len )   /* not all have been sent yet */
#endif  /* NPP_HTTP2 */
            {
                DDBG("ci=%d, changing state to CONN_STATE_SENDING_CONTENT", ci);
                G_connections[ci].conn_state = CONN_STATE_SENDING_CONTENT;
            }
            else    /* the whole content has been sent at once */
            {
                log_request(ci);
#ifdef NPP_HTTP2
                if ( NPP_CONN_IS_KEEP_ALIVE(G_connections[ci].flags) || G_connections[ci].http_ver[0] == '2' )
#else
                if ( NPP_CONN_IS_KEEP_ALIVE(G_connections[ci].flags) )
#endif
                {
                    DBG("End of processing, reset_conn\n");
                    reset_conn(ci, CONN_STATE_CONNECTED);
                }
                else
                {
                    DBG("End of processing, close_conn\n");
                    close_conn(ci);
                }
            }
        }
    }
    else if ( G_connections[ci].conn_state == CONN_STATE_SENDING_CONTENT )
    {
        G_connections[ci].data_sent += bytes;

        log_request(ci);

        if ( NPP_CONN_IS_KEEP_ALIVE(G_connections[ci].flags) )
        {
            DBG("End of processing, reset_conn\n");
            reset_conn(ci, CONN_STATE_CONNECTED);
        }
        else
        {
            DBG("End of processing, close_conn\n");
            close_conn(ci);
        }
    }
#endif  /* NPP_HTTPS */
}


/* --------------------------------------------------------------------------
   Read & parse conf file and set global parameters
-------------------------------------------------------------------------- */
static void read_conf()
{
    bool    conf_read=FALSE;

    /* set defaults */

    G_logLevel = 3;
    G_logToStdout = 0;
    G_logCombined = 0;
    G_httpPort = 80;
    G_httpsPort = 443;
    G_certFile[0] = EOS;
    G_certChainFile[0] = EOS;
    G_keyFile[0] = EOS;
    G_dbHost[0] = EOS;
    G_dbPort = 0;
    G_dbName[0] = EOS;
    G_dbUser[0] = EOS;
    G_dbPassword[0] = EOS;
    G_usersRequireActivation = 0;
    G_blockedIPList[0] = EOS;
    G_whiteList[0] = EOS;
    G_ASYNCId = -1;
    G_ASYNCDefTimeout = NPP_ASYNC_DEF_TIMEOUT;
    G_callHTTPTimeout = CALL_HTTP_DEFAULT_TIMEOUT;
    G_test = 0;

    /* get the conf file path & name */

    if ( G_appdir[0] )
    {
        char conf_path[1024];
        sprintf(conf_path, "%s/bin/npp.conf", G_appdir);
        if ( !(conf_read=npp_read_conf(conf_path)) )   /* no config file there */
            conf_read = npp_read_conf("npp.conf");
    }
    else    /* no NPP_DIR -- try current dir */
    {
        conf_read = npp_read_conf("npp.conf");
    }
    
    if ( conf_read )
    {
        npp_read_param_int("logLevel", &G_logLevel);
        npp_read_param_int("logToStdout", &G_logToStdout);
        npp_read_param_int("logCombined", &G_logCombined);
        npp_read_param_int("httpPort", &G_httpPort);
        npp_read_param_int("httpsPort", &G_httpsPort);
        npp_read_param_str("certFile", G_certFile);
        npp_read_param_str("certChainFile", G_certChainFile);
        npp_read_param_str("keyFile", G_keyFile);
        npp_read_param_str("dbHost", G_dbHost);
        npp_read_param_int("dbPort", &G_dbPort);
        npp_read_param_str("dbName", G_dbName);
        npp_read_param_str("dbUser", G_dbUser);
        npp_read_param_str("dbPassword", G_dbPassword);
        npp_read_param_int("usersRequireActivation", &G_usersRequireActivation);
        npp_read_param_str("blockedIPList", G_blockedIPList);
        npp_read_param_str("whiteList", G_whiteList);
        npp_read_param_int("ASYNCId", &G_ASYNCId);
        npp_read_param_int("ASYNCDefTimeout", &G_ASYNCDefTimeout);
        npp_read_param_int("callHTTPTimeout", &G_callHTTPTimeout);
        npp_read_param_int("test", &G_test);
    }
}


/* --------------------------------------------------------------------------
   Respond to Expect: header
-------------------------------------------------------------------------- */
static void respond_to_expect(int ci)
{
    static char reply_accept[]="HTTP/1.1 100 Continue\r\n\r\n";
    static char reply_refuse[]="HTTP/1.1 413 Request Entity Too Large\r\n\r\n";
    int bytes;

    if ( G_connections[ci].clen >= NPP_MAX_PAYLOAD_SIZE-1 )   /* refuse */
    {
        INF("Sending 413");
#ifdef NPP_HTTPS
        if ( NPP_CONN_IS_SECURE(G_connections[ci].flags) )
            bytes = SSL_write(G_connections[ci].ssl, reply_refuse, 41);
        else
#endif
            bytes = send(G_connections[ci].fd, reply_refuse, 41, 0);

        if ( bytes < 41 ) ERR("write error, bytes = %d", bytes);
    }
    else    /* accept */
    {
        INF("Sending 100");

#ifdef NPP_HTTPS
        if ( NPP_CONN_IS_SECURE(G_connections[ci].flags) )
            bytes = SSL_write(G_connections[ci].ssl, reply_accept, 25);
        else
#endif
            bytes = send(G_connections[ci].fd, reply_accept, 25, 0);

        if ( bytes < 25 ) ERR("write error, bytes = %d", bytes);
    }

    G_connections[ci].expect100 = FALSE;
}


/* --------------------------------------------------------------------------
   Log processing time
-------------------------------------------------------------------------- */
static void log_proc_time(int ci)
{
    G_connections[ci].elapsed = npp_elapsed(&G_connections[ci].proc_start);

    DBG("Processing time: %.3lf ms [%s]\n", G_connections[ci].elapsed, G_connections[ci].resource);

    G_cnts_today.elapsed += G_connections[ci].elapsed;
    G_cnts_today.average = G_cnts_today.elapsed / G_cnts_today.req;
}


/* --------------------------------------------------------------------------
   Log processing time
-------------------------------------------------------------------------- */
static void log_request(int ci)
{
    /* Use (almost) Combined Log Format */

    char logtime[64];
    strftime(logtime, 64, "%d/%b/%Y:%H:%M:%S +0000", G_ptm);

    if ( G_logCombined )
        INF("%s - - [%s] \"%s /%s HTTP/%s\" %d %u \"%s\" \"%s\"  #%u  %.3lf ms%s", G_connections[ci].ip, logtime, G_connections[ci].method, G_connections[ci].uri, G_connections[ci].http_ver, G_connections[ci].status, G_connections[ci].clen, G_connections[ci].referer, G_connections[ci].uagent, G_connections[ci].req, G_connections[ci].elapsed, REQ_BOT?"  [bot]":"");
    else
        INF("%s - - [%s] \"%s /%s HTTP/%s\" %d %u  #%u  %.3lf ms%s", G_connections[ci].ip, logtime, G_connections[ci].method, G_connections[ci].uri, G_connections[ci].http_ver, G_connections[ci].status, G_connections[ci].clen, G_connections[ci].req, G_connections[ci].elapsed, REQ_BOT?"  [bot]":"");
}


/* --------------------------------------------------------------------------
   Close connection
-------------------------------------------------------------------------- */
static void close_conn(int ci)
{
    DDBG("Closing connection ci=%d, fd=%d", ci, G_connections[ci].fd);

#ifdef NPP_HTTPS
    if ( G_connections[ci].ssl )
    {
        SSL_free(G_connections[ci].ssl);
        G_connections[ci].ssl = NULL;
    }
#endif  /* NPP_HTTPS */

#ifdef NPP_FD_MON_POLL  /* remove from monitored set */

    M_pollfds_cnt--;

    if ( G_connections[ci].pi != M_pollfds_cnt )    /* move the last one to just freed spot */
    {
        memcpy(&M_pollfds[G_connections[ci].pi], &M_pollfds[M_pollfds_cnt], sizeof(struct pollfd));
        /* update cross-references */
        M_poll_ci[G_connections[ci].pi] = M_poll_ci[M_pollfds_cnt];
        G_connections[M_poll_ci[M_pollfds_cnt]].pi = G_connections[ci].pi;
    }

#endif  /* NPP_FD_MON_POLL */

#ifdef _WIN32   /* Windows */
    closesocket(G_connections[ci].fd);
#else
    close(G_connections[ci].fd);
#endif  /* _WIN32 */
    reset_conn(ci, CONN_STATE_DISCONNECTED);

    G_connections_cnt--;
}


/* --------------------------------------------------------------------------
   Engine init
   Return TRUE if success
-------------------------------------------------------------------------- */
static bool init(int argc, char **argv)
{
    int i=0;

    /* init globals */

    G_days_up = 0;
    G_connections_cnt = 0;
    G_connections_hwm = 0;
    G_sessions_cnt = 0;
    G_sessions_hwm = 0;
    G_index_present = FALSE;
#ifdef NPP_MYSQL
    G_dbconn = NULL;
#endif

    /* counters */

    memset(&G_cnts_today, 0, sizeof(npp_counters_t));
    memset(&G_cnts_yesterday, 0, sizeof(npp_counters_t));
    memset(&G_cnts_day_before, 0, sizeof(npp_counters_t));

    /* init Node++ library */

    if ( !npp_lib_init() )
        return FALSE;

    /* read the config file or set defaults */

    read_conf();

    /* command line arguments */

    if ( argc > 1 )
    {
        G_httpPort = atoi(argv[1]);
        printf("Will be listening on the port %d...\n", G_httpPort);
    }

    /* start log --------------------------------------------------------- */

    char exec_name[256];
    npp_get_exec_name(exec_name, argv[0]);

    if ( !log_start("", G_test) )
        return FALSE;

    ALWAYS("Starting program");
    ALWAYS("");

#ifdef __linux__
    INF("This is Linux");
    INF("");
#endif

#ifdef _WIN32
    INF("This is Windows");
    INF("");
#endif

    ALWAYS("G_appdir [%s]", G_appdir);
    ALWAYS("logLevel = %d", G_logLevel);
    ALWAYS("logToStdout = %d", G_logToStdout);
    ALWAYS("logCombined = %d", G_logCombined);
    if ( argc > 1 )
    {
        ALWAYS("--------------------------------------------------------------------");
        WAR("httpPort = %d -- overwritten by a command line argument", G_httpPort);
        ALWAYS("--------------------------------------------------------------------");
    }
    else
        ALWAYS("httpPort = %d", G_httpPort);
    ALWAYS("httpsPort = %d", G_httpsPort);
    ALWAYS("dbHost [%s]", G_dbHost);
    ALWAYS("dbPort = %d", G_dbPort);
    ALWAYS("dbName [%s]", G_dbName);
    ALWAYS("ASYNCDefTimeout = %d", G_ASYNCDefTimeout);
    ALWAYS("callHTTPTimeout = %d", G_callHTTPTimeout);
    ALWAYS("test = %d", G_test);

    /* pid file ---------------------------------------------------------- */

    if ( !(M_pidfile=lib_create_pid_file("npp_app")) )
        return FALSE;

    /* empty static resources list */

    for ( i=0; i<NPP_MAX_STATICS; ++i )
        strcpy(M_stat[i].name, "-");

    /* check endianness and some parameters */

    ALWAYS("");
    ALWAYS_LINE_LONG;
    ALWAYS("");
    ALWAYS("System:");
    ALWAYS("-------");
#ifdef NPP_DEBUG
    // SIZE_MAX is not defined in older GCC!
//    ALWAYS("              SIZE_MAX = %lu (%lu KiB / %lu MiB)", SIZE_MAX, SIZE_MAX/1024, SIZE_MAX/1024/1024);
#endif
    npp_get_byteorder();

    ALWAYS("              FD_SETSIZE = %d", FD_SETSIZE);
    ALWAYS("               SOMAXCONN = %d", SOMAXCONN);
    ALWAYS("");
    ALWAYS("Server:");
    ALWAYS("-------");
    ALWAYS("                 NPP_DIR = %s", G_appdir);
    ALWAYS("             NPP_VERSION = %s", NPP_VERSION);
#ifdef NPP_MEM_TINY
    ALWAYS("            Memory model = NPP_MEM_TINY");
#elif defined NPP_MEM_MEDIUM
    ALWAYS("            Memory model = NPP_MEM_MEDIUM");
#elif defined NPP_MEM_LARGE
    ALWAYS("            Memory model = NPP_MEM_LARGE");
#elif defined NPP_MEM_XLARGE
    ALWAYS("            Memory model = NPP_MEM_XLARGE");
#elif defined NPP_MEM_XXLARGE
    ALWAYS("            Memory model = NPP_MEM_XXLARGE");
#elif defined NPP_MEM_XXXLARGE
    ALWAYS("            Memory model = NPP_MEM_XXXLARGE");
#elif defined NPP_MEM_XXXXLARGE
    ALWAYS("            Memory model = NPP_MEM_XXXXLARGE");
#else   /* NPP_MEM_SMALL -- default */
    ALWAYS("            Memory model = NPP_MEM_SMALL");
#endif
    ALWAYS("     NPP_MAX_CONNECTIONS = %d", NPP_MAX_CONNECTIONS);
    ALWAYS("        NPP_MAX_SESSIONS = %d", NPP_MAX_SESSIONS);
    ALWAYS("");
#ifdef NPP_FD_MON_SELECT
    ALWAYS("           FD monitoring = NPP_FD_MON_SELECT");
#endif
#ifdef NPP_FD_MON_POLL
    ALWAYS("           FD monitoring = NPP_FD_MON_POLL");
#endif
#ifdef NPP_FD_MON_EPOLL
    ALWAYS("           FD monitoring = NPP_FD_MON_EPOLL");
#endif
    ALWAYS("");
    ALWAYS("  NPP_CONNECTION_TIMEOUT = %d seconds", NPP_CONNECTION_TIMEOUT);
    ALWAYS("     NPP_SESSION_TIMEOUT = %d seconds", NPP_SESSION_TIMEOUT);
#ifdef NPP_USERS
    ALWAYS("NPP_AUTH_SESSION_TIMEOUT = %d seconds", NPP_AUTH_SESSION_TIMEOUT);
#endif
    ALWAYS("");
    ALWAYS("         NPP_OUT_BUFSIZE = %lu bytes (%lu KiB / %0.2lf MiB)", NPP_OUT_BUFSIZE, NPP_OUT_BUFSIZE/1024, (double)NPP_OUT_BUFSIZE/1024/1024);
    ALWAYS("");
    ALWAYS("     G_connections' size = %lu bytes (%lu KiB / %0.2lf MiB)", sizeof(G_connections), sizeof(G_connections)/1024, (double)sizeof(G_connections)/1024/1024);
    ALWAYS("        G_sessions' size = %lu bytes (%lu KiB / %0.2lf MiB)", sizeof(G_sessions), sizeof(G_sessions)/1024, (double)sizeof(G_sessions)/1024/1024);
    ALWAYS("G_app_session_data' size = %lu bytes (%lu KiB / %0.2lf MiB)", sizeof(G_app_session_data), sizeof(G_app_session_data)/1024, (double)sizeof(G_app_session_data)/1024/1024);
    ALWAYS("");
#ifdef NPP_ASYNC
    ALWAYS("      ASYNC_REQ_MSG_SIZE = %d bytes", ASYNC_REQ_MSG_SIZE);
    ALWAYS("      ASYNC req.hdr size = %lu bytes (%lu KiB / %0.2lf MiB)", sizeof(async_req_hdr_t), sizeof(async_req_hdr_t)/1024, (double)sizeof(async_req_hdr_t)/1024/1024);
    ALWAYS("   G_async_req_data_size = %lu bytes (%lu KiB / %0.2lf MiB)", G_async_req_data_size, G_async_req_data_size/1024, (double)G_async_req_data_size/1024/1024);
    ALWAYS("      ASYNC request size = %lu bytes (%lu KiB / %0.2lf MiB)", sizeof(async_req_t), sizeof(async_req_t)/1024, (double)sizeof(async_req_t)/1024/1024);
    ALWAYS("");
    ALWAYS("      ASYNC_RES_MSG_SIZE = %d bytes", ASYNC_RES_MSG_SIZE);
    ALWAYS("      ASYNC res.hdr size = %lu bytes (%lu KiB / %0.2lf MiB)", sizeof(async_res_hdr_t), sizeof(async_res_hdr_t)/1024, (double)sizeof(async_res_hdr_t)/1024/1024);
    ALWAYS("   G_async_res_data_size = %lu bytes (%lu KiB / %0.2lf MiB)", G_async_res_data_size, G_async_res_data_size/1024, (double)G_async_res_data_size/1024/1024);
    ALWAYS("     ASYNC response size = %lu bytes (%lu KiB / %0.2lf MiB)", sizeof(async_res_t), sizeof(async_res_t)/1024, (double)sizeof(async_res_t)/1024/1024);
    ALWAYS("");
#endif  /* NPP_ASYNC */
#ifdef NPP_OUT_FAST
    ALWAYS("             Output type = NPP_OUT_FAST");
#endif
#ifdef NPP_OUT_CHECK
    ALWAYS("             Output type = NPP_OUT_CHECK");
#endif
#ifdef NPP_OUT_CHECK_REALLOC
    ALWAYS("             Output type = NPP_OUT_CHECK_REALLOC");
#endif

#ifdef QS_DEF_SQL_ESCAPE
    ALWAYS("   Query string security = QS_DEF_SQL_ESCAPE");
#endif
#ifdef QS_DEF_DONT_ESCAPE
    ALWAYS("   Query string security = QS_DEF_DONT_ESCAPE");
#endif
#ifdef QS_DEF_HTML_ESCAPE
    ALWAYS("   Query string security = QS_DEF_HTML_ESCAPE");
#endif
    ALWAYS("");
    ALWAYS("Program:");
    ALWAYS("--------");
    ALWAYS("            NPP_APP_NAME = %s", NPP_APP_NAME);
    ALWAYS("          NPP_APP_DOMAIN = %s", NPP_APP_DOMAIN);
    ALWAYS("           NPP_LOGIN_URI = %s", NPP_LOGIN_URI);
    if ( NPP_REQUIRED_AUTH_LEVEL == AUTH_LEVEL_NONE )
        ALWAYS(" NPP_REQUIRED_AUTH_LEVEL = AUTH_LEVEL_NONE");
    else if ( NPP_REQUIRED_AUTH_LEVEL == AUTH_LEVEL_ANONYMOUS )
        ALWAYS(" NPP_REQUIRED_AUTH_LEVEL = AUTH_LEVEL_ANONYMOUS");
    else if ( NPP_REQUIRED_AUTH_LEVEL == AUTH_LEVEL_LOGGEDIN )
        ALWAYS(" NPP_REQUIRED_AUTH_LEVEL = AUTH_LEVEL_LOGGEDIN");
    else if ( NPP_REQUIRED_AUTH_LEVEL == AUTH_LEVEL_USER )
        ALWAYS(" NPP_REQUIRED_AUTH_LEVEL = AUTH_LEVEL_USER");
    else if ( NPP_REQUIRED_AUTH_LEVEL == AUTH_LEVEL_CUSTOMER )
        ALWAYS(" NPP_REQUIRED_AUTH_LEVEL = AUTH_LEVEL_CUSTOMER");
    else if ( NPP_REQUIRED_AUTH_LEVEL == AUTH_LEVEL_STAFF )
        ALWAYS(" NPP_REQUIRED_AUTH_LEVEL = AUTH_LEVEL_STAFF");
    else if ( NPP_REQUIRED_AUTH_LEVEL == AUTH_LEVEL_MODERATOR )
        ALWAYS(" NPP_REQUIRED_AUTH_LEVEL = AUTH_LEVEL_MODERATOR");
    else if ( NPP_REQUIRED_AUTH_LEVEL == AUTH_LEVEL_ADMIN )
        ALWAYS(" NPP_REQUIRED_AUTH_LEVEL = AUTH_LEVEL_ADMIN");
    else if ( NPP_REQUIRED_AUTH_LEVEL == AUTH_LEVEL_ROOT )
        ALWAYS(" NPP_REQUIRED_AUTH_LEVEL = AUTH_LEVEL_ROOT");

    ALWAYS("          NPP_SESSID_LEN = %d", NPP_SESSID_LEN);
    ALWAYS("         NPP_MAX_STATICS = %d", NPP_MAX_STATICS);
    ALWAYS("        NPP_MAX_MESSAGES = %d", NPP_MAX_MESSAGES);
    ALWAYS("         NPP_MAX_STRINGS = %d", NPP_MAX_STRINGS);
    ALWAYS("        NPP_MAX_SNIPPETS = %d", NPP_MAX_SNIPPETS);
    ALWAYS("     NPP_EXPIRES_STATICS = %d days", NPP_EXPIRES_STATICS);
    ALWAYS("   NPP_EXPIRES_GENERATED = %d days", NPP_EXPIRES_GENERATED);
#ifndef _WIN32
    ALWAYS("   NPP_COMPRESS_TRESHOLD = %d bytes", NPP_COMPRESS_TRESHOLD);
    ALWAYS("      NPP_COMPRESS_LEVEL = %d", NPP_COMPRESS_LEVEL);
#endif

#ifdef NPP_ADMIN_EMAIL
    ALWAYS("         NPP_ADMIN_EMAIL = %s", NPP_ADMIN_EMAIL);
#endif
#ifdef NPP_CONTACT_EMAIL
    ALWAYS("       NPP_CONTACT_EMAIL = %s", NPP_CONTACT_EMAIL);
#endif
#ifdef NPP_USERS
    ALWAYS("");
#ifdef NPP_USERS_BY_EMAIL
    ALWAYS("   Users' authentication = NPP_USERS_BY_EMAIL");
#else
    ALWAYS("   Users' authentication = NPP_USERS_BY_LOGIN");
#endif
    ALWAYS("");
#endif  /* NPP_USERS */
    ALWAYS_LINE_LONG;
    ALWAYS("");

#ifdef NPP_DEBUG
    WAR("NPP_DEBUG is enabled, this file may grow big quickly!");
    ALWAYS("");
#endif  /* NPP_DEBUG */

    /* USERS library init */

#ifdef NPP_USERS
    libusr_init();
#endif

    /* ensure the message sizes are sufficient */

#ifdef NPP_ASYNC
    if ( sizeof(async_req_hdr_t) > ASYNC_REQ_MSG_SIZE )
    {
        ERR("sizeof(async_req_hdr_t) > ASYNC_REQ_MSG_SIZE, increase APP_ASYNC_REQ_MSG_SIZE");
        return FALSE;
    }

    if ( sizeof(async_res_hdr_t) > ASYNC_RES_MSG_SIZE )
    {
        ERR("sizeof(async_res_hdr_t) > ASYNC_RES_MSG_SIZE, increase APP_ASYNC_RES_MSG_SIZE");
        return FALSE;
    }
#endif  /* NPP_ASYNC */


#ifdef NPP_MYSQL

    DBG("Trying npp_open_db...");

    if ( !npp_open_db() )
    {
        ERR("npp_open_db failed");
        return FALSE;
    }

    ALWAYS("Database connected");

#endif  /* NPP_MYSQL */


    /* custom init
       Among others, that may contain generating statics, like css and js */

    if ( !npp_app_init(argc, argv) )
    {
        ERR("app_init() failed");
        return FALSE;
    }

    DBG("app_init() OK");

    /* read static resources */

    if ( !read_resources(TRUE) )
    {
        ERR("read_resources() failed");
        return FALSE;
    }

    DBG("read_resources() OK");


    /* libSHA1 test */

/*    uint8_t sha1_res1[SHA1_DIGEST_SIZE];
    char    sha1_res2[64];

    DBG("");
    DBG("Trying libSHA1...\n");
    DBG("Expected: [A9993E36 4706816A BA3E2571 7850C26C 9CD0D89D]");

    libSHA1((unsigned char*)"abc", 3, sha1_res1);

    digest_to_hex(sha1_res1, sha1_res2);
    DBG("     Got: [%s]\n", sha1_res2);*/

    /* fill the M_random_numbers up */

    npp_lib_init_random_numbers();

    /* calculate Expires and Last-Modified header fields for static resources */

#ifdef _WIN32   /* Windows */
    strftime(G_last_modified, 32, "%a, %d %b %Y %H:%M:%S GMT", G_ptm);
#else
    strftime(G_last_modified, 32, "%a, %d %b %Y %T GMT", G_ptm);
#endif  /* _WIN32 */
    DBG("Now is: %s\n", G_last_modified);

    set_expiry_dates();

    /* handle signals ---------------------------------------------------- */

    signal(SIGINT,  sigdisp);   /* Ctrl-C */
    signal(SIGTERM, sigdisp);
#ifndef _WIN32
    signal(SIGQUIT, sigdisp);   /* Ctrl-\ */
    signal(SIGTSTP, sigdisp);   /* Ctrl-Z */

    signal(SIGPIPE, SIG_IGN);   /* ignore SIGPIPE */
#endif

    /* initialize SSL connection ----------------------------------------- */

#ifdef NPP_HTTPS
    if ( !init_ssl() )
    {
        ERR("init_ssl failed");
        return FALSE;
    }
#endif  /* NPP_HTTPS */

    /* init G_connections array --------------------------------------------------- */

    for ( i=0; i<NPP_MAX_CONNECTIONS; ++i )
    {
#ifdef NPP_OUT_CHECK_REALLOC
        if ( !(G_connections[i].out_data_alloc = (char*)malloc(NPP_OUT_BUFSIZE)) )
        {
            ERR("malloc for G_connections[%d].out_data failed", i);
            return FALSE;
        }
#endif  /* NPP_OUT_CHECK_REALLOC */

        G_connections[i].out_data_allocated = NPP_OUT_BUFSIZE;
        G_connections[i].in_data = NULL;
        reset_conn(i, CONN_STATE_DISCONNECTED);

#ifdef NPP_HTTPS
        G_connections[i].ssl = NULL;
#endif
        G_connections[i].req = 0;

#ifdef NPP_HTTP2
/*        if ( !(G_connections[i].http2_frame = (char*)malloc(HTTP2_DEFAULT_FRAME_SIZE)) )
        {
            ERR("malloc for G_connections[%d].http2_frame failed", i);
            return FALSE;
        } */
#endif  /* NPP_HTTP2 */
    }

    /* read blocked IPs list --------------------------------------------- */

    read_blocked_ips();

    /* read allowed IPs list --------------------------------------------- */

    read_allowed_ips();

    /* ASYNC ------------------------------------------------------------- */

#ifdef NPP_ASYNC
    ALWAYS("\nOpening message queues...\n");

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
#endif

    struct mq_attr attr={0};

    attr.mq_maxmsg = ASYNC_MQ_MAXMSG;

    /* ------------------------------------------------------------------- */

    if ( mq_unlink(G_req_queue_name) == 0 )
        INF("Message queue %s removed from system", G_req_queue_name);

    attr.mq_msgsize = ASYNC_REQ_MSG_SIZE;

    G_queue_req = mq_open(G_req_queue_name, O_WRONLY | O_CREAT | O_NONBLOCK, 0600, &attr);

    if (G_queue_req < 0)
    {
        ERR("mq_open for req failed, errno = %d (%s)", errno, strerror(errno));
        return FALSE;
    }

    INF("mq_open of %s OK", G_req_queue_name);

    /* ------------------------------------------------------------------- */

    if ( mq_unlink(G_res_queue_name) == 0 )
        INF("Message queue %s removed from system", G_res_queue_name);

    attr.mq_msgsize = ASYNC_RES_MSG_SIZE;   /* larger buffer */

    G_queue_res = mq_open(G_res_queue_name, O_RDONLY | O_CREAT | O_NONBLOCK, 0600, &attr);

    if (G_queue_res < 0)
    {
        ERR("mq_open for res failed, errno = %d (%s)", errno, strerror(errno));
        return FALSE;
    }

    INF("mq_open of %s OK", G_res_queue_name);

    /* ------------------------------------------------------------------- */

    for ( i=0; i<ASYNC_MAX_REQUESTS; ++i )
        M_areqs[i].state = NPP_ASYNC_STATE_FREE;

    M_last_call_id = 0;

    INF("");

#endif  /* NPP_ASYNC */

    npp_sort_messages();

    G_initialized = 1;

    return TRUE;
}


/* --------------------------------------------------------------------------
   Build select list
   This is on the latency critical path
   Try to minimize number of steps
-------------------------------------------------------------------------- */
static void build_fd_sets()
{
#ifdef NPP_FD_MON_SELECT
    FD_ZERO(&M_readfds);
    FD_ZERO(&M_writefds);

    FD_SET(M_listening_fd, &M_readfds);
#ifdef NPP_HTTPS
    FD_SET(M_listening_sec_fd, &M_readfds);
    M_highsock = M_listening_sec_fd;
#else
    M_highsock = M_listening_fd;
#endif  /* NPP_HTTPS */

    int i;
    int remain = G_connections_cnt;

    for ( i=0; remain>0 && i<NPP_MAX_CONNECTIONS; ++i )
    {
        if ( G_connections[i].conn_state == CONN_STATE_DISCONNECTED ) continue;

#ifdef NPP_HTTPS
        if ( NPP_CONN_IS_SECURE(G_connections[i].flags) )
        {
            /* reading */

            if ( G_connections[i].conn_state == CONN_STATE_CONNECTED
                    || G_connections[i].conn_state == CONN_STATE_READING_DATA
                    || G_connections[i].ssl_err == SSL_ERROR_WANT_READ )
            {
                FD_SET(G_connections[i].fd, &M_readfds);
            }

            /* writing */

            else if ( G_connections[i].conn_state == CONN_STATE_READY_TO_SEND_RESPONSE
                    || G_connections[i].conn_state == CONN_STATE_SENDING_CONTENT
#ifdef NPP_ASYNC
                    || G_connections[i].conn_state == CONN_STATE_WAITING_FOR_ASYNC
#endif
                    || G_connections[i].ssl_err == SSL_ERROR_WANT_WRITE )
            {
                FD_SET(G_connections[i].fd, &M_writefds);
            }
        }
        else    /* HTTP */
        {
#endif  /* NPP_HTTPS */
            /* reading */

            if ( G_connections[i].conn_state == CONN_STATE_CONNECTED
#ifdef NPP_HTTP2
                    || G_connections[i].conn_state == CONN_STATE_READY_FOR_CLIENT_PREFACE
#endif
                    || G_connections[i].conn_state == CONN_STATE_READING_DATA )
            {
                FD_SET(G_connections[i].fd, &M_readfds);
            }

            /* writing */

            else if ( G_connections[i].conn_state == CONN_STATE_READY_TO_SEND_RESPONSE
#ifdef NPP_HTTP2
//                    || G_connections[i].conn_state == CONN_STATE_READY_TO_SEND_SETTINGS
#endif
#ifdef NPP_ASYNC
                    || G_connections[i].conn_state == CONN_STATE_WAITING_FOR_ASYNC
#endif
                    || G_connections[i].conn_state == CONN_STATE_SENDING_CONTENT )
            {
                FD_SET(G_connections[i].fd, &M_writefds);
            }
#ifdef NPP_HTTPS
        }
#endif

        if ( G_connections[i].fd > M_highsock )
            M_highsock = G_connections[i].fd;

        remain--;
    }
#ifdef NPP_DEBUG
    if ( remain )
        DBG_T("remain should be 0 but currently %d", remain);
#endif
#endif  /* NPP_FD_MON_SELECT */
}


/* --------------------------------------------------------------------------
   Handle a brand new connection
   we've got fd and IP here for G_connections array
-------------------------------------------------------------------------- */
static void accept_http()
{
    int         i;
    int         connection;
    struct sockaddr_in cli_addr;
    socklen_t   addr_len;
    char        remote_addr[INET_ADDRSTRLEN]="";

    /* We have a new connection coming in! We'll
       try to find a spot for it in G_connections array  */

    addr_len = sizeof(cli_addr);

    connection = accept(M_listening_fd, (struct sockaddr*)&cli_addr, &addr_len);

    if ( connection < 0 )
    {
        ERR("accept failed, errno = %d (%s)", errno, strerror(errno));
        return;
    }

    /* get the remote address */
#ifdef _WIN32   /* Windows */
    strcpy(remote_addr, inet_ntoa(cli_addr.sin_addr));
#else
    inet_ntop(AF_INET, &(cli_addr.sin_addr), remote_addr, INET_ADDRSTRLEN);
#endif

    if ( G_blockedIPList[0] && ip_blocked(remote_addr) )
    {
        ++G_cnts_today.blocked;
#ifdef _WIN32   /* Windows */
        closesocket(connection);
#else
        close(connection);
#endif  /* _WIN32 */
        return;
    }

    if ( G_whiteList[0] && !ip_allowed(remote_addr) )
    {
#ifdef _WIN32   /* Windows */
        closesocket(connection);
#else
        close(connection);
#endif  /* _WIN32 */
        return;
    }

    npp_lib_setnonblocking(connection);

    /* find a free slot in G_connections */

    for ( i=0; i<NPP_MAX_CONNECTIONS; ++i )
    {
        if ( G_connections[i].conn_state == CONN_STATE_DISCONNECTED )    /* free connection slot -- we'll use it */
        {
            DBG("\nConnection accepted: %s, slot=%d, fd=%d", remote_addr, i, connection);
            G_connections[i].fd = connection;
            G_connections[i].flags = G_connections[i].flags & ~NPP_CONN_FLAG_SECURE;

            if ( ++G_connections_cnt > G_connections_hwm )
                G_connections_hwm = G_connections_cnt;

            strcpy(G_connections[i].ip, remote_addr);        /* possibly client IP */
            strcpy(G_connections[i].pip, remote_addr);       /* possibly proxy IP */

            DDBG("Changing state to CONN_STATE_CONNECTED");
            G_connections[i].conn_state = CONN_STATE_CONNECTED;

            G_connections[i].last_activity = G_now;

#ifdef NPP_FD_MON_POLL  /* add connection to monitored set */
            /* reference ... */
            G_connections[i].pi = M_pollfds_cnt;
            /* ... each other to avoid unnecessary looping */
            M_poll_ci[M_pollfds_cnt] = i;
            M_pollfds[M_pollfds_cnt].fd = connection;
            M_pollfds[M_pollfds_cnt].events = POLLIN;
            M_pollfds[M_pollfds_cnt].revents = 0;
            ++M_pollfds_cnt;
#endif  /* NPP_FD_MON_POLL */

            connection = -1;                        /* mark as OK */
            break;
        }
    }

    if (connection != -1)   /* none was free */
    {
        WAR("No room left for new client, sending 503");

        int bytes = send(connection, "HTTP/1.1 503 Service Unavailable\r\n\r\n", 36, 0);

        if ( bytes < 36 )
            ERR("write error, bytes = %d of 36", bytes);
#ifdef _WIN32   /* Windows */
        closesocket(connection);
#else
        close(connection);
#endif  /* _WIN32 */
    }
}


/* --------------------------------------------------------------------------
   Handle a brand new connection
   we've got fd and IP here for G_connections array
-------------------------------------------------------------------------- */
static void accept_https()
{
#ifdef NPP_HTTPS
    int         i;
    int         connection;
    struct sockaddr_in cli_addr;
    socklen_t   addr_len;
    char        remote_addr[INET_ADDRSTRLEN]="";
    int         ret;

    /* We have a new connection coming in! We'll
       try to find a spot for it in G_connections array  */

    addr_len = sizeof(cli_addr);

    connection = accept(M_listening_sec_fd, (struct sockaddr*)&cli_addr, &addr_len);

    if ( connection < 0 )
    {
        ERR("accept failed, errno = %d (%s)", errno, strerror(errno));
        return;
    }

    /* get the remote address */
#ifdef _WIN32   /* Windows */
    strcpy(remote_addr, inet_ntoa(cli_addr.sin_addr));
#else
    inet_ntop(AF_INET, &(cli_addr.sin_addr), remote_addr, INET_ADDRSTRLEN);
#endif

    if ( G_blockedIPList[0] && ip_blocked(remote_addr) )
    {
        ++G_cnts_today.blocked;
#ifdef _WIN32   /* Windows */
        closesocket(connection);
#else
        close(connection);
#endif  /* _WIN32 */
        return;
    }

    if ( G_whiteList[0] && !ip_allowed(remote_addr) )
    {
#ifdef _WIN32   /* Windows */
        closesocket(connection);
#else
        close(connection);
#endif  /* _WIN32 */
        return;
    }

    npp_lib_setnonblocking(connection);

    /* find a free slot in G_connections */

    for ( i=0; i<NPP_MAX_CONNECTIONS; ++i )
    {
        if ( G_connections[i].conn_state == CONN_STATE_DISCONNECTED )    /* free connection slot -- we'll use it */
        {
            DBG("\nSecure connection accepted: %s, slot=%d, fd=%d", remote_addr, i, connection);
            G_connections[i].fd = connection;
            G_connections[i].flags |= NPP_CONN_FLAG_SECURE;

            if ( ++G_connections_cnt > G_connections_hwm )
                G_connections_hwm = G_connections_cnt;

#ifdef NPP_FD_MON_POLL  /* add connection to monitored set */
            /* reference ... */
            G_connections[i].pi = M_pollfds_cnt;
            /* ... each other to avoid unnecessary looping */
            M_poll_ci[M_pollfds_cnt] = i;
            M_pollfds[M_pollfds_cnt].fd = connection;
            M_pollfds[M_pollfds_cnt].events = POLLIN;
            M_pollfds[M_pollfds_cnt].revents = 0;
            ++M_pollfds_cnt;
#endif  /* NPP_FD_MON_POLL */

            G_connections[i].ssl = SSL_new(M_ssl_ctx);

            if ( !G_connections[i].ssl )
            {
                ERR("SSL_new failed");
                close_conn(i);
                return;
            }

            /* SSL_set_fd() sets the file descriptor fd as the input/output facility
               for the TLS/SSL (encrypted) side of ssl. fd will typically be the socket
               file descriptor of a network connection.
               When performing the operation, a socket BIO is automatically created to
               interface between the ssl and fd. The BIO and hence the SSL engine inherit
               the behaviour of fd. If fd is non-blocking, the ssl will also have non-blocking behaviour.
               If there was already a BIO connected to ssl, BIO_free() will be called
               (for both the reading and writing side, if different). */

            ret = SSL_set_fd(G_connections[i].ssl, connection);

            if ( ret <= 0 )
            {
                ERR("SSL_set_fd failed, ret = %d", ret);
                close_conn(i);
                return;
            }

            ret = SSL_accept(G_connections[i].ssl);   /* handshake here */

            if ( ret <= 0 )
            {
                G_connections[i].ssl_err = SSL_get_error(G_connections[i].ssl, ret);

                if ( G_connections[i].ssl_err != SSL_ERROR_WANT_READ && G_connections[i].ssl_err != SSL_ERROR_WANT_WRITE )
                {
                    DBG("SSL_accept failed, ssl_err = %d", G_connections[i].ssl_err);
                    close_conn(i);
                    return;
                }
            }

#ifdef NPP_FD_MON_POLL
            if ( G_connections[i].ssl_err == SSL_ERROR_WANT_WRITE )
                M_pollfds[G_connections[i].pi].events = POLLOUT;
#endif
            strcpy(G_connections[i].ip, remote_addr);        /* possibly client IP */
            strcpy(G_connections[i].pip, remote_addr);       /* possibly proxy IP */

            DDBG("Changing state to CONN_STATE_ACCEPTING");
            G_connections[i].conn_state = CONN_STATE_ACCEPTING;

            G_connections[i].last_activity = G_now;
            connection = -1;                        /* mark as OK */
            break;
        }
    }

    if (connection != -1)   /* none was free */
    {
        WAR("No room left for new client, sending 503");

        int bytes = send(connection, "HTTP/1.1 503 Service Unavailable\r\n\r\n", 36, 0);

        if ( bytes < 36 )
            ERR("write error, bytes = %d of 36", bytes);
#ifdef _WIN32   /* Windows */
        closesocket(connection);
#else
        close(connection);
#endif  /* _WIN32 */
    }
#endif
}


/* --------------------------------------------------------------------------
   Read list of blocked IPs from the file
-------------------------------------------------------------------------- */
static void read_blocked_ips()
{
    char    fname[1024];
    FILE    *h_file=NULL;
    char    c;
    int     i=0;
    char    now_value=1;
    char    now_comment=0;
    char    value[INET_ADDRSTRLEN];

    if ( G_blockedIPList[0] == EOS ) return;

    INF("Updating blocked IPs list");

    /* open the file */

    if ( G_blockedIPList[0] == '/' )    /* full path */
        strcpy(fname, G_blockedIPList);
    else if ( G_appdir[0] )
        sprintf(fname, "%s/bin/%s", G_appdir, G_blockedIPList);
    else
        strcpy(fname, G_blockedIPList);

    if ( NULL == (h_file=fopen(fname, "r")) )
    {
        WAR("Couldn't open %s", fname);
        return;
    }

    G_blacklist_cnt = 0;

    /* parse the file */

    while ( EOF != (c=fgetc(h_file)) )
    {
        if ( c == ' ' || c == '\t' || c == '\r' ) continue;  /* omit whitespaces */

        if ( c == '\n' )    /* end of value or end of comment or empty line */
        {
            if ( now_value && i )   /* end of value */
            {
                value[i] = EOS;
                i = 0;
                if ( !ip_blocked(value) )   /* avoid duplicates */
                {
                    strcpy(G_blacklist[G_blacklist_cnt++], value);
                    if ( G_blacklist_cnt == NPP_MAX_BLACKLIST )
                    {
                        WAR("Blacklist full! (%d IPs)", G_blacklist_cnt);
                        now_value = 0;
                        break;
                    }
                }
            }
            now_value = 1;
            now_comment = 0;
        }
        else if ( now_comment )
        {
            continue;
        }
        else if ( c == '#' )    /* possible end of value */
        {
            if ( now_value && i )   /* end of value */
            {
                value[i] = EOS;
                i = 0;
                strcpy(G_blacklist[G_blacklist_cnt++], value);
                if ( G_blacklist_cnt == NPP_MAX_BLACKLIST )
                {
                    WAR("Blacklist full! (%d IPs)", G_blacklist_cnt);
                    now_value = 0;
                    break;
                }
            }
            now_value = 0;
            now_comment = 1;
        }
        else if ( now_value )   /* value */
        {
            if ( i < INET_ADDRSTRLEN-1 )
                value[i++] = c;
        }
    }

    if ( now_value && i )   /* end of value */
    {
        value[i] = EOS;
        strcpy(G_blacklist[G_blacklist_cnt++], value);
    }

//    if ( NULL != h_file )
        fclose(h_file);

    ALWAYS("%d IPs blacklisted", G_blacklist_cnt);

    /* show the list */

/*  DBG("");
    for ( i=0; i<G_blacklist_cnt; ++i )
        DBG("%s", G_blacklist[i]);
    DBG("");*/
}


/* --------------------------------------------------------------------------
   Return TRUE if addr is on our blacklist
-------------------------------------------------------------------------- */
static bool ip_blocked(const char *addr)
{
    int i;

    for ( i=0; i<G_blacklist_cnt; ++i )
    {
        if ( 0==strcmp(G_blacklist[i], addr) )
            return TRUE;
    }

    return FALSE;
}


/* --------------------------------------------------------------------------
   Read whitelist from the file
-------------------------------------------------------------------------- */
static void read_allowed_ips()
{
    char    fname[1024];
    FILE    *h_file=NULL;
    int     c=0;
    int     i=0;
    char    now_value=1;
    char    now_comment=0;
    char    value[64]="";

    if ( G_whiteList[0] == EOS ) return;

    INF("Updating whitelist");

    /* open the file */

    if ( G_whiteList[0] == '/' )    /* full path */
        strcpy(fname, G_whiteList);
    else if ( G_appdir[0] )
        sprintf(fname, "%s/bin/%s", G_appdir, G_whiteList);
    else
        strcpy(fname, G_whiteList);

    if ( NULL == (h_file=fopen(fname, "r")) )
    {
        WAR("Couldn't open %s", fname);
        return;
    }

    G_whitelist_cnt = 0;

    /* parse the file */

    while ( EOF != (c=fgetc(h_file)) )
    {
        if ( c == ' ' || c == '\t' || c == '\r' ) continue;  /* omit whitespaces */

        if ( c == '\n' )    /* end of value or end of comment or empty line */
        {
            if ( now_value && i )   /* end of value */
            {
                value[i] = EOS;
                if ( !ip_blocked(value) )   /* avoid duplicates */
                {
                    strcpy(G_whitelist[G_whitelist_cnt++], value);
                    if ( G_whitelist_cnt == NPP_MAX_WHITELIST )
                    {
                        WAR("Whitelist full! (%d IPs)", G_whitelist_cnt);
                        now_value = 0;
                        break;
                    }
                }
                i = 0;
            }
            now_value = 1;
            now_comment = 0;
        }
        else if ( now_comment )
        {
            continue;
        }
        else if ( c == '#' )    /* possible end of value */
        {
            if ( now_value && i )   /* end of value */
            {
                value[i] = EOS;
                strcpy(G_whitelist[G_whitelist_cnt++], value);
                if ( G_whitelist_cnt == NPP_MAX_WHITELIST )
                {
                    WAR("Whitelist full! (%d IPs)", G_whitelist_cnt);
                    now_value = 0;
                    break;
                }
                i = 0;
            }
            now_value = 0;
            now_comment = 1;
        }
        else if ( now_value )   /* value */
        {
            if ( i < INET_ADDRSTRLEN-1 )
                value[i++] = c;
        }
    }

    if ( now_value && i )   /* end of value */
    {
        value[i] = EOS;
        strcpy(G_whitelist[G_whitelist_cnt++], value);
    }

//    if ( NULL != h_file )
        fclose(h_file);

    ALWAYS("%d IPs on whitelist", G_whitelist_cnt);
}


/* --------------------------------------------------------------------------
   Return TRUE if addr is on whitelist
-------------------------------------------------------------------------- */
static bool ip_allowed(const char *addr)
{
    int i;

    for ( i=0; i<G_whitelist_cnt; ++i )
    {
        if ( 0==strcmp(G_whitelist[i], addr) )
            return TRUE;
    }

    return FALSE;
}


/* --------------------------------------------------------------------------
   Read static resources from disk
   Read all the files from G_appdir/res or resmin directory
   path is a relative path under `res` or `resmin`
   Unlike snippets, these are only used by main npp_app process
-------------------------------------------------------------------------- */
static bool read_files(const char *host, const char *directory, char source, bool first_scan, const char *path)
{
    bool    minify=FALSE;
    int     i;
    char    resdir[NPP_STATIC_PATH_LEN+1];      /* full path to res */
    char    ressubdir[NPP_STATIC_PATH_LEN*2+2]; /* full path to res/subdir */
    char    namewpath[NPP_STATIC_PATH_LEN*2+2]; /* full path including file name */
    char    resname[NPP_STATIC_PATH_LEN+1];     /* relative path including file name */
    DIR     *dir;
    struct dirent *dirent;
    FILE    *fd;
    char    *data_tmp=NULL;
    char    *data_tmp_min=NULL;
    struct stat fstat;

#ifndef _WIN32
    if ( G_appdir[0] == EOS ) return TRUE;
#endif

    if ( first_scan && !path ) DBG("");

#ifdef NPP_DEBUG
    if ( first_scan )
    {
        if ( !path ) DBG_LINE_LONG;
        DBG("read_files, directory [%s]", directory);
    }
#endif  /* NPP_DEBUG */

#ifdef _WIN32   /* be more forgiving */

    if ( G_appdir[0] )
    {
        sprintf(resdir, "%s\\%s", G_appdir, directory);
    }
    else    /* no NPP_DIR */
    {
        sprintf(resdir, "..\\%s", directory);
    }

#else   /* Linux -- don't fool around */

    sprintf(resdir, "%s/%s", G_appdir, directory);

#endif  /* _WIN32 */

#ifdef NPP_DEBUG
    if ( first_scan )
        DBG("resdir [%s]", resdir);
#endif

    if ( !path )   /* highest level */
    {
        strcpy(ressubdir, resdir);
    }
    else    /* recursive call */
    {
#ifdef _WIN32
        sprintf(ressubdir, "%s\\%s", resdir, path);
#else
        sprintf(ressubdir, "%s/%s", resdir, path);
#endif
    }

#ifdef NPP_DEBUG
    if ( first_scan )
        DBG("ressubdir [%s]", ressubdir);
#endif

    if ( (dir=opendir(ressubdir)) == NULL )
    {
        if ( first_scan )
            DBG("Couldn't open directory [%s]", ressubdir);
        return TRUE;    /* don't panic, just no external resources will be used */
    }

    /* ------------------------------------------------------------------- */
    /* check removed files */

    if ( !first_scan && !path )   /* on the highest level only */
    {
//        DDBG("Checking removed files...");

        for ( i=0; i<=M_static_cnt; ++i )
        {
            if ( M_stat[i].name[0]==EOS ) continue;   /* already removed */

            if ( 0 != strcmp(M_stat[i].host, host) || M_stat[i].source != source ) continue;

//            DDBG("Checking %s...", M_stat[i].name);

            char fullpath[NPP_STATIC_PATH_LEN*2];
#ifdef _WIN32
            sprintf(fullpath, "%s\\%s", resdir, M_stat[i].name);
#else
            sprintf(fullpath, "%s/%s", resdir, M_stat[i].name);
#endif
            if ( !npp_file_exists(fullpath) )
            {
                INF("Removing %s from static resources", M_stat[i].name);

                if ( 0==strcmp(directory, "res") )
                {
                    if ( 0==strcmp(M_stat[i].name, "index.html") )
                        G_index_present = FALSE;
                    else if ( 0==strcmp(M_stat[i].name, "favicon.ico") )
                        M_popular_favicon = FALSE;
                    else if ( 0==strcmp(M_stat[i].name, "apple-touch-icon.png") )
                        M_popular_appleicon = FALSE;
                    else if ( 0==strcmp(M_stat[i].name, "robots.txt") )
                        M_popular_robots = FALSE;
                }
                else if ( 0==strcmp(directory, "resmin") )
                {
                    if ( 0==strcmp(M_stat[i].name, "sw.js") )
                        M_popular_sw = FALSE;
                }
#ifdef NPP_MULTI_HOST
                else    /* side gig */
                {
                    if ( 0==strcmp(M_stat[i].name, "index.html") )
                    {
                        int j;
                        for ( j=1; j<M_hosts_cnt; ++j )
                        {
                            if ( 0==strcmp(M_hosts[j].host, host) )
                            {
                                M_hosts[j].index_present = FALSE;
                                break;
                            }
                        }
                    }
                }
#endif  /* NPP_MULTI_HOST */

                M_stat[i].host[0] = EOS;
                M_stat[i].name[0] = EOS;

                free(M_stat[i].data);
                M_stat[i].data = NULL;
                M_stat[i].len = 0;

                if ( M_stat[i].data_deflated )
                {
                    free(M_stat[i].data_deflated);
                    M_stat[i].data_deflated = NULL;
                    M_stat[i].len_deflated = 0;
                }
            }
        }
    }

    /* ------------------------------------------------------------------- */

//    DDBG("Reading %sfiles", first_scan?"":"new ");

    /* read files into memory */

    while ( (dirent=readdir(dir)) )
    {
        if ( dirent->d_name[0] == '.' )   /* skip ".", ".." and hidden files */
            continue;

        /* ------------------------------------------------------------------- */
        /* resource name */

        if ( !path )
            strcpy(resname, dirent->d_name);
        else
#ifdef _WIN32
            sprintf(resname, "%s\\%s", path, dirent->d_name);
#else
            sprintf(resname, "%s/%s", path, dirent->d_name);
#endif

#ifdef NPP_DEBUG
        if ( first_scan )
            DBG("resname [%s]", resname);
#endif

        /* ------------------------------------------------------------------- */
        /* additional file info */

#ifdef _WIN32
        sprintf(namewpath, "%s\\%s", resdir, resname);
#else
        sprintf(namewpath, "%s/%s", resdir, resname);
#endif

#ifdef NPP_DEBUG
        if ( first_scan )
            DBG("namewpath [%s]", namewpath);
#endif

        if ( stat(namewpath, &fstat) != 0 )
        {
            ERR("stat for [%s] failed, errno = %d (%s)", namewpath, errno, strerror(errno));
            closedir(dir);
            return FALSE;
        }

        /* ------------------------------------------------------------------- */

        if ( S_ISDIR(fstat.st_mode) )   /* directory */
        {
#ifdef NPP_DEBUG
            if ( first_scan )
                DBG("Reading subdirectory [%s]...", dirent->d_name);
#endif
            read_files(host, directory, source, first_scan, resname);
            continue;
        }
        else if ( !S_ISREG(fstat.st_mode) )    /* skip if not a regular file nor directory */
        {
#ifdef NPP_DEBUG
            if ( first_scan )
                DBG("[%s] is not a regular file", resname);
#endif
            continue;
        }

        /* ------------------------------------------------------------------- */
        /* already read? */

        bool reread = FALSE;

        if ( !first_scan )
        {
            bool exists_not_changed = FALSE;

            for ( i=0; i<=M_static_cnt; ++i )
            {
                if ( M_stat[i].name[0]==EOS ) continue;   /* removed */

                /* ------------------------------------------------------------------- */

                if ( 0==strcmp(M_stat[i].host, host) && 0==strcmp(M_stat[i].name, resname) && M_stat[i].source == source )
                {
//                    DDBG("%s already read", resname);

                    if ( M_stat[i].modified == fstat.st_mtime )
                    {
//                        DDBG("Not modified");
                        exists_not_changed = TRUE;
                    }
                    else
                    {
                        INF("%s has been modified", resname);
                        reread = TRUE;
                    }

                    break;
                }
            }

            if ( exists_not_changed ) continue;   /* not modified */
        }

        /* find the first unused slot in M_stat array */

        if ( !reread )
        {
            i = first_free_stat();

            /* host -- already uppercase */

            strcpy(M_stat[i].host, host);

            /* file name */

            strcpy(M_stat[i].name, resname);
        }

        /* source */

        M_stat[i].source = source;

        if ( source == STATIC_SOURCE_RESMIN )
            minify = TRUE;

        /* last modified */

        M_stat[i].modified = fstat.st_mtime;

        /* size and content */

#ifdef _WIN32   /* Windows */
        if ( NULL == (fd=fopen(namewpath, "rb")) )
#else
        if ( NULL == (fd=fopen(namewpath, "r")) )
#endif  /* _WIN32 */
            ERR("Couldn't open %s", namewpath);
        else
        {
            fseek(fd, 0, SEEK_END);     /* determine the file size */
            M_stat[i].len = ftell(fd);
            rewind(fd);

            if ( minify )
            {
                /* we don't know the minified size yet -- read file into temp buffer */

                if ( NULL == (data_tmp=(char*)malloc(M_stat[i].len+1)) )
                {
                    ERR("Couldn't allocate %u bytes for %s", M_stat[i].len, M_stat[i].name);
                    fclose(fd);
                    closedir(dir);
                    return FALSE;
                }

                if ( NULL == (data_tmp_min=(char*)malloc(M_stat[i].len+1)) )
                {
                    ERR("Couldn't allocate %u bytes for %s", M_stat[i].len, M_stat[i].name);
                    fclose(fd);
                    closedir(dir);
                    return FALSE;
                }

                if ( fread(data_tmp, M_stat[i].len, 1, fd) != 1 )
                {
                    ERR("Couldn't read from %s", M_stat[i].name);
                    fclose(fd);
                    closedir(dir);
                    return FALSE;
                }

                *(data_tmp+M_stat[i].len) = EOS;

                M_stat[i].len = npp_minify(data_tmp_min, data_tmp);   /* new length */
            }

            /* allocate the final destination */

            if ( reread )
            {
                free(M_stat[i].data);
                M_stat[i].data = NULL;

                if ( M_stat[i].data_deflated )
                {
                    free(M_stat[i].data_deflated);
                    M_stat[i].data_deflated = NULL;
                }
            }

            M_stat[i].data = (char*)malloc(M_stat[i].len+1+NPP_OUT_HEADER_BUFSIZE);

            if ( NULL == M_stat[i].data )
            {
                ERR("Couldn't allocate %u bytes for %s", M_stat[i].len+1+NPP_OUT_HEADER_BUFSIZE, M_stat[i].name);
                fclose(fd);
                closedir(dir);
                return FALSE;
            }

            if ( minify )   /* STATIC_SOURCE_RESMIN */
            {
                memcpy(M_stat[i].data+NPP_OUT_HEADER_BUFSIZE, data_tmp_min, M_stat[i].len+1);
                free(data_tmp);
                data_tmp = NULL;
                free(data_tmp_min);
                data_tmp_min = NULL;
            }
            else if ( M_stat[i].source == STATIC_SOURCE_RES )
            {
                if ( fread(M_stat[i].data+NPP_OUT_HEADER_BUFSIZE, M_stat[i].len, 1, fd) != 1 )
                {
                    ERR("Couldn't read from %s", M_stat[i].name);
                    fclose(fd);
                    closedir(dir);
                    return FALSE;
                }
            }

            fclose(fd);

            /* get the file type ------------------------------- */

            if ( !reread )
            {
                M_stat[i].type = npp_lib_get_res_type(M_stat[i].name);

                if ( 0==strcmp(directory, "res") )
                {
                    if ( 0==strcmp(M_stat[i].name, "index.html") )
                        G_index_present = TRUE;
                    else if ( 0==strcmp(M_stat[i].name, "favicon.ico") )
                        M_popular_favicon = TRUE;
                    else if ( 0==strcmp(M_stat[i].name, "apple-touch-icon.png") )
                        M_popular_appleicon = TRUE;
                    else if ( 0==strcmp(M_stat[i].name, "robots.txt") )
                        M_popular_robots = TRUE;
                }
                else if ( 0==strcmp(directory, "resmin") )
                {
                    if ( 0==strcmp(M_stat[i].name, "sw.js") )
                        M_popular_sw = TRUE;
                }
#ifdef NPP_MULTI_HOST
                else    /* side gig */
                {
                    if ( 0==strcmp(M_stat[i].name, "index.html") )
                    {
                        int j;
                        for ( j=1; j<M_hosts_cnt; ++j )
                        {
                            if ( 0==strcmp(M_hosts[j].host, host) )
                            {
                                M_hosts[j].index_present = TRUE;
                                break;
                            }
                        }
                    }
                }
#endif  /* NPP_MULTI_HOST */
            }

            /* compress ---------------------------------------- */

#ifndef _WIN32

            if ( SHOULD_BE_COMPRESSED(M_stat[i].len, M_stat[i].type) && M_stat[i].source != STATIC_SOURCE_SNIPPETS )
            {
                if ( NULL == (data_tmp=(char*)malloc(M_stat[i].len)) )
                {
                    ERR("Couldn't allocate %u bytes for %s", M_stat[i].len, M_stat[i].name);
                    fclose(fd);
                    closedir(dir);
                    return FALSE;
                }

                unsigned deflated_len = deflate_data((unsigned char*)data_tmp, (unsigned char*)M_stat[i].data+NPP_OUT_HEADER_BUFSIZE, M_stat[i].len);

                if ( deflated_len == -1 )
                {
                    WAR("Couldn't compress %s", M_stat[i].name);

                    if ( M_stat[i].data_deflated )
                    {
                        free(M_stat[i].data_deflated);
                        M_stat[i].data_deflated = NULL;
                    }

                    M_stat[i].len_deflated = 0;
                }
                else
                {
                    if ( NULL == (M_stat[i].data_deflated=(char*)malloc(deflated_len+NPP_OUT_HEADER_BUFSIZE)) )
                    {
                        ERR("Couldn't allocate %u bytes for deflated %s", deflated_len+NPP_OUT_HEADER_BUFSIZE, M_stat[i].name);
                        fclose(fd);
                        closedir(dir);
                        free(data_tmp);
                        return FALSE;
                    }

                    memcpy(M_stat[i].data_deflated+NPP_OUT_HEADER_BUFSIZE, data_tmp, deflated_len);
                    M_stat[i].len_deflated = deflated_len;
                }

                free(data_tmp);
                data_tmp = NULL;
            }

#endif  /* _WIN32 */

            /* log file info ----------------------------------- */

            if ( G_logLevel > LOG_INF )
            {
                G_ptm = gmtime(&M_stat[i].modified);
                char mod_time[128];
                sprintf(mod_time, "%d-%02d-%02d %02d:%02d:%02d", G_ptm->tm_year+1900, G_ptm->tm_mon+1, G_ptm->tm_mday, G_ptm->tm_hour, G_ptm->tm_min, G_ptm->tm_sec);
                G_ptm = gmtime(&G_now);     /* set it back */
                DBG("%s %s\t\t%u bytes", npp_add_spaces(M_stat[i].name, 28), mod_time, M_stat[i].len);
            }
        }

#ifdef NPP_DEBUG
//      if ( minify )   /* temporarily */
//          DBG("minified %s: [%s]", M_stat[i].name, M_stat[i].data);
#endif  /* NPP_DEBUG */
    }

    closedir(dir);

    if ( first_scan && !path ) DBG("");

    return TRUE;
}


/* --------------------------------------------------------------------------
   Read all static resources from disk
-------------------------------------------------------------------------- */
static bool read_resources(bool first_scan)
{
    if ( !read_files("", "res", STATIC_SOURCE_RES, first_scan, NULL) )
    {
        ERR("reading res failed");
        return FALSE;
    }

    if ( !read_files("", "resmin", STATIC_SOURCE_RESMIN, first_scan, NULL) )
    {
        ERR("reading resmin failed");
        return FALSE;
    }

    if ( !npp_lib_read_snippets("", "snippets", first_scan, NULL) )
    {
        ERR("reading snippets failed");
        return FALSE;
    }


#ifdef NPP_MULTI_HOST   /* side gigs */

    int i;

    for ( i=1; i<M_hosts_cnt; ++i )
    {
        if ( M_hosts[i].res[0] && !read_files(M_hosts[i].host, M_hosts[i].res, STATIC_SOURCE_RES, first_scan, NULL) )
        {
            ERR("reading %s's res failed", M_hosts[i].host);
            return FALSE;
        }

        if ( M_hosts[i].resmin[0] && !read_files(M_hosts[i].host, M_hosts[i].resmin, STATIC_SOURCE_RESMIN, first_scan, NULL) )
        {
            ERR("reading %s's resmin failed", M_hosts[i].host);
            return FALSE;
        }

        if ( M_hosts[i].snippets[0] && !npp_lib_read_snippets(M_hosts[i].host, M_hosts[i].snippets, first_scan, NULL) )
        {
            ERR("reading %s's snippets failed", M_hosts[i].host);
            return FALSE;
        }
    }

#endif  /* NPP_MULTI_HOST */

    return TRUE;
}


/* --------------------------------------------------------------------------
   Find first free slot in M_stat
-------------------------------------------------------------------------- */
static int first_free_stat()
{
    int i=0;

    for ( i=0; i<NPP_MAX_STATICS; ++i )
    {
        if ( M_stat[i].name[0]=='-' || M_stat[i].name[0]==EOS )
        {
            if ( i > M_static_cnt ) M_static_cnt = i;
            return i;
        }
    }

    ERR("NPP_MAX_STATICS reached (%d)! You can set/increase APP_MAX_STATICS in npp_app.h.", NPP_MAX_STATICS);

    return -1;  /* nothing's free, we ran out of statics! */
}


/* --------------------------------------------------------------------------
   Return M_stat array index if URI is on statics' list
-------------------------------------------------------------------------- */
static int is_static_res(int ci)
{
    int i;

#ifdef NPP_MULTI_HOST

    if ( !G_connections[ci].host_id )    /* main host */
    {
        for ( i=0; M_stat[i].name[0] != '-'; ++i )
        {
            if ( !M_stat[i].host[0] && 0==strcmp(M_stat[i].name, G_connections[ci].uri) /*&& M_stat[i].source != STATIC_SOURCE_SNIPPETS*/ )
            {
                if ( G_connections[ci].if_mod_since >= M_stat[i].modified )
                {
                    G_connections[ci].status = 304;  /* Not Modified */
                }

                return i;
            }
        }
    }
    else    /* side gig */
    {
        for ( i=0; M_stat[i].name[0] != '-'; ++i )
        {
            if ( 0==strcmp(M_stat[i].host, G_connections[ci].host_normalized) && 0==strcmp(M_stat[i].name, G_connections[ci].uri) )
            {
                if ( G_connections[ci].if_mod_since >= M_stat[i].modified )
                {
                    G_connections[ci].status = 304;  /* Not Modified */
                }

                return i;
            }
        }
    }

#else   /* NOT NPP_MULTI_HOST */

    for ( i=0; M_stat[i].name[0] != '-'; ++i )
    {
        if ( 0==strcmp(M_stat[i].name, G_connections[ci].uri) /*&& M_stat[i].source != STATIC_SOURCE_SNIPPETS*/ )
        {
            if ( G_connections[ci].if_mod_since >= M_stat[i].modified )
            {
                G_connections[ci].status = 304;  /* Not Modified */
            }

            return i;
        }
    }

#endif  /* NPP_MULTI_HOST */

    return -1;
}


/* --------------------------------------------------------------------------
   Main new request processing
   Request received over current G_connections is already parsed
-------------------------------------------------------------------------- */
static void process_req(int ci)
{
    int ret=OK;

    DBG("process_req, ci=%d", ci);

    /* ------------------------------------------------------------------------ */

#ifdef NPP_DEBUG
    if ( NPP_CONN_IS_PAYLOAD(G_connections[ci].flags) && G_connections[ci].in_data )
        log_long(G_connections[ci].in_data, G_connections[ci].was_read, "Payload");
#endif

    /* ------------------------------------------------------------------------ */
    /* make room for an outgoing header */

    G_connections[ci].p_content = G_connections[ci].out_data + NPP_OUT_HEADER_BUFSIZE;

    /* ------------------------------------------------------------------------ */

    if ( G_connections[ci].status != 200 )
        return;

    /* ------------------------------------------------------------------------ */
    /* Generate HTML content before header -- to know its size & type --------- */

    /* ------------------------------------------------------------------------ */
    /* authorization check / log in from cookies ------------------------------ */

    bool fresh_session=FALSE;

#ifdef NPP_USERS

#ifdef NPP_ALLOW_BEARER_AUTH
    /* copy bearer token to cookie_in_l */

    if ( !G_connections[ci].cookie_in_l[0] && G_connections[ci].authorization[0] )
    {
        char type[8];
        strncpy(type, npp_upper(G_connections[ci].authorization), 7);
        type[7] = EOS;

        if ( 0==strcmp(type, "BEARER ") )
        {
            strncpy(G_connections[ci].cookie_in_l, G_connections[ci].authorization+7, NPP_SESSID_LEN);
            G_connections[ci].cookie_in_l[NPP_SESSID_LEN] = EOS;
        }
    }
#endif  /* NPP_ALLOW_BEARER_AUTH */

    /* authenticated requests' checks */

    if ( LOGGED )
    {
        DBG("Connection already authenticated");
    }
    else if ( G_connections[ci].cookie_in_l[0] )  /* logged in sesid cookie present */
    {
        ret = libusr_luses_ok(ci);     /* is it valid? */

        if ( ret == OK )    /* valid sesid -- user logged in */
            DBG("User logged in from cookie");
        else if ( ret != ERR_INT_SERVER_ERROR && ret != ERR_SERVER_TOOBUSY )   /* dodged sesid... or session expired */
            WAR("Invalid ls cookie");
    }

    /* authorization */

    if ( LOGGED && G_connections[ci].required_auth_level > G_sessions[G_connections[ci].si].auth_level )
    {
        WAR("Insufficient user authorization level, returning 404");
        ret = ERR_NOT_FOUND;
        RES_DONT_CACHE;
    }
    else if ( !LOGGED && G_connections[ci].required_auth_level > AUTH_LEVEL_ANONYMOUS )   /* redirect to login page */
    {
        INF("auth_level > AUTH_LEVEL_ANONYMOUS required, redirecting to login");

        ret = ERR_REDIRECTION;

#ifndef NPP_DONT_PASS_QS_ON_LOGIN_REDIRECTION
        char *qs = strchr(G_connections[ci].uri, '?');

        if ( !strlen(NPP_LOGIN_URI) )   /* login page = landing page */
            sprintf(G_connections[ci].location, "/%s", qs?qs:"");
        else
            sprintf(G_connections[ci].location, "%s%s", NPP_LOGIN_URI, qs?qs:"");
#else   /* don't pass the query string */
        if ( !strlen(NPP_LOGIN_URI) )   /* login page = landing page */
            strcpy(G_connections[ci].location, "/");
        else
            strcpy(G_connections[ci].location, NPP_LOGIN_URI);
#endif  /* NPP_DONT_PASS_QS_ON_LOGIN_REDIRECTION */

    }
    else
    {
        DDBG("Request authorized");
        ret = OK;
    }

    /* anonymous session check */

    if ( G_connections[ci].required_auth_level==AUTH_LEVEL_ANONYMOUS && !REQ_BOT && G_connections[ci].method[0]!='H' && !LOGGED )    /* anonymous user session required */
#else   /* NPP_USERS NOT defined */
    if ( G_connections[ci].required_auth_level==AUTH_LEVEL_ANONYMOUS && !REQ_BOT && G_connections[ci].method[0]!='H' )
#endif  /* NPP_USERS */
    {
        DDBG("At least anonymous session is required");

        if ( !G_connections[ci].cookie_in_a[0] || !a_usession_ok(ci) )       /* valid anonymous sesid cookie not present */
        {
            ret = npp_eng_session_start(ci, NULL);
            fresh_session = TRUE;
        }
    }

    /* ------------------------------------------------------------------------ */
    /* process request -------------------------------------------------------- */

    if ( ret == OK )
    {
        if ( !G_connections[ci].location[0] )    /* if not redirection */
        {
#ifdef NPP_SET_TZ
            if ( REQ("npp_set_tz") )    /* set time zone for the session */
            {
                if ( IS_SESSION && !SESSION.tz_set )
                    npp_set_tz(ci);
            }
            else
#endif  /* NPP_SET_TZ */

                npp_app_main(ci);         /* main application called here */
        }
    }

    /* ------------------------------------------------------------------------ */

    G_connections[ci].last_activity = G_now;
    if ( IS_SESSION ) SESSION.last_activity = G_now;

#ifdef NPP_ASYNC
    if ( G_connections[ci].conn_state == CONN_STATE_WAITING_FOR_ASYNC ) return;
#endif
    /* ------------------------------------------------------------------------ */

    if ( G_connections[ci].location[0] || ret == ERR_REDIRECTION )    /* redirection has a priority */
        G_connections[ci].status = 303;
    else if ( ret == ERR_INVALID_REQUEST )
        G_connections[ci].status = 400;
    else if ( ret == ERR_UNAUTHORIZED )
        G_connections[ci].status = 401;
    else if ( ret == ERR_FORBIDDEN )
        G_connections[ci].status = 403;
    else if ( ret == ERR_NOT_FOUND )
        G_connections[ci].status = 404;
    else if ( ret == ERR_INT_SERVER_ERROR )
        G_connections[ci].status = 500;
    else if ( ret == ERR_SERVER_TOOBUSY )
        G_connections[ci].status = 503;

    if ( ret==ERR_REDIRECTION || G_connections[ci].status==400 || G_connections[ci].status==401 || G_connections[ci].status==403 || G_connections[ci].status==404 || G_connections[ci].status==500 || G_connections[ci].status==503 )
    {
        if ( fresh_session )    /* prevent sessions' exhaustion by vulnerability testing bots */
        {
#ifdef NPP_USERS
            if ( !LOGGED ) close_uses(G_connections[ci].si, ci);
#else
            close_uses(G_connections[ci].si, ci);
#endif  /* NPP_USERS */
        }

        if ( !NPP_CONN_IS_KEEP_CONTENT(G_connections[ci].flags) )   /* reset out buffer pointer as it could have contained something already */
        {
            G_connections[ci].p_content = G_connections[ci].out_data + NPP_OUT_HEADER_BUFSIZE;

            if ( ret == OK )   /* RES_STATUS could be used, show the proper message */
            {
                if ( G_connections[ci].status == 400 )
                    ret = ERR_INVALID_REQUEST;
                else if ( G_connections[ci].status == 401 )
                    ret = ERR_UNAUTHORIZED;
                else if ( G_connections[ci].status == 403 )
                    ret = ERR_FORBIDDEN;
                else if ( G_connections[ci].status == 404 )
                    ret = ERR_NOT_FOUND;
                else if ( G_connections[ci].status == 500 )
                    ret = ERR_INT_SERVER_ERROR;
                else if ( G_connections[ci].status == 503 )
                    ret = ERR_SERVER_TOOBUSY;
            }

            render_page_msg(ci, ret);
        }

        RES_DONT_CACHE;
    }
}


#ifndef _WIN32
/* --------------------------------------------------------------------------
   Compress src into dest, return dest length
-------------------------------------------------------------------------- */
static unsigned deflate_data(unsigned char *dest, const unsigned char *src, unsigned src_len)
{
    DBG("src_len = %u", src_len);

    z_stream strm={0};

    if ( deflateInit(&strm, Z_DEFAULT_COMPRESSION) != Z_OK )
    {
        ERR("deflateInit failed");
        return -1;
    }

    strm.next_in = (unsigned char*)src;
    strm.avail_in = src_len;
    strm.next_out = dest;
    strm.avail_out = src_len;

    int ret=Z_OK;

//    while ( ret == Z_OK )
//    {
//        strm.avail_out = strm.avail_in ? strm.next_in-strm.next_out : (dest+src_len) - strm.next_out;
        ret = deflate(&strm, Z_FINISH);
//    }

    if ( ret != Z_STREAM_END )
    {
        ERR("ret != Z_STREAM_END");
        return -1;
    }

    if ( strm.total_out > src_len )
    {
        ERR("strm.total_out > src_len");
        return -1;
    }

    unsigned new_len = strm.total_out;

    deflateEnd(&strm);

    if ( ret != Z_STREAM_END )
    {
        ERR("ret != Z_STREAM_END");
        return -1;
    }

    DBG("new_len = %u", new_len);

    return new_len;
}


/* --------------------------------------------------------------------------
   Straight from Mark Adler!

   Compress buf[0..len-1] in place into buf[0..*max-1].  *max must be greater
   than or equal to len.  Return Z_OK on success, Z_BUF_ERROR if *max is not
   enough output space, Z_MEM_ERROR if there is not enough memory, or
   Z_STREAM_ERROR if *strm is corrupted (e.g. if it wasn't initialized or if it
   was inadvertently written over).  If Z_OK is returned, *max is set to the
   actual size of the output.  If Z_BUF_ERROR is returned, then *max is
   unchanged and buf[] is filled with *max bytes of uncompressed data (which is
   not all of it, but as much as would fit).

   Incompressible data will require more output space than len, so max should
   be sufficiently greater than len to handle that case in order to avoid a
   Z_BUF_ERROR. To assure that there is enough output space, max should be
   greater than or equal to the result of deflateBound(strm, len).

   strm is a deflate stream structure that has already been successfully
   initialized by deflateInit() or deflateInit2().  That structure can be
   reused across multiple calls to deflate_inplace().  This avoids unnecessary
   memory allocations and deallocations from the repeated use of deflateInit()
   and deflateEnd().
-------------------------------------------------------------------------- */
static int deflate_inplace(z_stream *strm, unsigned char *buf, unsigned len, unsigned *max)
{
    int ret;                    /* return code from deflate functions */
    unsigned have;              /* number of bytes in temp[] */
    unsigned char *hold;        /* allocated buffer to hold input data */
    unsigned char temp[11];     /* must be large enough to hold zlib or gzip
                                   header (if any) and one more byte -- 11
                                   works for the worst case here, but if gzip
                                   encoding is used and a deflateSetHeader()
                                   call is inserted in this code after the
                                   deflateReset(), then the 11 needs to be
                                   increased to accomodate the resulting gzip
                                   header size plus one */

    /* initialize deflate stream and point to the input data */
    ret = deflateReset(strm);
    if (ret != Z_OK)
        return ret;
    strm->next_in = buf;
    strm->avail_in = len;

    /* kick start the process with a temporary output buffer -- this allows
       deflate to consume a large chunk of input data in order to make room for
       output data there */
    if (*max < len)
        *max = len;
    strm->next_out = temp;
    strm->avail_out = sizeof(temp) > *max ? *max : sizeof(temp);
    ret = deflate(strm, Z_FINISH);
    if (ret == Z_STREAM_ERROR)
        return ret;

    /* if we can, copy the temporary output data to the consumed portion of the
       input buffer, and then continue to write up to the start of the consumed
       input for as long as possible */
    have = strm->next_out - temp;
    if (have <= (strm->avail_in ? len - strm->avail_in : *max)) {
        memcpy(buf, temp, have);
        strm->next_out = buf + have;
        have = 0;
        while (ret == Z_OK) {
            strm->avail_out = strm->avail_in ? strm->next_in - strm->next_out :
                                               (buf + *max) - strm->next_out;
            ret = deflate(strm, Z_FINISH);
        }
        if (ret != Z_BUF_ERROR || strm->avail_in == 0) {
            *max = strm->next_out - buf;
            return ret == Z_STREAM_END ? Z_OK : ret;
        }
    }
    /* the output caught up with the input due to insufficiently compressible
       data -- copy the remaining input data into an allocated buffer and
       complete the compression from there to the now empty input buffer (this
       will only occur for long incompressible streams, more than ~20 MB for
       the default deflate memLevel of 8, or when *max is too small and less
       than the length of the header plus one byte) */
    hold = (unsigned char*)strm->zalloc(strm->opaque, strm->avail_in, 1);
    if (hold == Z_NULL)
        return Z_MEM_ERROR;
    memcpy(hold, strm->next_in, strm->avail_in);
    strm->next_in = hold;
    if (have) {
        memcpy(buf, temp, have);
        strm->next_out = buf + have;
    }
    strm->avail_out = (buf + *max) - strm->next_out;
    ret = deflate(strm, Z_FINISH);
    strm->zfree(strm->opaque, hold);
    *max = strm->next_out - buf;
    return ret == Z_OK ? Z_BUF_ERROR : (ret == Z_STREAM_END ? Z_OK : ret);
}
#endif  /* _WIN32 */


/* --------------------------------------------------------------------------
   Generate HTTP response header
-------------------------------------------------------------------------- */
static void gen_response_header(int ci)
{
#ifdef NPP_DEBUG
    bool compressed=FALSE;
#endif

    DBG("gen_response_header, ci=%d", ci);

    char out_header[NPP_OUT_HEADER_BUFSIZE];
    G_connections[ci].p_header = out_header;

#ifdef NPP_HTTP2

    if ( G_connections[ci].http2_upgrade_in_progress && G_connections[ci].status == 200 )   /* upgrade to HTTP/2 cleartext requested (rare) */
    {
        DDBG("Responding with 101");

        PRINT_HTTP_STATUS(101);

        PRINT_HTTP2_UPGRADE_CLEAR;
    }
    else    /* normal response */
    {
        G_connections[ci].http2_upgrade_in_progress = FALSE;

        if ( G_connections[ci].http_ver[0] == '2' )
            PRINT_HTTP2_STATUS(G_connections[ci].status);
        else

#endif  /* NPP_HTTP2 */

            PRINT_HTTP_STATUS(G_connections[ci].status);

        /* Date */

#ifdef NPP_HTTP2
        if ( G_connections[ci].http_ver[0] == '2' )
            PRINT_HTTP2_DATE;
        else
#endif  /* NPP_HTTP2 */
            PRINT_HTTP_DATE;

        if ( G_connections[ci].status == 301 || G_connections[ci].status == 303 )     /* redirection */
        {
            DDBG("Redirecting");

#ifdef NPP_HTTPS
            if ( NPP_CONN_IS_UPGRADE_TO_HTTPS(G_connections[ci].flags) )    /* Upgrade-Insecure-Requests */
            {
#ifdef NPP_HTTP2
                if ( G_connections[ci].http_ver[0] == '2' )
                    PRINT_HTTP2_VARY_UIR;
                else
#endif  /* NPP_HTTP2 */
                    PRINT_HTTP_VARY_UIR;
            }
#endif  /* NPP_HTTPS */

            if ( G_connections[ci].location[0] )    /* redirection */
            {
#ifdef NPP_HTTP2
                if ( G_connections[ci].http_ver[0] == '2' )
                    PRINT_HTTP2_LOCATION;
                else
#endif  /* NPP_HTTP2 */
                    PRINT_HTTP_LOCATION;
            }

            G_connections[ci].clen = 0;
        }
        else if ( G_connections[ci].status == 304 )   /* not modified since */
        {
            DDBG("Not Modified");

            /*
               Since the goal of a 304 response is to minimize information transfer
               when the recipient already has one or more cached representations,
               a sender SHOULD NOT generate representation metadata other than
               the above listed fields unless said metadata exists for the purpose
               of guiding cache updates (e.g., Last-Modified might be useful if
               the response does not have an ETag field).
            */

            if ( G_connections[ci].static_res == NPP_NOT_STATIC )    /* generated */
            {
                if ( G_connections[ci].modified )
                {
#ifdef NPP_HTTP2
                    if ( G_connections[ci].http_ver[0] == '2' )
                        PRINT_HTTP2_LAST_MODIFIED(time_epoch2http(G_connections[ci].modified));
                    else
#endif  /* NPP_HTTP2 */
                        PRINT_HTTP_LAST_MODIFIED(time_epoch2http(G_connections[ci].modified));
                }
                else
                {
#ifdef NPP_HTTP2
                    if ( G_connections[ci].http_ver[0] == '2' )
                        PRINT_HTTP2_LAST_MODIFIED(G_last_modified);
                    else
#endif  /* NPP_HTTP2 */
                        PRINT_HTTP_LAST_MODIFIED(G_last_modified);
                }

                if ( NPP_EXPIRES_GENERATED > 0 )
                {
#ifdef NPP_HTTP2
                    if ( G_connections[ci].http_ver[0] == '2' )
                        PRINT_HTTP2_EXPIRES_GENERATED;
                    else
#endif  /* NPP_HTTP2 */
                        PRINT_HTTP_EXPIRES_GENERATED;
                }
            }
            else    /* static resource */
            {
#ifdef NPP_HTTP2
                if ( G_connections[ci].http_ver[0] == '2' )
                    PRINT_HTTP2_LAST_MODIFIED(time_epoch2http(M_stat[G_connections[ci].static_res].modified));
                else
#endif  /* NPP_HTTP2 */
                    PRINT_HTTP_LAST_MODIFIED(time_epoch2http(M_stat[G_connections[ci].static_res].modified));

                if ( NPP_EXPIRES_STATICS > 0 )
                {
#ifdef NPP_HTTP2
                    if ( G_connections[ci].http_ver[0] == '2' )
                        PRINT_HTTP2_EXPIRES_STATICS;
                    else
#endif  /* NPP_HTTP2 */
                        PRINT_HTTP_EXPIRES_STATICS;
                }
            }

            G_connections[ci].clen = 0;
        }
        else    /* normal response with content: 2xx, 4xx, 5xx */
        {
            DDBG("Normal response");

            if ( NPP_CONN_IS_DONT_CACHE(G_connections[ci].flags) )    /* dynamic content */
            {
#ifdef NPP_HTTP2
                if ( G_connections[ci].http_ver[0] == '2' )
                {
                    PRINT_HTTP2_VARY_DYN;
                    PRINT_HTTP2_NO_CACHE;
                }
                else
#endif  /* NPP_HTTP2 */
                {
                    PRINT_HTTP_VARY_DYN;
                    PRINT_HTTP_NO_CACHE;
                }
            }
            else    /* static content */
            {
#ifdef NPP_HTTP2
                if ( G_connections[ci].http_ver[0] == '2' )
                    PRINT_HTTP2_VARY_STAT;
                else
#endif  /* NPP_HTTP2 */
                    PRINT_HTTP_VARY_STAT;

                if ( G_connections[ci].static_res == NPP_NOT_STATIC )   /* generated -- moderate caching */
                {
                    if ( G_connections[ci].modified )
                    {
#ifdef NPP_HTTP2
                        if ( G_connections[ci].http_ver[0] == '2' )
                            PRINT_HTTP2_LAST_MODIFIED(time_epoch2http(G_connections[ci].modified));
                        else
#endif  /* NPP_HTTP2 */
                            PRINT_HTTP_LAST_MODIFIED(time_epoch2http(G_connections[ci].modified));
                    }
                    else
                    {
#ifdef NPP_HTTP2
                        if ( G_connections[ci].http_ver[0] == '2' )
                            PRINT_HTTP2_LAST_MODIFIED(G_last_modified);
                        else
#endif  /* NPP_HTTP2 */
                            PRINT_HTTP_LAST_MODIFIED(G_last_modified);
                    }

                    if ( NPP_EXPIRES_GENERATED > 0 )
                    {
#ifdef NPP_HTTP2
                        if ( G_connections[ci].http_ver[0] == '2' )
                            PRINT_HTTP2_EXPIRES_GENERATED;
                        else
#endif  /* NPP_HTTP2 */
                            PRINT_HTTP_EXPIRES_GENERATED;
                    }
                }
                else    /* static resource -- aggressive caching */
                {
#ifdef NPP_HTTP2
                    if ( G_connections[ci].http_ver[0] == '2' )
                        PRINT_HTTP2_LAST_MODIFIED(time_epoch2http(M_stat[G_connections[ci].static_res].modified));
                    else
#endif  /* NPP_HTTP2 */
                        PRINT_HTTP_LAST_MODIFIED(time_epoch2http(M_stat[G_connections[ci].static_res].modified));

                    if ( NPP_EXPIRES_STATICS > 0 )
                    {
#ifdef NPP_HTTP2
                        if ( G_connections[ci].http_ver[0] == '2' )
                            PRINT_HTTP2_EXPIRES_STATICS;
                        else
#endif  /* NPP_HTTP2 */
                            PRINT_HTTP_EXPIRES_STATICS;
                    }
                }
            }

            /* content length and type */

            if ( G_connections[ci].static_res == NPP_NOT_STATIC )
            {
                G_connections[ci].clen = G_connections[ci].p_content - G_connections[ci].out_data - NPP_OUT_HEADER_BUFSIZE;
            }
            else    /* static resource */
            {
                G_connections[ci].clen = M_stat[G_connections[ci].static_res].len;
                G_connections[ci].ctype = M_stat[G_connections[ci].static_res].type;
            }

            /* compress? ------------------------------------------------------------------ */

#ifndef _WIN32  /* in Windows it's just too much headache */

            if ( SHOULD_BE_COMPRESSED(G_connections[ci].clen, G_connections[ci].ctype) && NPP_CONN_IS_ACCEPT_DEFLATE(G_connections[ci].flags) && !NPP_UA_IE )
            {
                if ( G_connections[ci].static_res==NPP_NOT_STATIC )
                {
                    DBG("Compressing content");

                    int ret;
static              z_stream strm;
static              bool first=TRUE;

                    if ( first )
                    {
                        strm.zalloc = Z_NULL;
                        strm.zfree = Z_NULL;
                        strm.opaque = Z_NULL;

                        ret = deflateInit(&strm, NPP_COMPRESS_LEVEL);

                        if ( ret != Z_OK )
                        {
                            ERR("deflateInit failed, ret = %d", ret);
                            return;
                        }

                        first = FALSE;
                    }

                    unsigned max = G_connections[ci].clen;

                    ret = deflate_inplace(&strm, (unsigned char*)G_connections[ci].out_data+NPP_OUT_HEADER_BUFSIZE, G_connections[ci].clen, &max);

                    if ( ret == Z_OK )
                    {
                        DBG("Compression success, old len=%u, new len=%u", G_connections[ci].clen, max);
                        G_connections[ci].clen = max;
#ifdef NPP_HTTP2
                        if ( G_connections[ci].http_ver[0] == '2' )
                            PRINT_HTTP2_CONTENT_ENCODING_DEFLATE;
                        else
#endif  /* NPP_HTTP2 */
                            PRINT_HTTP_CONTENT_ENCODING_DEFLATE;
#ifdef NPP_DEBUG
                        compressed = TRUE;
#endif
                    }
                    else
                    {
                        ERR("deflate_inplace failed, ret = %d", ret);
                    }
                }
                else if ( M_stat[G_connections[ci].static_res].len_deflated )   /* compressed static resource is available */
                {
                    G_connections[ci].out_data = M_stat[G_connections[ci].static_res].data_deflated;
                    G_connections[ci].clen = M_stat[G_connections[ci].static_res].len_deflated;
#ifdef NPP_HTTP2
                    if ( G_connections[ci].http_ver[0] == '2' )
                        PRINT_HTTP2_CONTENT_ENCODING_DEFLATE;
                    else
#endif  /* NPP_HTTP2 */
                        PRINT_HTTP_CONTENT_ENCODING_DEFLATE;
#ifdef NPP_DEBUG
                    compressed = TRUE;
#endif
                }
            }
#endif  /* _WIN32 */

            /* ---------------------------------------------------------------------------- */
        }

        /* Content-Type */

        if ( G_connections[ci].clen == 0 )   /* don't set for these */
        {                   /* this covers 301, 303 and 304 */
        }
        else if ( G_connections[ci].ctypestr[0] )    /* custom */
        {
#ifdef NPP_HTTP2
            if ( G_connections[ci].http_ver[0] == '2' )
                PRINT_HTTP2_CONTENT_TYPE(G_connections[ci].ctypestr);
            else
#endif
                PRINT_HTTP_CONTENT_TYPE(G_connections[ci].ctypestr);
        }
        else if ( G_connections[ci].ctype != NPP_CONTENT_TYPE_UNSET )
        {
            print_content_type(ci, G_connections[ci].ctype);
        }

        /* Content-Disposition */

        if ( G_connections[ci].cdisp[0] )
        {
#ifdef NPP_HTTP2
            if ( G_connections[ci].http_ver[0] == '2' )
                PRINT_HTTP2_CONTENT_DISP(G_connections[ci].cdisp);
            else
#endif
                PRINT_HTTP_CONTENT_DISP(G_connections[ci].cdisp);
        }

        /* Content-Length */

#ifdef NPP_HTTP2
        if ( G_connections[ci].http_ver[0] == '2' )
            PRINT_HTTP2_CONTENT_LEN(G_connections[ci].clen);
        else
#endif  /* NPP_HTTP2 */
            PRINT_HTTP_CONTENT_LEN(G_connections[ci].clen);

        /* Security */

        if ( G_connections[ci].clen > 0 )
        {
#ifndef NPP_NO_SAMEORIGIN
#ifdef NPP_HTTP2
            if ( G_connections[ci].http_ver[0] == '2' )
                PRINT_HTTP2_SAMEORIGIN;
            else
#endif  /* NPP_HTTP2 */
                PRINT_HTTP_SAMEORIGIN;
#endif  /* NPP_NO_SAMEORIGIN */

#ifndef NPP_NO_NOSNIFF
#ifdef NPP_HTTP2
            if ( G_connections[ci].http_ver[0] == '2' )
                PRINT_HTTP2_NOSNIFF;
            else
#endif  /* NPP_HTTP2 */
                PRINT_HTTP_NOSNIFF;
#endif  /* NPP_NO_NOSNIFF */
        }

        /* Connection */

#ifdef NPP_HTTP2
        if ( G_connections[ci].http_ver[0] != '2' )
#endif  /* NPP_HTTP2 */
            PRINT_HTTP_CONNECTION;

        /* Cookie */

        if ( G_connections[ci].static_res == NPP_NOT_STATIC && (G_connections[ci].status == 200 || G_connections[ci].status == 303) && G_connections[ci].method[0]!='H' )
        {
            if ( G_connections[ci].cookie_out_l[0] )         /* logged in cookie has been produced */
            {
                if ( G_connections[ci].cookie_out_l_exp[0] )
                {
#ifdef NPP_HTTP2
                    if ( G_connections[ci].http_ver[0] == '2' )
                        PRINT_HTTP2_COOKIE_L_EXP(ci);    /* with expiration date */
                    else
#endif  /* NPP_HTTP2 */
                        PRINT_HTTP_COOKIE_L_EXP(ci);
                }
                else
                {
#ifdef NPP_HTTP2
                    if ( G_connections[ci].http_ver[0] == '2' )
                        PRINT_HTTP2_COOKIE_L(ci);
                    else
#endif  /* NPP_HTTP2 */
                        PRINT_HTTP_COOKIE_L(ci);
                }
            }

            if ( G_connections[ci].cookie_out_a[0] )         /* anonymous cookie has been produced */
            {
                if ( G_connections[ci].cookie_out_a_exp[0] )
                {
#ifdef NPP_HTTP2
                    if ( G_connections[ci].http_ver[0] == '2' )
                        PRINT_HTTP2_COOKIE_A_EXP(ci);    /* with expiration date */
                    else
#endif  /* NPP_HTTP2 */
                        PRINT_HTTP_COOKIE_A_EXP(ci);
                }
                else
                {
#ifdef NPP_HTTP2
                    if ( G_connections[ci].http_ver[0] == '2' )
                        PRINT_HTTP2_COOKIE_A(ci);
                    else
#endif  /* NPP_HTTP2 */
                        PRINT_HTTP_COOKIE_A(ci);
                }
            }
        }

#ifdef NPP_HTTPS
#ifndef NPP_NO_HSTS
        if ( NPP_CONN_IS_SECURE(G_connections[ci].flags) )
            PRINT_HTTP_HSTS;
#endif
#endif  /* NPP_HTTPS */

#ifndef NPP_NO_IDENTITY
#ifdef NPP_HTTP2
        if ( G_connections[ci].http_ver[0] == '2' )
            PRINT_HTTP2_SERVER;
        else
#endif  /* NPP_HTTP2 */
            PRINT_HTTP_SERVER;
#endif  /* NPP_NO_IDENTITY */

        /* ------------------------------------------------------------- */
        /* custom headers */

        if ( G_connections[ci].cust_headers[0] )
        {
            HOUT(G_connections[ci].cust_headers);
        }

        /* ------------------------------------------------------------- */

#ifdef NPP_HTTP2
    }
#endif

#ifdef NPP_HTTP2
    if ( G_connections[ci].http_ver[0] != '2' )
#endif
        PRINT_HTTP_END_OF_HEADER;

    /* header length */

    G_connections[ci].out_hlen = G_connections[ci].p_header - out_header;

    DDBG("ci=%d, out_hlen = %u", ci, G_connections[ci].out_hlen);

    DBG("Response status: %d", G_connections[ci].status);

    DDBG("ci=%d, Changing state to CONN_STATE_READY_TO_SEND_RESPONSE", ci);
    G_connections[ci].conn_state = CONN_STATE_READY_TO_SEND_RESPONSE;

#ifdef NPP_FD_MON_POLL
    M_pollfds[G_connections[ci].pi].events = POLLOUT;
#endif

#ifdef NPP_HTTP2
    if ( G_connections[ci].http_ver[0] != '2' )
#endif
#if NPP_OUT_HEADER_BUFSIZE-1 <= NPP_MAX_LOG_STR_LEN
        DBG("\nResponse header:\n\n[%s]\n", out_header);
#else
        log_long(out_header, G_connections[ci].out_hlen, "\nResponse header");
#endif  /* NPP_OUT_HEADER_BUFSIZE-1 <= NPP_MAX_LOG_STR_LEN */

    /* ----------------------------------------------------------------- */
    /* try to send everything at once */
    /* copy response header just before the content */

    G_connections[ci].out_start = G_connections[ci].out_data + (NPP_OUT_HEADER_BUFSIZE - G_connections[ci].out_hlen);
    memcpy(G_connections[ci].out_start, out_header, G_connections[ci].out_hlen);
    G_connections[ci].out_len = G_connections[ci].out_hlen + G_connections[ci].clen;

    /* ----------------------------------------------------------------- */

#ifdef NPP_DEBUG     /* low-level tests */
    if ( G_logLevel>=LOG_DBG
            && G_connections[ci].clen > 0
            && G_connections[ci].method[0]!='H'
            && G_connections[ci].static_res==NPP_NOT_STATIC
            && !compressed
            && (G_connections[ci].ctype==NPP_CONTENT_TYPE_TEXT || G_connections[ci].ctype==NPP_CONTENT_TYPE_JSON) )
        log_long(G_connections[ci].out_data+NPP_OUT_HEADER_BUFSIZE, G_connections[ci].clen, "Content to send");
#endif  /* NPP_DEBUG */

    /* ----------------------------------------------------------------- */

    G_connections[ci].last_activity = G_now;
    if ( IS_SESSION ) SESSION.last_activity = G_now;

    log_proc_time(ci);
}


/* --------------------------------------------------------------------------
   Print Content-Type to response header
   Mirrored npp_lib_set_res_content_type
-------------------------------------------------------------------------- */
static void print_content_type(int ci, char type)
{
    char http_type[256];

    if ( type == NPP_CONTENT_TYPE_HTML )
        strcpy(http_type, "text/html; charset=utf-8");
    else if ( type == NPP_CONTENT_TYPE_CSS )
        strcpy(http_type, "text/css");
    else if ( type == NPP_CONTENT_TYPE_JS )
        strcpy(http_type, "application/javascript");
    else if ( type == NPP_CONTENT_TYPE_GIF )
        strcpy(http_type, "image/gif");
    else if ( type == NPP_CONTENT_TYPE_JPG )
        strcpy(http_type, "image/jpeg");
    else if ( type == NPP_CONTENT_TYPE_ICO )
        strcpy(http_type, "image/x-icon");
    else if ( type == NPP_CONTENT_TYPE_PNG )
        strcpy(http_type, "image/png");
    else if ( type == NPP_CONTENT_TYPE_BMP )
        strcpy(http_type, "image/bmp");
    else if ( type == NPP_CONTENT_TYPE_SVG )
        strcpy(http_type, "image/svg+xml");
    else if ( type == NPP_CONTENT_TYPE_JSON )
        strcpy(http_type, "application/json");
    else if ( type == NPP_CONTENT_TYPE_PDF )
        strcpy(http_type, "application/pdf");
    else if ( type == NPP_CONTENT_TYPE_AMPEG )
        strcpy(http_type, "audio/mpeg");
    else if ( type == NPP_CONTENT_TYPE_EXE )
        strcpy(http_type, "application/x-msdownload");
    else if ( type == NPP_CONTENT_TYPE_ZIP )
        strcpy(http_type, "application/zip");
    else
        strcpy(http_type, "text/plain");

#ifdef NPP_HTTP2
    if ( G_connections[ci].http_ver[0] == '2' )
        PRINT_HTTP2_CONTENT_TYPE(http_type);
    else
#endif  /* NPP_HTTP2 */
        PRINT_HTTP_CONTENT_TYPE(http_type);
}


/* --------------------------------------------------------------------------
   Verify IP & User-Agent against sesid in G_sessions (anonymous users)
   Return true if session exists
-------------------------------------------------------------------------- */
static bool a_usession_ok(int ci)
{
    DBG("a_usession_ok");

    if ( IS_SESSION )    /* existing connection */
    {
        if ( G_sessions[G_connections[ci].si].sesid[0]
                && G_sessions[G_connections[ci].si].auth_level<AUTH_LEVEL_AUTHENTICATED
                && 0==strcmp(G_connections[ci].cookie_in_a, G_sessions[G_connections[ci].si].sesid)
                && 0==strcmp(G_connections[ci].uagent, G_sessions[G_connections[ci].si].uagent) )
        {
            DBG("Anonymous session found, si=%d, sesid [%s]", G_connections[ci].si, G_sessions[G_connections[ci].si].sesid);
            return TRUE;
        }
        else    /* session was closed -- it should never happen */
        {
            WAR("si > 0 and no session!");
            G_connections[ci].si = 0;
        }
    }
    else    /* fresh connection */
    {
        int i;

        for ( i=1; i<=NPP_MAX_SESSIONS; ++i )
        {
            if ( G_sessions[i].sesid[0]
                    && G_sessions[i].auth_level<AUTH_LEVEL_AUTHENTICATED
                    && 0==strcmp(G_connections[ci].cookie_in_a, G_sessions[i].sesid)
                    && 0==strcmp(G_connections[ci].uagent, G_sessions[i].uagent) )
            {
                DBG("Anonymous session found, si=%d, sesid [%s]", i, G_sessions[i].sesid);
                G_connections[ci].si = i;
                return TRUE;
            }
        }
    }

    /* not found */
    return FALSE;
}


/* --------------------------------------------------------------------------
   Close timeouted connections
-------------------------------------------------------------------------- */
static void close_old_conn()
{
    int     i;
    time_t  last_allowed;

    last_allowed = G_now - NPP_CONNECTION_TIMEOUT;

    for ( i=0; G_connections_cnt>0 && i<NPP_MAX_CONNECTIONS; ++i )
    {
        if ( G_connections[i].conn_state != CONN_STATE_DISCONNECTED && G_connections[i].last_activity < last_allowed )
        {
            DBG("Closing timeouted connection %d", i);
            close_conn(i);
        }
    }
}


/* --------------------------------------------------------------------------
   Close timeouted anonymous user sessions
-------------------------------------------------------------------------- */
static void uses_close_timeouted()
{
    int     i;
    time_t  last_allowed;

    last_allowed = G_now - NPP_SESSION_TIMEOUT;

    for ( i=1; G_sessions_cnt>0 && i<=NPP_MAX_SESSIONS; ++i )
    {
        if ( G_sessions[i].sesid[0] && G_sessions[i].auth_level<AUTH_LEVEL_AUTHENTICATED && G_sessions[i].last_activity < last_allowed )
            close_uses(i, NPP_NOT_CONNECTED);
    }
}


/* --------------------------------------------------------------------------
   Close anonymous user session
-------------------------------------------------------------------------- */
static void close_uses(int si, int ci)
{
    DBG("Closing anonymous session, si=%d, sesid [%s]", si, G_sessions[si].sesid);

    if ( ci != NPP_NOT_CONNECTED )   /* still connected */
        npp_app_session_done(ci);
    else    /* trick to maintain consistency across npp_app_xxx functions */
    {       /* that use ci for everything -- even to get user session data */
        G_connections[CLOSING_SESSION_CI].si = si;
        npp_app_session_done(CLOSING_SESSION_CI);
    }

    /* reset session data */

    memset(&G_sessions[si], 0, sizeof(eng_session_data_t));
    memset(&G_app_session_data[si], 0, sizeof(app_session_data_t));

    G_sessions_cnt--;

    if ( ci != NPP_NOT_CONNECTED )   /* still connected */
        G_connections[ci].si = 0;

    DBG("%d session(s) remaining", G_sessions_cnt);
}


/* --------------------------------------------------------------------------
   Reset connection after processing request
-------------------------------------------------------------------------- */
static void reset_conn(int ci, char new_state)
{
#ifdef NPP_DEBUG
    if ( G_initialized )
        DBG("Resetting connection ci=%d, fd=%d, new state == %s\n", ci, G_connections[ci].fd, new_state==CONN_STATE_CONNECTED?"CONN_STATE_CONNECTED":"CONN_STATE_DISCONNECTED");
#endif

    G_connections[ci].conn_state = new_state;

    G_connections[ci].status = 200;
    G_connections[ci].method[0] = EOS;
//    G_connections[ci].head_only = FALSE;
//    G_connections[ci].post = FALSE;
    if ( G_connections[ci].in_data )
    {
        free(G_connections[ci].in_data);
        G_connections[ci].in_data = NULL;
    }
    G_connections[ci].was_read = 0;
//    G_connections[ci].upgrade2https = FALSE;
    G_connections[ci].resource[0] = EOS;
#if NPP_RESOURCE_LEVELS > 1
    G_connections[ci].req1[0] = EOS;
#if NPP_RESOURCE_LEVELS > 2
    G_connections[ci].req2[0] = EOS;
#if NPP_RESOURCE_LEVELS > 3
    G_connections[ci].req3[0] = EOS;
#if NPP_RESOURCE_LEVELS > 4
    G_connections[ci].req4[0] = EOS;
#if NPP_RESOURCE_LEVELS > 5
    G_connections[ci].req5[0] = EOS;
#endif  /* NPP_RESOURCE_LEVELS > 5 */
#endif  /* NPP_RESOURCE_LEVELS > 4 */
#endif  /* NPP_RESOURCE_LEVELS > 3 */
#endif  /* NPP_RESOURCE_LEVELS > 2 */
#endif  /* NPP_RESOURCE_LEVELS > 1 */
    G_connections[ci].id[0] = EOS;
//    G_connections[ci].http_ver[0] = EOS;
#ifdef NPP_HTTP2
//    G_connections[ci].http2_settings[0] = EOS;
#endif  /* NPP_HTTP2 */
    G_connections[ci].uagent[0] = EOS;
    G_connections[ci].ua_type = NPP_UA_TYPE_DSK;
//    G_connections[ci].keep_alive = FALSE;
    G_connections[ci].referer[0] = EOS;
    G_connections[ci].clen = 0;
    G_connections[ci].in_cookie[0] = EOS;
    G_connections[ci].cookie_in_a[0] = EOS;
    G_connections[ci].cookie_in_l[0] = EOS;
    G_connections[ci].host[0] = EOS;
    strcpy(G_connections[ci].app_name, NPP_APP_NAME);
    G_connections[ci].lang[0] = EOS;
    G_connections[ci].formats = (char)0;
    G_connections[ci].if_mod_since = 0;
    G_connections[ci].in_ctypestr[0] = EOS;
    G_connections[ci].in_ctype = NPP_CONTENT_TYPE_UNSET;
    G_connections[ci].boundary[0] = EOS;
    G_connections[ci].authorization[0] = EOS;
    G_connections[ci].required_auth_level = NPP_REQUIRED_AUTH_LEVEL;

    /* what goes out */

    G_connections[ci].cust_headers[0] = EOS;
    G_connections[ci].cust_headers_len = 0;
    G_connections[ci].data_sent = 0;

    G_connections[ci].out_data = G_connections[ci].out_data_alloc;

    /* don't reset session id for the entire connection life */
    /* this also means that authenticated connection stays this way until closed or logged out */

    if ( new_state == CONN_STATE_DISCONNECTED )
    {
        G_connections[ci].si = 0;

#ifdef NPP_HTTP2
        G_connections[ci].http2_upgrade_in_progress = FALSE;
        G_connections[ci].http2_last_stream_id = 1;
#endif  /* NPP_HTTP2 */
    }

    G_connections[ci].static_res = NPP_NOT_STATIC;
    G_connections[ci].ctype = NPP_CONTENT_TYPE_HTML;
    G_connections[ci].ctypestr[0] = EOS;
    G_connections[ci].cdisp[0] = EOS;
    G_connections[ci].modified = 0;
    G_connections[ci].cookie_out_a[0] = EOS;
    G_connections[ci].cookie_out_a_exp[0] = EOS;
    G_connections[ci].cookie_out_l[0] = EOS;
    G_connections[ci].cookie_out_l_exp[0] = EOS;
    G_connections[ci].location[0] = EOS;
//    REQ_BOT = FALSE;
//    G_connections[ci].dont_cache = FALSE;
//    G_connections[ci].keep_content = FALSE;
//    G_connections[ci].accept_deflate = FALSE;
#ifdef NPP_MULTI_HOST
    G_connections[ci].host_id = 0;
#endif

#ifdef NPP_ASYNC
    G_connections[ci].service[0] = EOS;
    G_connections[ci].async_err_code = OK;
#endif

    if ( new_state == CONN_STATE_CONNECTED )
    {
#ifdef NPP_FD_MON_POLL
        M_pollfds[G_connections[ci].pi].events = POLLIN;
#endif
        G_connections[ci].last_activity = G_now;
        if ( IS_SESSION ) SESSION.last_activity = G_now;
    }

    G_connections[ci].flags = (char)0;
    G_connections[ci].expect100 = FALSE;
}


/* --------------------------------------------------------------------------
   Parse HTTP request
   Return HTTP status code
-------------------------------------------------------------------------- */
static int parse_req(int ci, int len)
{
    int  ret=200;

    /* --------------------------------------------

    Shortest valid request:

    GET / HTTP/1.1      15 including \n +
    Host: 1.1.1.1       14 including \n = 29

    -------------------------------------------- */

    DBG("parse_req, ci=%d", ci);

    G_connections[ci].req = ++G_cnts_today.req;    /* for reporting processing time at the end */

    DBG("\n------------------------------------------------\n %s  Request %u\n------------------------------------------------\n", DT_NOW_GMT, G_connections[ci].req);

//  if ( G_connections[ci].conn_state != STATE_SENDING ) /* ignore Range requests for now */
//      G_connections[ci].conn_state = STATE_RECEIVED;   /* by default */

    if ( len < 14 )  /* ignore any junk */
    {
        DBG("request len < 14, ignoring");
        return 400;   /* Bad Request */
    }

    /* look for end of header */

    char *p_hend = strstr(G_connections[ci].in, "\r\n\r\n");

    if ( !p_hend )
    {
        p_hend = strstr(G_connections[ci].in, "\n\n");

        if ( !p_hend )
        {
            if ( 0 == strncmp(G_connections[ci].in, "GET / HTTP/1.", 13) )   /* temporary solution for good looking partial requests */
            {
                strcat(G_connections[ci].in, "\n");  /* for values reading algorithm */
                p_hend = G_connections[ci].in + len;
            }
            else
            {
                DBG("Request syntax error, ignoring");
                return 400;  /* Bad Request */
            }
        }
    }

    int hlen = p_hend - G_connections[ci].in;    /* HTTP header length including first of the last new line characters to simplify parsing algorithm in the third 'for' loop below */

    DDBG("hlen = %d", hlen);

    log_long(G_connections[ci].in, hlen, "Incoming buffer");     /* NPP_IN_BUFSIZE > NPP_MAX_LOG_STR_LEN! */

    ++hlen;     /* HTTP header length including first of the last new line characters to simplify parsing algorithm in the third 'for' loop below */

    /* parse the header -------------------------------------------------------------------------- */

    int i;

    for ( i=0; i<hlen; ++i )    /* the first line is special -- consists of more than one token */
    {                                   /* the very first token is a request method */
        if ( isalpha(G_connections[ci].in[i]) )
        {
            if ( i < NPP_METHOD_LEN )
                G_connections[ci].method[i] = G_connections[ci].in[i];
            else
            {
                ERR("Method too long, ignoring");
                return 400;  /* Bad Request */
            }
        }
        else    /* most likely space = end of method */
        {
            G_connections[ci].method[i] = EOS;

            /* check against the list of allowed methods */

            if ( 0==strcmp(G_connections[ci].method, "GET") )
            {
                G_connections[ci].in_ctype = NPP_CONTENT_TYPE_URLENCODED;
            }
            else if ( 0==strcmp(G_connections[ci].method, "POST") || 0==strcmp(G_connections[ci].method, "PUT") || 0==strcmp(G_connections[ci].method, "DELETE") )
            {
                G_connections[ci].flags |= NPP_CONN_FLAG_PAYLOAD;
            }
            else if ( 0==strcmp(G_connections[ci].method, "OPTIONS") )
            {
                /* just go ahead */
            }
//            else if ( 0==strcmp(G_connections[ci].method, "HEAD") )
//            {
//                G_connections[ci].head_only = TRUE;  /* respond with the header only */
//            }
            else
            {
                WAR("Method [%s] not allowed, ignoring", G_connections[ci].method);
                return 405;
            }

            break;
        }
    }

    /* only for low-level tests ------------------------------------- */
//  DBG("method: [%s]", G_connections[ci].method);
    /* -------------------------------------------------------------- */

    i += 2;     /* skip " /" */
    int j=0;

    for ( i; i<hlen; ++i )   /* URI */
    {
        if ( G_connections[ci].in[i] != ' ' && G_connections[ci].in[i] != '\t' )
        {
            if ( j < NPP_MAX_URI_LEN )
                G_connections[ci].uri[j++] = G_connections[ci].in[i];
            else
            {
                WAR("URI too long, ignoring");
                return 414;  /* Request-URI Too Long */
            }
        }
        else    /* end of URI */
        {
            G_connections[ci].uri[j] = EOS;
            break;
        }
    }

    /* strip the trailing slash off */

    if ( j && G_connections[ci].uri[j-1] == '/' )
    {
        G_connections[ci].uri[j-1] = EOS;
    }

#ifdef NPP_ROOT_URI
    /*
       If i.e. NPP_ROOT_URI: "app"
       then with URL: example.com/app/something
       we want G_connections[ci].uri to be "something"

       Initial G_connections[ci].uri: app/something
       root_uri_len = 3
    */
    int root_uri_len = strlen(NPP_ROOT_URI);
    if ( 0==strncmp(G_connections[ci].uri, NPP_ROOT_URI, root_uri_len) )
    {
        char tmp[NPP_MAX_URI_LEN+1];
        strcpy(tmp, G_connections[ci].uri+root_uri_len+1);

        DDBG("tmp: [%s]", tmp);

        strcpy(G_connections[ci].uri, tmp);
    }
#endif  /* NPP_ROOT_URI */

    DDBG("URI: [%s]", G_connections[ci].uri);

    i += 6;   /* skip the space and HTTP/ */

    j = 0;
    while ( i < hlen && G_connections[ci].in[i] != '\r' && G_connections[ci].in[i] != '\n' )
    {
        if ( j < 3 )
            G_connections[ci].http_ver[j++] = G_connections[ci].in[i];
        ++i;
    }
    G_connections[ci].http_ver[j] = EOS;

    DDBG("http_ver [%s]", G_connections[ci].http_ver);

    /* -------------------------------------------------------------- */
    /* parse the rest of the header */

    char flg_data=FALSE;
    char now_label=TRUE;
    char now_value=FALSE;
    char was_cr=FALSE;
    char label[NPP_MAX_LABEL_LEN+1];
    char value[NPP_MAX_VALUE_LEN+1];

    while ( i < hlen && G_connections[ci].in[i] != '\n' ) ++i;

    j = 0;

    for ( i; i<hlen; ++i )   /* next lines */
    {
        if ( !now_value && (G_connections[ci].in[i] == ' ' || G_connections[ci].in[i] == '\t') )  /* omit whitespaces */
            continue;

        if ( G_connections[ci].in[i] == '\n' && was_cr )
        {
            was_cr = FALSE;
            continue;   /* value has already been saved in a previous loop go */
        }

        if ( G_connections[ci].in[i] == '\r' )
            was_cr = TRUE;

        if ( G_connections[ci].in[i] == '\r' || G_connections[ci].in[i] == '\n' ) /* end of value. Caution: \n only if continue above is in place! */
        {
            if ( now_value )
            {
                value[j] = EOS;
                now_value = FALSE;
                if ( j == 0 )
                    WAR("Value of %s is empty!", label);
                else
                    if ( (ret=set_http_req_val(ci, label, value+1)) > 200 ) return ret;
            }
            now_label = TRUE;
            j = 0;
        }
        else if ( now_label && G_connections[ci].in[i] == ':' )   /* end of label, start of value */
        {
            label[j] = EOS;
            now_label = FALSE;
            now_value = TRUE;
            j = 0;
        }
        else if ( now_label )   /* label */
        {
            if ( j < NPP_MAX_LABEL_LEN )
                label[j++] = G_connections[ci].in[i];
            else
            {
                label[j] = EOS;
                WAR("Label [%s] too long, ignoring", label);
                return 400;  /* Bad Request */
            }
        }
        else if ( now_value )   /* value */
        {
            value[j++] = G_connections[ci].in[i];

            if ( j == NPP_MAX_VALUE_LEN )   /* truncate here */
            {
                DBG("Truncating %s's value", label);
                value[j] = EOS;

                DDBG("value: [%s]", value);

                if ( (ret=set_http_req_val(ci, label, value+1)) > 200 ) return ret;
                now_value = FALSE;
            }
        }
    }

    /* -------------------------------------------------------------- */
    /* -------------------------------------------------------------- */
    /* The request headers have been read at this point */

    /* -------------------------------------------------------------- */
    /* HTTP/2 requires HTTP2-settings to be present */

#ifdef NPP_HTTP2

//    if ( G_connections[ci].status == 101 && !G_connections[ci].http2_settings[0] )
//        return 400;     /* Bad request */

#endif  /* NPP_HTTP2 */

    /* -------------------------------------------------------------- */
    /* determine whether main host has been requested */

#ifdef NPP_MULTI_HOST
#ifndef NPP_DONT_NORMALIZE_HOST

    for ( i=0; i<M_hosts_cnt; ++i )
    {
        if ( HOST(M_hosts[i].host) )
        {
            G_connections[ci].host_id = i;
            break;
        }
    }

#endif  /* NPP_DONT_NORMALIZE_HOST */
#endif  /* NPP_MULTI_HOST */

    /* Serve index if present --------------------------------------- */

#ifndef NPP_DONT_LOOK_FOR_INDEX

    if ( !G_connections[ci].uri[0] && REQ_GET )
    {
#ifdef NPP_MULTI_HOST
        if ( (!G_connections[ci].host_id && G_index_present) || M_hosts[G_connections[ci].host_id].index_present )
#else
        if ( G_index_present )
#endif
        {
            INF("Serving index.html");
            strcpy(G_connections[ci].uri, "index.html");
        }
    }

#endif  /* NPP_DONT_LOOK_FOR_INDEX */

    /* split URI and resource / id --------------------------------------- */

    if ( G_connections[ci].uri[0] )  /* if not empty */
    {
        if ( (0==strcmp(G_connections[ci].uri, "favicon.ico") && !M_popular_favicon)
                || (0==strcmp(G_connections[ci].uri, "apple-touch-icon.png") && !M_popular_appleicon)
                || (0==strcmp(G_connections[ci].uri, "robots.txt") && !M_popular_robots)
                || (0==strcmp(G_connections[ci].uri, "sw.js") && !M_popular_sw) )
            return 404;     /* Not Found */

        /* cut the query string off */

        char uri[NPP_MAX_URI_LEN+1];
        int  uri_i=0;
        while ( G_connections[ci].uri[uri_i] != EOS && G_connections[ci].uri[uri_i] != '?' )
        {
            uri[uri_i] = G_connections[ci].uri[uri_i];
            ++uri_i;
        }

        uri[uri_i] = EOS;

        DBG("uri w/o qs [%s]", uri);

        /* -------------------------------------------------------------- */
        /* tokenize */

        const char slash[]="/";
        char *token;

        token = strtok(uri, slash);

        /* resource (REQ0) */

        if ( token )
            strncpy(G_connections[ci].resource, token, NPP_MAX_RESOURCE_LEN);
        else
            strncpy(G_connections[ci].resource, uri, NPP_MAX_RESOURCE_LEN);

        G_connections[ci].resource[NPP_MAX_RESOURCE_LEN] = EOS;

#if NPP_RESOURCE_LEVELS > 1
        /* REQ1 */

        if ( token && (token=strtok(NULL, slash)) )
        {
            strncpy(G_connections[ci].req1, token, NPP_MAX_RESOURCE_LEN);
            G_connections[ci].req1[NPP_MAX_RESOURCE_LEN] = EOS;

#if NPP_RESOURCE_LEVELS > 2
            /* REQ2 */

            if ( token=strtok(NULL, slash) )
            {
                strncpy(G_connections[ci].req2, token, NPP_MAX_RESOURCE_LEN);
                G_connections[ci].req2[NPP_MAX_RESOURCE_LEN] = EOS;

#if NPP_RESOURCE_LEVELS > 3
                /* REQ3 */

                if ( token=strtok(NULL, slash) )
                {
                    strncpy(G_connections[ci].req3, token, NPP_MAX_RESOURCE_LEN);
                    G_connections[ci].req3[NPP_MAX_RESOURCE_LEN] = EOS;

#if NPP_RESOURCE_LEVELS > 4
                    /* REQ4 */

                    if ( token=strtok(NULL, slash) )
                    {
                        strncpy(G_connections[ci].req4, token, NPP_MAX_RESOURCE_LEN);
                        G_connections[ci].req4[NPP_MAX_RESOURCE_LEN] = EOS;

#if NPP_RESOURCE_LEVELS > 5
                        /* REQ5 */

                        if ( token=strtok(NULL, slash) )
                        {
                            strncpy(G_connections[ci].req5, token, NPP_MAX_RESOURCE_LEN);
                            G_connections[ci].req5[NPP_MAX_RESOURCE_LEN] = EOS;
                        }
#endif  /* NPP_RESOURCE_LEVELS > 5 */
                    }
#endif  /* NPP_RESOURCE_LEVELS > 4 */
                }
#endif  /* NPP_RESOURCE_LEVELS > 3 */
            }
#endif  /* NPP_RESOURCE_LEVELS > 2 */
        }
#endif  /* NPP_RESOURCE_LEVELS > 1 */

        /* -------------------------------------------------------------- */
        /* ID for RESTful stuff */

        char *last_slash = strrchr(G_connections[ci].uri, '/');

        if ( last_slash )
        {
            ++last_slash;   /* skip '/' */

            uri_i = 0;

            while ( *last_slash && *last_slash != '?' && uri_i < NPP_MAX_RESOURCE_LEN-1 )
                G_connections[ci].id[uri_i++] = *last_slash++;

            G_connections[ci].id[uri_i] = EOS;
        }

        /* -------------------------------------------------------------- */

#ifdef NPP_DEBUG
        DBG("REQ0 [%s]", G_connections[ci].resource);
#if NPP_RESOURCE_LEVELS > 1
        DBG("REQ1 [%s]", G_connections[ci].req1);
#if NPP_RESOURCE_LEVELS > 2
        DBG("REQ2 [%s]", G_connections[ci].req2);
#if NPP_RESOURCE_LEVELS > 3
        DBG("REQ3 [%s]", G_connections[ci].req3);
#if NPP_RESOURCE_LEVELS > 4
        DBG("REQ4 [%s]", G_connections[ci].req4);
#if NPP_RESOURCE_LEVELS > 5
        DBG("REQ5 [%s]", G_connections[ci].req5);
#endif  /* NPP_RESOURCE_LEVELS > 5 */
#endif  /* NPP_RESOURCE_LEVELS > 4 */
#endif  /* NPP_RESOURCE_LEVELS > 3 */
#endif  /* NPP_RESOURCE_LEVELS > 2 */
#endif  /* NPP_RESOURCE_LEVELS > 1 */
        DBG("  ID [%s]", G_connections[ci].id);
#endif
        /* -------------------------------------------------------------- */

        G_connections[ci].static_res = is_static_res(ci);    /* statics --> set the flag!!! */
        /* now, it may have set G_connections[ci].status to 304 */

        if ( G_connections[ci].static_res != NPP_NOT_STATIC )    /* static resource */
            G_connections[ci].out_data = M_stat[G_connections[ci].static_res].data;
    }

    /* -------------------------------------------------------------- */
    /* get the required authorization level for this resource */

    if ( G_connections[ci].static_res == NPP_NOT_STATIC )
    {
        i = 0;
        while ( M_auth_levels[i].path[0] != '-' )
        {
            if ( URI(M_auth_levels[i].path) )
            {
                DDBG("Required authorization level = %d", M_auth_levels[i].level);
                G_connections[ci].required_auth_level = M_auth_levels[i].level;
                break;
            }
            ++i;
        }
    }
    else    /* don't do any checks for static resources */
    {
        G_connections[ci].required_auth_level = AUTH_LEVEL_NONE;
    }

    /* ignore Range requests for now -------------------------------------------- */

/*  if ( G_connections[ci].conn_state == STATE_SENDING )
    {
        DBG("conn_state == STATE_SENDING, this request will be ignored");
        return 200;
    } */

    DBG("bot = %s", REQ_BOT?"TRUE":"FALSE");

    /* update request counters -------------------------------------------------- */

    if ( REQ_BOT )
        ++G_cnts_today.req_bot;

    if ( G_connections[ci].ua_type == NPP_UA_TYPE_MOB )
        ++G_cnts_today.req_mob;
    else if ( G_connections[ci].ua_type == NPP_UA_TYPE_TAB )
        ++G_cnts_today.req_tab;
    else    /* desktop */
        ++G_cnts_today.req_dsk;

    /* Block IP? ---------------------------------------------------------------- */

#ifdef NPP_BLACKLIST_AUTO_UPDATE
        if ( check_block_ip(ci, "Resource", G_connections[ci].resource) )
            return 404;     /* Forbidden */
#endif

    /* Redirection? ------------------------------------------------------------- */
    /*

    Test Cases (for HTTPS enabled)

    +------------+------+----------------------------+------------------------+
    | NPP_DOMAIN_ONLY | HSTS | request                    | result                 |
    +            +      +------------------+---------+------+-----------------+
    |            |      | url              | U2HTTPS | code | location        |
    +------------+------+------------------+---------+------+-----------------+
    | F          | F    | http://domain    | F       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | F          | F    | http://domain    | T       | 301  | https://domain  |
    +------------+------+------------------+---------+------+-----------------+
    | F          | F    | http://1.2.3.4   | F       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | F          | F    | http://1.2.3.4   | T       | 301  | https://1.2.3.4 |
    +------------+------+------------------+---------+------+-----------------+
    | F          | F    | https://domain   | F       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | F          | F    | https://domain   | T       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | F          | F    | https://1.2.3.4  | F       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | F          | F    | https://1.2.3.4  | T       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | F          | T    | http://domain    | F       | 301  | https://domain  |
    +------------+------+------------------+---------+------+-----------------+
    | F          | T    | http://domain    | T       | 301  | https://domain  |
    +------------+------+------------------+---------+------+-----------------+
    | F          | T    | http://1.2.3.4   | F       | 200  | https://1.2.3.4 |
    +------------+------+------------------+---------+------+-----------------+
    | F          | T    | http://1.2.3.4   | T       | 200  | https://1.2.3.4 |
    +------------+------+------------------+---------+------+-----------------+
    | F          | T    | https://domain   | F       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | F          | T    | https://domain   | T       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | F          | T    | https://1.2.3.4  | F       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | F          | T    | https://1.2.3.4  | T       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+

    (NPP_DOMAIN_ONLY enabled)

    +------------+------+------------------+---------+------+-----------------+
    | T          | F    | http://domain    | F       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | T          | F    | http://domain    | T       | 301  | https://domain  |
    +------------+------+------------------+---------+------+-----------------+
    | T          | F    | http://1.2.3.4   | F       | 301  | http://domain   |
    +------------+------+------------------+---------+------+-----------------+
    | T          | F    | http://1.2.3.4   | T       | 301  | https://domain  |
    +------------+------+------------------+---------+------+-----------------+
    | T          | F    | https://domain   | F       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | T          | F    | https://domain   | T       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | T          | F    | https://1.2.3.4  | F       | 301  | http://domain   |
    +------------+------+------------------+---------+------+-----------------+
    | T          | F    | https://1.2.3.4  | T       | 301  | https://domain  |
    +------------+------+------------------+---------+------+-----------------+
    | T          | T    | http://domain    | F       | 301  | https://domain  |
    +------------+------+------------------+---------+------+-----------------+
    | T          | T    | http://domain    | T       | 301  | https://domain  |
    +------------+------+------------------+---------+------+-----------------+
    | T          | T    | http://1.2.3.4   | F       | 301  | https://domain  |
    +------------+------+------------------+---------+------+-----------------+
    | T          | T    | http://1.2.3.4   | T       | 301  | https://domain  |
    +------------+------+------------------+---------+------+-----------------+
    | T          | T    | https://domain   | F       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | T          | T    | https://domain   | T       | 200  |                 |
    +------------+------+------------------+---------+------+-----------------+
    | T          | T    | https://1.2.3.4  | F       | 301  | https://domain  |
    +------------+------+------------------+---------+------+-----------------+
    | T          | T    | https://1.2.3.4  | T       | 301  | https://domain  |
    +------------+------+------------------+---------+------+-----------------+
    */

    if ( !G_test )   /* don't redirect if test */
    {

#ifdef NPP_HTTPS

#ifdef NPP_HSTS_ON
        if ( !NPP_CONN_IS_SECURE(G_connections[ci].flags) )
        {
            DDBG("Redirecting due to HSTS");
#ifdef NPP_DOMAIN_ONLY
            if ( G_connections[ci].uri[0] )
                RES_LOCATION("https://%s/%s", NPP_APP_DOMAIN, G_connections[ci].uri);
            else
                RES_LOCATION("https://%s", NPP_APP_DOMAIN);
#else
            if ( G_connections[ci].uri[0] )
                RES_LOCATION("https://%s/%s", G_connections[ci].host, G_connections[ci].uri);
            else
                RES_LOCATION("https://%s", G_connections[ci].host);
#endif
            return 301;
        }
#endif  /* NPP_HSTS_ON */

        if ( !NPP_CONN_IS_SECURE(G_connections[ci].flags) && NPP_CONN_IS_UPGRADE_TO_HTTPS(G_connections[ci].flags) )
        {
            DDBG("Redirecting due to upgrade2https");

#ifdef NPP_DOMAIN_ONLY
            if ( G_connections[ci].uri[0] )
                RES_LOCATION("https://%s/%s", NPP_APP_DOMAIN, G_connections[ci].uri);
            else
                RES_LOCATION("https://%s", NPP_APP_DOMAIN);
#else
            if ( G_connections[ci].uri[0] )
                RES_LOCATION("https://%s/%s", G_connections[ci].host, G_connections[ci].uri);
            else
                RES_LOCATION("https://%s", G_connections[ci].host);
#endif
            return 301;
        }

#ifdef NPP_DOMAIN_ONLY
        if ( 0 != strcmp(G_connections[ci].host, NPP_APP_DOMAIN) )
        {
            DDBG("Redirecting due to NPP_DOMAIN_ONLY");

            if ( G_connections[ci].uri[0] )
                RES_LOCATION("%s://%s/%s", NPP_PROTOCOL, NPP_APP_DOMAIN, G_connections[ci].uri);
            else
                RES_LOCATION("%s://%s", NPP_PROTOCOL, NPP_APP_DOMAIN);

            return 301;
        }
#endif  /* NPP_DOMAIN_ONLY */

#else   /* not HTTPS */

#ifdef NPP_DOMAIN_ONLY
        if ( 0 != strcmp(G_connections[ci].host, NPP_APP_DOMAIN) )
        {
            DDBG("Redirecting due to NPP_DOMAIN_ONLY");

            if ( G_connections[ci].uri[0] )
                RES_LOCATION("http://%s/%s", NPP_APP_DOMAIN, G_connections[ci].uri);
            else
                RES_LOCATION("http://%s", NPP_APP_DOMAIN);

            return 301;
        }
#endif  /* NPP_DOMAIN_ONLY */

#endif  /* NPP_HTTPS */

    }

    /* handle the POST content -------------------------------------------------- */

    if ( NPP_CONN_IS_PAYLOAD(G_connections[ci].flags) && G_connections[ci].clen > 0 )
    {
        /* i = number of request characters read so far */

        /* p_hend will now point to the content */

        if ( 0==strncmp(p_hend, "\r\n\r\n", 4) )
            p_hend += 4;
        else    /* was "\n\n" */
            p_hend += 2;

        len = G_connections[ci].in+len - p_hend;   /* remaining request length -- likely a content */

        DDBG("Remaining request length (content) = %d", len);

        if ( len > G_connections[ci].clen )
            return 400;     /* Bad Request */

        /* copy so far received POST data from G_connections[ci].in to G_connections[ci].in_data */

        if ( NULL == (G_connections[ci].in_data=(char*)malloc(G_connections[ci].clen+1)) )
        {
            ERR("Couldn't allocate %u bytes for POST data", G_connections[ci].clen);
            return 500;     /* Internal Sever Error */
        }

        memcpy(G_connections[ci].in_data, p_hend, len);
        G_connections[ci].was_read = len;    /* if POST then was_read applies to data section only! */

        if ( len < G_connections[ci].clen )      /* the whole content not received yet */
        {                               /* this is the only case when conn_state != received */
            DBG("The whole content not received yet, len=%d", len);

            DDBG("Changing state to CONN_STATE_READING_DATA");
            G_connections[ci].conn_state = CONN_STATE_READING_DATA;

            return ret;
        }
        else    /* the whole content received with the header at once */
        {
            G_connections[ci].in_data[len] = EOS;
            DBG("POST data received with header");
        }
    }

    if ( G_connections[ci].status == 304 )   /* Not Modified */
        return 304;
    else
        return ret;
}


static const uint8_t base64url_dec_table[128] =
{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
    0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F,
    0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
  
  
/* --------------------------------------------------------------------------
   Decode base64url encoded string
   Return binary length
-------------------------------------------------------------------------- */
static int base64url_decode(unsigned char *dst, const char *src)
{
    uint32_t value=0;
    char c;
    int i;
    int n=0;
    unsigned char *p=dst;
 
    for ( i=0; src[i]; ++i )
    {
        c = src[i];

        if ( c < 128 && base64url_dec_table[c] < 64 )
        {
            value = (value << 6) | base64url_dec_table[c];

            if ( (i%4) == 3 )
            {
                if (p != NULL)
                {
                    p[n] = (value >> 16) & 0xFF;
                    p[n+1] = (value >> 8) & 0xFF;
                    p[n+2] = value & 0xFF;
                }

                n += 3;
                value = 0;
            }
        }
        else
        {
            WAR("Invalid base64url character");
            return 0;
        }
    }
  
    /* treat the last block */

    int len = strlen(src);

    if ( (len % 4) == 2 )
    {
        if ( p != NULL )
            p[n] = (value >> 4) & 0xFF;

        ++n;
    }
    else if ( (len % 4) == 3 )
    {
        if ( p != NULL )
        {
            p[n] = (value >> 10) & 0xFF;
            p[n+1] = (value >> 2) & 0xFF;
        }

        n += 2;
    }

    return n;
}


/* --------------------------------------------------------------------------
   Set request properties read from HTTP request header
   Caller is responsible for ensuring value length > 0
   Return HTTP status code
-------------------------------------------------------------------------- */
static int set_http_req_val(int ci, const char *label, const char *value)
{
    char ulabel[NPP_MAX_LABEL_LEN+1];
    char uvalue[NPP_MAX_VALUE_LEN+1];
    char *p;
    int  i;
#ifdef NPP_HTTP2
    unsigned char http2_settings[4096];
#endif  /* NPP_HTTP2 */

    /* only for low-level tests ------------------------------------- */
//  DBG("label: [%s], value: [%s]", label, value);
    /* -------------------------------------------------------------- */

    strcpy(ulabel, npp_upper(label));

    if ( 0==strcmp(ulabel, "HOST") )
    {
#ifdef NPP_BLACKLIST_AUTO_UPDATE
        if ( check_block_ip(ci, "Host", value) )
            return 404;     /* Forbidden */
#endif
        COPY(G_connections[ci].host, value, NPP_MAX_HOST_LEN);

#ifndef NPP_DONT_NORMALIZE_HOST

        DDBG("Host before normalization [%s]", G_connections[ci].host);
        /* normalize for comparisons */

        /* cut the port off */

        i = 0;
        int dots = 0;
        int first_dot;

        while ( G_connections[ci].host[i] )
        {
            if ( G_connections[ci].host[i] == '.' )
            {
                if ( !dots )
                    first_dot = i;

                ++dots;

                G_connections[ci].host_normalized[i] = '.';
            }
            else if ( G_connections[ci].host[i] == ':' )
            {
                break;
            }
            else
            {
                G_connections[ci].host_normalized[i] = toupper(G_connections[ci].host[i]);
            }

            ++i;
        }

        G_connections[ci].host_normalized[i] = EOS;

        if ( dots > 1 && !isdigit(G_connections[ci].host_normalized[i-1]) )
        {
            DDBG("Need to cut subdomain off [%s]", G_connections[ci].host_normalized);

            i = first_dot + 1;
            int j=0;

            while ( G_connections[ci].host[i] )
            {
                if ( G_connections[ci].host[i] == ':' ) break;

                G_connections[ci].host_normalized[j++] = toupper(G_connections[ci].host[i]);

                ++i;
            }

            G_connections[ci].host_normalized[j] = EOS;
        }

        DDBG("host_normalized [%s]", G_connections[ci].host_normalized);

#endif  /* NPP_DONT_NORMALIZE_HOST */
    }
    else if ( 0==strcmp(ulabel, "USER-AGENT") )
    {
#ifdef NPP_BLACKLIST_AUTO_UPDATE
        if ( check_block_ip(ci, "User-Agent", value) )
            return 404;     /* Forbidden */
#endif
        strcpy(G_connections[ci].uagent, value);
        strcpy(uvalue, npp_upper(value));

        if ( strstr(uvalue, "IPAD") || strstr(uvalue, "TABLET") || strstr(uvalue, "KINDLE") || strstr(uvalue, "PLAYBOOK") || strstr(uvalue, "SM-T555") )
        {
            G_connections[ci].ua_type = NPP_UA_TYPE_TAB;
            DBG("ua_type = NPP_UA_TYPE_TAB");
        }
        else if ( strstr(uvalue, "ANDROID") || strstr(uvalue, "IPHONE") || strstr(uvalue, "BLACKBERRY") || strstr(uvalue, "MOBILE") || strstr(uvalue, "SYMBIAN") )
        {
            G_connections[ci].ua_type = NPP_UA_TYPE_MOB;
            DBG("ua_type = NPP_UA_TYPE_MOB");
        }
        else
        {
            DBG("ua_type = NPP_UA_TYPE_DSK");
        }

        if ( !REQ_BOT &&
                (strstr(uvalue, "BOT")
                || strstr(uvalue, "SCAN")
                || strstr(uvalue, "CRAWLER")
                || strstr(uvalue, "SPIDER")
                || strstr(uvalue, "SURDOTLY")
                || strstr(uvalue, "BAIDU")
                || strstr(uvalue, "ZGRAB")
                || strstr(uvalue, "DOMAINSONO")
                || strstr(uvalue, "NETCRAFT")
                || 0==strncmp(uvalue, "APPENGINE", 9)
                || 0==strncmp(uvalue, "NETSYSTEMSRESEARCH", 18)
                || 0==strncmp(uvalue, "CURL", 4)
                || 0==strncmp(uvalue, "BUBING", 6)
                || 0==strncmp(uvalue, "PYTHON-REQUESTS", 15)
                || 0==strncmp(uvalue, "CLOUD MAPPING", 13)
                || 0==strcmp(uvalue, "TELESPHOREO")
                || 0==strcmp(uvalue, "MAGIC BROWSER")) )
        {
            G_connections[ci].flags |= NPP_CONN_FLAG_BOT;
        }
    }
    else if ( 0==strcmp(ulabel, "CONNECTION") )
    {
        strcpy(uvalue, npp_upper(value));
        if ( 0==strcmp(uvalue, "KEEP-ALIVE") )
            G_connections[ci].flags |= NPP_CONN_FLAG_KEEP_ALIVE;
    }
    else if ( 0==strcmp(ulabel, "COOKIE") )
    {
        strcpy(G_connections[ci].in_cookie, value);

        /* parse authentication data */

        if ( strlen(value) < NPP_SESSID_LEN+3 ) return 200;   /* no valid session cookie but request still OK */

        /* parse cookies, set anonymous and / or logged in sesid */

        if ( NULL != (p=(char*)strstr(value, "as=")) )   /* anonymous sesid present? */
        {
            p += 3;
            if ( strlen(p) >= NPP_SESSID_LEN )
            {
                strncpy(G_connections[ci].cookie_in_a, p, NPP_SESSID_LEN);
                G_connections[ci].cookie_in_a[NPP_SESSID_LEN] = EOS;
            }
        }
        if ( NULL != (p=(char*)strstr(value, "ls=")) )   /* logged in sesid present? */
        {
            p += 3;
            if ( strlen(p) >= NPP_SESSID_LEN )
            {
                strncpy(G_connections[ci].cookie_in_l, p, NPP_SESSID_LEN);
                G_connections[ci].cookie_in_l[NPP_SESSID_LEN] = EOS;
            }
        }
    }
    else if ( 0==strcmp(ulabel, "REFERER") )
    {
        strcpy(G_connections[ci].referer, value);
//      if ( !G_connections[ci].uri[0] && value[0] )
//          INF("Referer: [%s]", value);
    }
    else if ( 0==strcmp(ulabel, "CONTENT-TYPE") )
    {
        strcpy(G_connections[ci].in_ctypestr, value);

        strcpy(uvalue, npp_upper(value));

        int len = strlen(value);

        if ( len > 15 && 0==strncmp(uvalue, "APPLICATION/JSON", 16) )
        {
            G_connections[ci].in_ctype = NPP_CONTENT_TYPE_JSON;
        }
        else if ( len > 32 && 0==strncmp(uvalue, "APPLICATION/X-WWW-FORM-URLENCODED", 33) )
        {
            G_connections[ci].in_ctype = NPP_CONTENT_TYPE_URLENCODED;
        }
        else if ( len > 18 && 0==strncmp(uvalue, "MULTIPART/FORM-DATA", 19) )
        {
            G_connections[ci].in_ctype = NPP_CONTENT_TYPE_MULTIPART;

            if ( p=(char*)strstr(value, "boundary=") )
            {
                COPY(G_connections[ci].boundary, p+9, NPP_MAX_BOUNDARY_LEN);
                DBG("boundary: [%s]", G_connections[ci].boundary);
            }
        }
        else if ( len > 23 && 0==strncmp(uvalue, "APPLICATION/OCTET-STREAM", 24) )
        {
            G_connections[ci].in_ctype = NPP_CONTENT_TYPE_OCTET_STREAM;
        }
    }
    else if ( 0==strcmp(ulabel, "AUTHORIZATION") )
    {
        strcpy(G_connections[ci].authorization, value);
    }
    else if ( 0==strcmp(ulabel, "FROM") )
    {
        strcpy(uvalue, npp_upper(value));
        if ( !REQ_BOT && (strstr(uvalue, "GOOGLEBOT") || strstr(uvalue, "BINGBOT") || strstr(uvalue, "YANDEX") || strstr(uvalue, "CRAWLER")) )
            G_connections[ci].flags |= NPP_CONN_FLAG_BOT;
    }
    else if ( 0==strcmp(ulabel, "IF-MODIFIED-SINCE") )
    {
        G_connections[ci].if_mod_since = time_http2epoch(value);
    }
    else if ( !NPP_CONN_IS_SECURE(G_connections[ci].flags) && !G_test && 0==strcmp(ulabel, "UPGRADE-INSECURE-REQUESTS") && 0==strcmp(value, "1") )
    {
        DBG("Client wants to upgrade to HTTPS");
        G_connections[ci].flags |= NPP_CONN_FLAG_UPGRADE_TO_HTTPS;
    }
    else if ( 0==strcmp(ulabel, "X-FORWARDED-FOR") )    /* keep first IP as client IP */
    {
        /* it can be 'unknown' */

        char tmp[INET_ADDRSTRLEN+1];
        int len = strlen(value);
        i = 0;

        while ( i<len && (value[i]=='.' || isdigit(value[i])) && i<INET_ADDRSTRLEN )
        {
            tmp[i] = value[i];
            ++i;
        }

        tmp[i] = EOS;

        DBG("%s's value: [%s]", label, tmp);

        if ( strlen(tmp) > 6 )
            strcpy(G_connections[ci].ip, tmp);
    }
    else if ( 0==strcmp(ulabel, "CONTENT-LENGTH") )
    {
        G_connections[ci].clen = atoi(value);
        if ( (!NPP_CONN_IS_PAYLOAD(G_connections[ci].flags) && G_connections[ci].clen >= NPP_IN_BUFSIZE) || (NPP_CONN_IS_PAYLOAD(G_connections[ci].flags) && G_connections[ci].clen >= NPP_MAX_PAYLOAD_SIZE-1) )
        {
            ERR("Request too long, clen = %u, sending 413", G_connections[ci].clen);
            return 413;
        }
        DBG("G_connections[ci].clen = %u", G_connections[ci].clen);
    }
    else if ( 0==strcmp(ulabel, "ACCEPT-ENCODING") )    /* gzip, deflate, br */
    {
        strcpy(uvalue, npp_upper(value));

        if ( strstr(uvalue, "DEFLATE") )
        {
            G_connections[ci].flags |= NPP_CONN_FLAG_ACCEPT_DEFLATE;
            DDBG("accept_deflate = TRUE");
        }
    }
    else if ( 0==strcmp(ulabel, "ACCEPT-LANGUAGE") )    /* en-US en-GB pl-PL */
    {
        if ( !IS_SESSION || !SESSION.lang[0] )    /* session data has priority */
        {
            DBG("No session or no language in session, setting formats");

            i = 0;
            while ( value[i] != EOS && value[i] != ',' && value[i] != ';' && i < NPP_LANG_LEN )
            {
                G_connections[ci].lang[i] = toupper(value[i]);
                ++i;
            }

            G_connections[ci].lang[i] = EOS;

            DBG("G_connections[ci].lang: [%s]", G_connections[ci].lang);

            /* for npp_message and npp_lib_get_string if no session */

//            strcpy(G_sessions[0].lang, G_connections[ci].lang);

            if ( G_connections[ci].lang[0] )
                npp_lib_set_datetime_formats(ci);
        }
    }
#ifdef NPP_HTTP2
    else if ( 0==strcmp(ulabel, "UPGRADE") )
    {
        if ( strcmp(value, "h2c") == 0 )
        {
            INF("Client wants to switch to HTTP/2 (cleartext)");
            G_connections[ci].http2_upgrade_in_progress = TRUE;
//            return 101;     /* Switching Protocols */
        }
    }
    else if ( 0==strcmp(ulabel, "HTTP2-SETTINGS") )
    {
        DDBG("HTTP2-Settings received [%s]", value);

        int rem = base64url_decode(http2_settings, value);
        DDBG("rem = %d", rem);

        http2_SETTINGS_pld_t s;
        unsigned char *p=http2_settings;
        int read = 0;

        while ( rem >= HTTP2_SETTINGS_PAIR_LEN )
        {
            memcpy((char*)&s, p+read, HTTP2_SETTINGS_PAIR_LEN);
            read += HTTP2_SETTINGS_PAIR_LEN;
            rem -= HTTP2_SETTINGS_PAIR_LEN;

            /* convert from network byte order to machine */

            s.id = ntohs(s.id);
            s.value = ntohl(s.value);

            if ( s.id == HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS )
            {
                DBG("http2_max_streams = %u", s.value);
                G_connections[ci].http2_max_streams = s.value;
            }
            else if ( s.id == HTTP2_SETTINGS_INITIAL_WINDOW_SIZE )
            {
                DBG("http2_wnd_size = %u", s.value);
                G_connections[ci].http2_wnd_size = s.value;
            }
            else if ( s.id == HTTP2_SETTINGS_MAX_FRAME_SIZE )
            {
                DBG("http2_max_frame_size = %u", s.value);
                G_connections[ci].http2_max_frame_size = s.value;
            }
            else
            {
                DBG("HTTP/2 settings that we ignore:");
                DBG("id = %hd", s.id);
                DBG("value = %u", s.value);
            }
        }
    }
#endif  /* NPP_HTTP2 */
    else if ( 0==strcmp(ulabel, "EXPECT") )
    {
        if ( 0==strcmp(value, "100-continue") )
            G_connections[ci].expect100 = TRUE;
    }

    return 200;
}


/* --------------------------------------------------------------------------
   Check the rules and block IP if matches
   Return TRUE if blocked
   It doesn't really change anything security-wise but just saves bandwidth
-------------------------------------------------------------------------- */
static bool check_block_ip(int ci, const char *rule, const char *value)
{
    if ( G_test ) return FALSE;    /* don't block for tests */

#ifdef NPP_BLACKLIST_AUTO_UPDATE
    if ( (rule[0]=='H' && NPP_CONN_IS_PAYLOAD(G_connections[ci].flags) && isdigit(value[0])) /* Host */
            || (rule[0]=='U' && 0==strcmp(value, "Mozilla/5.0 Jorgee"))     /* User-Agent */
            || (rule[0]=='R' && 0==strcmp(value, "wp-login.php"))           /* Resource */
            || (rule[0]=='R' && 0==strcmp(value, "wp-config.php"))          /* Resource */
            || (rule[0]=='R' && 0==strcmp(value, "administrator"))          /* Resource */
            || (rule[0]=='R' && 0==strncmp(value, "phpmyadmin", 10))        /* Resource */
            || (rule[0]=='R' && 0==strncmp(value, "phpMyAdmin", 10))        /* Resource */
            || (rule[0]=='R' && 0==strcmp(value, "java.php"))               /* Resource */
            || (rule[0]=='R' && 0==strcmp(value, "logon.php"))              /* Resource */
            || (rule[0]=='R' && 0==strcmp(value, "log.php"))                /* Resource */
            || (rule[0]=='R' && 0==strcmp(value, "hell.php"))               /* Resource */
            || (rule[0]=='R' && 0==strcmp(value, "shell.php"))              /* Resource */
            || (rule[0]=='R' && 0==strcmp(value, "tty.php"))                /* Resource */
            || (rule[0]=='R' && 0==strcmp(value, "cmd.php"))                /* Resource */
            || (rule[0]=='R' && 0==strcmp(value, ".env"))                   /* Resource */
            || (rule[0]=='R' && strstr(value, "setup.php")) )               /* Resource */
    {
        npp_eng_block_ip(G_connections[ci].ip, TRUE);
        G_connections[ci].flags = G_connections[ci].flags & ~NPP_CONN_FLAG_KEEP_ALIVE;    /* disconnect */
        return TRUE;
    }
#endif

    return FALSE;
}


/* --------------------------------------------------------------------------
   Return HTTP status description
-------------------------------------------------------------------------- */
static char *get_http_descr(int status_code)
{
    int i;

    for ( i=0; M_http_status[i].status != -1; ++i )
    {
        if ( M_http_status[i].status == status_code )  /* found */
            return (char*)&(M_http_status[i].description);
    }

    return NULL;
}


/* --------------------------------------------------------------------------
   Dump counters
-------------------------------------------------------------------------- */
static void dump_counters()
{
    ALWAYS("");
    ALWAYS("Counters:\n");
    ALWAYS("            req: %u", G_cnts_today.req);
    ALWAYS("        req_dsk: %u", G_cnts_today.req_dsk);
    ALWAYS("        req_tab: %u", G_cnts_today.req_tab);
    ALWAYS("        req_mob: %u", G_cnts_today.req_mob);
    ALWAYS("        req_bot: %u", G_cnts_today.req_bot);
    ALWAYS("         visits: %u", G_cnts_today.visits);
    ALWAYS("     visits_dsk: %u", G_cnts_today.visits_dsk);
    ALWAYS("     visits_tab: %u", G_cnts_today.visits_tab);
    ALWAYS("     visits_mob: %u", G_cnts_today.visits_mob);
    ALWAYS("        blocked: %u", G_cnts_today.blocked);
    ALWAYS("        average: %.3lf ms", G_cnts_today.average);
    ALWAYS("connections HWM: %d", G_connections_hwm);
    ALWAYS("   sessions HWM: %d", G_sessions_hwm);
    ALWAYS("");
}


/* --------------------------------------------------------------------------
   Clean up
-------------------------------------------------------------------------- */
static void clean_up()
{
    M_shutdown = TRUE;

    ALWAYS("");
    ALWAYS("Cleaning up...\n");
    npp_log_memory();
    dump_counters();

    npp_app_done();

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

#ifdef NPP_USERS
    libusr_luses_save_csrft();
#endif

    npp_close_db();

#endif  /* NPP_MYSQL */


#ifdef NPP_HTTPS
    SSL_CTX_free(M_ssl_ctx);
    EVP_cleanup();
#endif


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

#ifdef _WIN32   /* Windows */
    WSACleanup();
#endif  /* _WIN32 */

    npp_lib_done();
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
   Generic message page
-------------------------------------------------------------------------- */
static void render_page_msg(int ci, int code)
{
    DBG("render_page_msg");

#ifdef NPP_APP_CUSTOM_MESSAGE_PAGE

    npp_app_custom_message_page(ci, code);

#else

    OUT("<!DOCTYPE html>");
    OUT("<html>");
    OUT("<head>");
    OUT("<title>%s</title>", NPP_APP_NAME);
    if ( REQ_MOB )  // if mobile request
        OUT("<meta name=\"viewport\" content=\"width=device-width\">");
    OUT("</head>");
    OUT("<body><p>%s</p></body>", npp_message(code));
    OUT("</html>");

#endif  /* NPP_APP_CUSTOM_MESSAGE_PAGE */
}


/* --------------------------------------------------------------------------
   Init SSL for a server
-------------------------------------------------------------------------- */
static bool init_ssl()
{
#ifdef NPP_HTTPS
    const SSL_METHOD *method;
    /*
       From Hynek Schlawack's blog:
       https://hynek.me/articles/hardening-your-web-servers-ssl-ciphers
       https://www.ssllabs.com/ssltest
       Last update: 2019-04-08
       Qualys says Forward Secrecy isn't enabled
    */
//    char ciphers[1024]="ECDH+AESGCM:ECDH+CHACHA20:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:RSA+AESGCM:RSA+AES:!aNULL:!MD5:!DSS";

    /*
       https://www.digicert.com/ssl-support/ssl-enabling-perfect-forward-secrecy.htm
       Last update: 2019-04-18
    */
    char ciphers[1024]="EECDH+ECDSA+AESGCM EECDH+aRSA+AESGCM EECDH+ECDSA+SHA384 EECDH+ECDSA+SHA256 EECDH+aRSA+SHA384 EECDH+aRSA+SHA256 EECDH+aRSA+RC4 EECDH EDH+aRSA RC4 !aNULL !eNULL !LOW !3DES !MD5 !EXP !PSK !SRP !DSS !RC4";

    DBG("init_ssl");

    /* libssl init */
    SSL_library_init();
    SSL_load_error_strings();

    /* libcrypto init */
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    G_ssl_lib_initialized = TRUE;

    method = SSLv23_server_method();    /* negotiate the highest protocol version supported by both the server and the client */

    M_ssl_ctx = SSL_CTX_new(method);    /* create new context from method */

    if ( M_ssl_ctx == NULL )
    {
        ERR("SSL_CTX_new failed");
        return FALSE;
    }

    const long flags = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1;
    SSL_CTX_set_options(M_ssl_ctx, flags);

    /* support ECDH using the most appropriate shared curve */

    if ( SSL_CTX_set_ecdh_auto(M_ssl_ctx, 1) <= 0 )
    {
        ERR("SSL_CTX_set_ecdh_auto failed");
        return FALSE;
    }

    if ( G_cipherList[0] )
        strcpy(ciphers, G_cipherList);

    ALWAYS("        Using ciphers: [%s]", ciphers);

    SSL_CTX_set_cipher_list(M_ssl_ctx, ciphers);

    /* set the local certificate */

    ALWAYS("    Using certificate: [%s]", G_certFile);

    if ( SSL_CTX_use_certificate_file(M_ssl_ctx, G_certFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR("SSL_CTX_use_certificate_file failed");
        return FALSE;
    }

    if ( G_certChainFile[0] )   /* set the chain file */
    {
        ALWAYS("Using cert chain file: [%s]", G_certChainFile);

        if ( SSL_CTX_load_verify_locations(M_ssl_ctx, G_certChainFile, NULL) <= 0 )
        {
            ERR("SSL_CTX_load_verify_locations failed");
            return FALSE;
        }
    }

   /* set the private key from KeyFile (may be the same as CertFile) */

    ALWAYS("    Using private key: [%s]", G_keyFile);

    if ( SSL_CTX_use_PrivateKey_file(M_ssl_ctx, G_keyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR("SSL_CTX_use_PrivateKey_file failed");
        return FALSE;
    }

    /* verify private key */

    if ( !SSL_CTX_check_private_key(M_ssl_ctx) )
    {
        ERR("Private key does not match the public certificate");
        return FALSE;
    }
#endif  /* NPP_HTTPS */
    return TRUE;
}





/* ============================================================================================================= */
/* PUBLIC ENGINE FUNCTIONS (callbacks)                                                                           */
/* ============================================================================================================= */


/* --------------------------------------------------------------------------
   Set required authorization level for the resource
-------------------------------------------------------------------------- */
void npp_set_auth_level(const char *path, char level)
{
static int current=0;

    if ( current > NPP_MAX_RESOURCES-2 )
        return;

    strncpy(M_auth_levels[current].path, path, 255);
    M_auth_levels[current].path[255] = EOS;
    M_auth_levels[current].level = level;

    strcpy(M_auth_levels[++current].path, "-");
}


/* --------------------------------------------------------------------------
   Start new anonymous user session
-------------------------------------------------------------------------- */
int npp_eng_session_start(int ci, const char *sesid)
{
    int     i;
    char    new_sesid[NPP_SESSID_LEN+1];

    DBG("npp_eng_session_start");

    if ( G_sessions_cnt == NPP_MAX_SESSIONS )
    {
        WAR("User sessions exhausted");
        return ERR_SERVER_TOOBUSY;
    }

    ++G_sessions_cnt;   /* start from 1 */

    /* find first free slot */

    for ( i=1; i<=NPP_MAX_SESSIONS; ++i )
    {
        if ( G_sessions[i].sesid[0] == EOS )
        {
            G_connections[ci].si = i;
            break;
        }
    }

    if ( sesid )
    {
        strcpy(new_sesid, sesid);
    }
    else    /* generate sesid */
    {
        npp_random(new_sesid, NPP_SESSID_LEN);
    }

    INF("Starting new session for ci=%d, si=%d, sesid [%s]", ci, G_connections[ci].si, new_sesid);

    /* add record to G_sessions */

    strcpy(SESSION.sesid, new_sesid);
    strcpy(SESSION.ip, G_connections[ci].ip);
    strcpy(SESSION.uagent, G_connections[ci].uagent);
    strcpy(SESSION.referer, G_connections[ci].referer);
    strcpy(SESSION.lang, G_connections[ci].lang);
    SESSION.formats = G_connections[ci].formats;

    /* generate CSRF token */

    npp_random(SESSION.csrft, NPP_CSRFT_LEN);

    DBG("New csrft generated [%s]", SESSION.csrft);

    /* custom session init */

    if ( !npp_app_session_init(ci) )
    {
        close_uses(G_connections[ci].si, ci);
        return ERR_INT_SERVER_ERROR;
    }

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
#ifdef NPP_USERS
    int i;

    if ( ci > -1 )  /* keep the current session */
    {
        for ( i=1; G_sessions_cnt>0 && i<=NPP_MAX_SESSIONS; ++i )
        {
            if ( G_sessions[i].sesid[0] && G_sessions[i].auth_level>AUTH_LEVEL_ANONYMOUS && G_sessions[i].user_id==user_id && 0!=strcmp(G_sessions[i].sesid, SESSION.sesid) )
                libusr_luses_downgrade(i, NPP_NOT_CONNECTED, FALSE);
        }
    }
    else    /* all sessions */
    {
        for ( i=1; G_sessions_cnt>0 && i<=NPP_MAX_SESSIONS; ++i )
        {
            if ( G_sessions[i].sesid[0] && G_sessions[i].auth_level>AUTH_LEVEL_ANONYMOUS && G_sessions[i].user_id==user_id )
                libusr_luses_downgrade(i, NPP_NOT_CONNECTED, FALSE);
        }
    }
#endif  /* NPP_USERS */
}


/* --------------------------------------------------------------------------
   Send asynchronous request
-------------------------------------------------------------------------- */
void npp_eng_call_async(int ci, const char *service, const char *data, bool want_response, int timeout, int size)
{
#ifdef NPP_ASYNC

    async_req_t req;

    if ( M_last_call_id >= 1000000000 ) M_last_call_id = 0;

    req.hdr.call_id = ++M_last_call_id;
    req.hdr.ci = ci;

    if ( service )
        strcpy(req.hdr.service, service);
    else
        req.hdr.service[0] = EOS;

//    req.hdr.want_response = want_response;

    /* G_connections */

//    req.hdr.secure = G_connections[ci].secure;
    strcpy(req.hdr.ip, G_connections[ci].ip);
    strcpy(req.hdr.method, G_connections[ci].method);
//    req.hdr.post = G_connections[ci].post;
    strcpy(req.hdr.uri, G_connections[ci].uri);
    strcpy(req.hdr.resource, G_connections[ci].resource);
#if NPP_RESOURCE_LEVELS > 1
    strcpy(req.hdr.req1, G_connections[ci].req1);
#if NPP_RESOURCE_LEVELS > 2
    strcpy(req.hdr.req2, G_connections[ci].req2);
#if NPP_RESOURCE_LEVELS > 3
    strcpy(req.hdr.req3, G_connections[ci].req3);
#if NPP_RESOURCE_LEVELS > 4
    strcpy(req.hdr.req4, G_connections[ci].req4);
#if NPP_RESOURCE_LEVELS > 5
    strcpy(req.hdr.req5, G_connections[ci].req5);
#endif  /* NPP_RESOURCE_LEVELS > 5 */
#endif  /* NPP_RESOURCE_LEVELS > 4 */
#endif  /* NPP_RESOURCE_LEVELS > 3 */
#endif  /* NPP_RESOURCE_LEVELS > 2 */
#endif  /* NPP_RESOURCE_LEVELS > 1 */
    strcpy(req.hdr.id, G_connections[ci].id);
    strcpy(req.hdr.uagent, G_connections[ci].uagent);
    req.hdr.ua_type = G_connections[ci].ua_type;
    strcpy(req.hdr.referer, G_connections[ci].referer);
    req.hdr.clen = G_connections[ci].clen;
    strcpy(req.hdr.in_cookie, G_connections[ci].in_cookie);
    strcpy(req.hdr.host, G_connections[ci].host);
    strcpy(req.hdr.app_name, G_connections[ci].app_name);
    strcpy(req.hdr.lang, G_connections[ci].lang);
    req.hdr.in_ctype = G_connections[ci].in_ctype;
    strcpy(req.hdr.boundary, G_connections[ci].boundary);
    req.hdr.status = G_connections[ci].status;
    strcpy(req.hdr.cust_headers, G_connections[ci].cust_headers);
    req.hdr.cust_headers_len = G_connections[ci].cust_headers_len;
    req.hdr.ctype = G_connections[ci].ctype;
    strcpy(req.hdr.ctypestr, G_connections[ci].ctypestr);
    strcpy(req.hdr.cdisp, G_connections[ci].cdisp);
    strcpy(req.hdr.cookie_out_a, G_connections[ci].cookie_out_a);
    strcpy(req.hdr.cookie_out_a_exp, G_connections[ci].cookie_out_a_exp);
    strcpy(req.hdr.cookie_out_l, G_connections[ci].cookie_out_l);
    strcpy(req.hdr.cookie_out_l_exp, G_connections[ci].cookie_out_l_exp);
    strcpy(req.hdr.location, G_connections[ci].location);
    req.hdr.si = G_connections[ci].si;
//    req.hdr.bot = G_connections[ci].bot;
//    req.hdr.dont_cache = G_connections[ci].dont_cache;
//    req.hdr.keep_content = G_connections[ci].keep_content;
    req.hdr.flags = G_connections[ci].flags;

    if ( want_response )
        req.hdr.async_flags = NPP_ASYNC_FLAG_WANT_RESPONSE;
    else
        req.hdr.async_flags = (char)0;

    /* For POST, the payload can be in the data space of the message,
       or -- if it's bigger -- in the shared memory */

    if ( NPP_CONN_IS_PAYLOAD(G_connections[ci].flags) )
    {
        if ( G_connections[ci].clen < G_async_req_data_size )
        {
            DBG("Payload fits in msg");

            memcpy(req.data, G_connections[ci].in_data, G_connections[ci].clen+1);
//            req.hdr.payload_location = ASYNC_PAYLOAD_MSG;
        }
        else    /* ASYNC_PAYLOAD_SHM */
        {
            DBG("Payload requires SHM");

            if ( !M_async_shm )
            {
                if ( (M_async_shm=lib_shm_create(NPP_MAX_PAYLOAD_SIZE, 0)) == NULL )
                {
                    ERR("Couldn't create SHM");
                    return;
                }

                M_async_shm[NPP_MAX_PAYLOAD_SIZE-1] = 0;
            }

            /* use the last byte as a simple semaphore */

            while ( M_async_shm[NPP_MAX_PAYLOAD_SIZE-1] )
            {
                WAR("Waiting for the SHM segment to be freed by npp_svc process");
                msleep(100);   /* temporarily */
            }

            memcpy(M_async_shm, G_connections[ci].in_data, G_connections[ci].clen+1);
            M_async_shm[NPP_MAX_PAYLOAD_SIZE-1] = 1;
//            req.hdr.payload_location = ASYNC_PAYLOAD_SHM;
            req.hdr.async_flags |= NPP_ASYNC_FLAG_PAYLOAD_IN_SHM;
        }
    }

    /* pass user session over */

    if ( IS_SESSION )
    {
        memcpy(&req.hdr.eng_session_data, &SESSION, sizeof(eng_session_data_t));
#ifdef NPP_ASYNC_INCLUDE_SESSION_DATA
        memcpy(&req.hdr.app_session_data, &SESSION_DATA, sizeof(app_session_data_t));
#endif
    }
    else    /* no session */
    {
        memset(&req.hdr.eng_session_data, 0, sizeof(eng_session_data_t));
#ifdef NPP_ASYNC_INCLUDE_SESSION_DATA
        memset(&req.hdr.app_session_data, 0, sizeof(app_session_data_t));
#endif
    }

    /* globals */

    memcpy(&req.hdr.cnts_today, &G_cnts_today, sizeof(npp_counters_t));
    memcpy(&req.hdr.cnts_yesterday, &G_cnts_yesterday, sizeof(npp_counters_t));
    memcpy(&req.hdr.cnts_day_before, &G_cnts_day_before, sizeof(npp_counters_t));

    req.hdr.days_up = G_days_up;
    req.hdr.connections_cnt = G_connections_cnt;
    req.hdr.connections_hwm = G_connections_hwm;
    req.hdr.sessions_cnt = G_sessions_cnt;
    req.hdr.sessions_hwm = G_sessions_hwm;

    req.hdr.blacklist_cnt = G_blacklist_cnt;

    strcpy(req.hdr.last_modified, G_last_modified);


    bool found=0;

    if ( want_response )     /* we will wait */
    {
        /* add to M_areqs (async response array) */

        int j;

        for ( j=0; j<ASYNC_MAX_REQUESTS; ++j )
        {
            if ( M_areqs[j].state == NPP_ASYNC_STATE_FREE )    /* free slot */
            {
                DBG("free slot %d found in M_areqs", j);
                M_areqs[j].ci = ci;
                M_areqs[j].state = NPP_ASYNC_STATE_SENT;
                M_areqs[j].sent = G_now;
                if ( timeout < 0 ) timeout = 0;
                if ( timeout == 0 || timeout > ASYNC_MAX_TIMEOUT ) timeout = ASYNC_MAX_TIMEOUT;
                M_areqs[j].timeout = timeout;
                req.hdr.ai = j;
                found = 1;
                break;
            }
        }

        if ( found )
        {
            /* set request state */

            DDBG("Changing state to CONN_STATE_WAITING_FOR_ASYNC");
            G_connections[ci].conn_state = CONN_STATE_WAITING_FOR_ASYNC;

#ifdef NPP_FD_MON_POLL
            M_pollfds[G_connections[ci].pi].events = POLLOUT;
#endif
        }
        else
        {
            ERR("M_areqs is full");
        }
    }

    if ( found || !want_response )
    {
        DBG("Sending a message on behalf of ci=%d, call_id=%u, service [%s]", ci, req.hdr.call_id, req.hdr.service);
        if ( mq_send(G_queue_req, (char*)&req, ASYNC_REQ_MSG_SIZE, 0) != 0 )
            ERR("mq_send failed, errno = %d (%s)", errno, strerror(errno));
    }

#endif
}


/* --------------------------------------------------------------------------
   Set internal (generated) static resource data & size
-------------------------------------------------------------------------- */
void npp_add_to_static_res(const char *name, const char *src)
{
    int i;

    i = first_free_stat();

    strcpy(M_stat[i].name, name);

    M_stat[i].len = strlen(src);   /* internal are text based */

    if ( NULL == (M_stat[i].data=(char*)malloc(M_stat[i].len+1+NPP_OUT_HEADER_BUFSIZE)) )
    {
        ERR("Couldn't allocate %u bytes for %s", M_stat[i].len+1+NPP_OUT_HEADER_BUFSIZE, M_stat[i].name);
        return;
    }

    strcpy(M_stat[i].data+NPP_OUT_HEADER_BUFSIZE, src);

    M_stat[i].type = npp_lib_get_res_type(M_stat[i].name);
    M_stat[i].modified = G_now;
    M_stat[i].source = STATIC_SOURCE_INTERNAL;

    INF("%s (%u bytes)", M_stat[i].name, M_stat[i].len);
}


/* --------------------------------------------------------------------------
   Add to blocked IP
-------------------------------------------------------------------------- */
void npp_eng_block_ip(const char *value, bool autoblocked)
{
    if ( G_blockedIPList[0] == EOS ) return;

    if ( G_blacklist_cnt > NPP_MAX_BLACKLIST-1 )
    {
        WAR("G_blacklist_cnt at max (%d)!", NPP_MAX_BLACKLIST);
        return;
    }

    if ( ip_blocked(value) )
    {
        DBG("%s already blocked", value);
        return;
    }

    char fname[NPP_STATIC_PATH_LEN+1];
    char command[NPP_STATIC_PATH_LEN*2];

    strcpy(G_blacklist[G_blacklist_cnt++], value);

    if ( G_blockedIPList[0] == '/' )    /* full path */
        strcpy(fname, G_blockedIPList);
    else if ( G_appdir[0] )
        sprintf(fname, "%s/bin/%s", G_appdir, G_blockedIPList);
    else
        strcpy(fname, G_blockedIPList);

    sprintf(command, "echo \"%s\t# %sblocked on %s\" >> %s", value, autoblocked?"auto":"", DT_NOW_GMT, fname);

    if ( system(command) != EXIT_SUCCESS )
        WAR("Couldn't execute %s", command);

    WAR("IP %s blacklisted", value);
}


/* --------------------------------------------------------------------------
   Add a host and assign resource directories
-------------------------------------------------------------------------- */
bool npp_add_host(const char *host, const char *res, const char *resmin, const char *snippets)
{
#ifdef NPP_MULTI_HOST

    if ( M_hosts_cnt >= NPP_MAX_HOSTS ) return FALSE;

    COPY(M_hosts[M_hosts_cnt].host, npp_upper(host), NPP_MAX_HOST_LEN);

    if ( res )
        COPY(M_hosts[M_hosts_cnt].res, res, 255);
    if ( resmin )
        COPY(M_hosts[M_hosts_cnt].resmin, resmin, 255);
    if ( snippets )
        COPY(M_hosts[M_hosts_cnt].snippets, snippets, 255);

    ++M_hosts_cnt;

#endif  /* NPP_MULTI_HOST */

    return TRUE;
}


/* --------------------------------------------------------------------------
   Return true if request URI matches uri

    uri    | request     | result
   --------+-------------+--------
    log    | log         | T
           | log?qs=1    | T
           | logout      | F
           | logout?qs=1 | F
   --------+-------------+--------
    logout | log         | F
           | log?qs=1    | F
           | logout      | T
           | logout?qs=1 | T
   --------+-------------+--------
    log*   | log         | T
           | log?qs=1    | T
           | logout      | T
           | logout?qs=1 | T
-------------------------------------------------------------------------- */
bool npp_eng_is_uri(int ci, const char *uri)
{
    const char *u = uri;

    if ( uri[0] == '/' )
        ++u;

    int len = strlen(u);

    if ( *(u+len-1) == '*' )
    {
        len--;
        return (0==strncmp(G_connections[ci].uri, u, len));
    }
    else if ( len > 4
                        && *(u+len-4)=='{'
                        && (*(u+len-3)=='i' || *(u+len-3)=='I')
                        && (*(u+len-2)=='d' || *(u+len-2)=='D')
                        && *(u+len-1)=='}' )
    {
        len -= 4;
        return (0==strncmp(G_connections[ci].uri, u, len));
    }

    /* ------------------------------------------------------------------- */
    /* no wildcard ==> exact match is required, but excluding query string */

    char *q = strchr(G_connections[ci].uri, '?');

    if ( !q )
        return (0==strcmp(G_connections[ci].uri, u));

    /* there's a query string */

    int req_len = q - G_connections[ci].uri;

    if ( req_len != len )
        return FALSE;

    return (0==strncmp(G_connections[ci].uri, u, len));
}


/* --------------------------------------------------------------------------
   Write string to output buffer with buffer overwrite protection
-------------------------------------------------------------------------- */
void npp_eng_out_check(int ci, const char *str)
{
    int available = NPP_OUT_BUFSIZE - (G_connections[ci].p_content - G_connections[ci].out_data);

    if ( strlen(str) < available )  /* the whole string will fit */
    {
        G_connections[ci].p_content = stpcpy(G_connections[ci].p_content, str);
    }
    else    /* let's write only what we can. WARNING: no UTF-8 checking is done here! */
    {
        G_connections[ci].p_content = stpncpy(G_connections[ci].p_content, str, available-1);
        *G_connections[ci].p_content = EOS;
    }
}


/* --------------------------------------------------------------------------
   Write string to output buffer with buffer resizing if necessary
-------------------------------------------------------------------------- */
void npp_eng_out_check_realloc(int ci, const char *str)
{
    if ( strlen(str) < G_connections[ci].out_data_allocated - (G_connections[ci].p_content-G_connections[ci].out_data) )    /* the whole string will fit */
    {
        G_connections[ci].p_content = stpcpy(G_connections[ci].p_content, str);
    }
    else    /* resize output buffer and try again */
    {
        unsigned used = G_connections[ci].p_content - G_connections[ci].out_data;
        char *tmp = (char*)realloc(G_connections[ci].out_data_alloc, G_connections[ci].out_data_allocated*2);
        if ( !tmp )
        {
            ERR("Couldn't reallocate output buffer for ci=%d, tried %u bytes", ci, G_connections[ci].out_data_allocated*2);
            return;
        }
        G_connections[ci].out_data_alloc = tmp;
        G_connections[ci].out_data = G_connections[ci].out_data_alloc;
        G_connections[ci].out_data_allocated = G_connections[ci].out_data_allocated * 2;
        G_connections[ci].p_content = G_connections[ci].out_data + used;
        INF("Reallocated output buffer for ci=%d, new size = %u bytes", ci, G_connections[ci].out_data_allocated);
        npp_eng_out_check_realloc(ci, str);     /* call itself! */
    }
}


/* --------------------------------------------------------------------------
   Write binary data to output buffer with buffer resizing if necessary
-------------------------------------------------------------------------- */
void npp_eng_out_check_realloc_bin(int ci, const char *data, int len)
{
    if ( len < G_connections[ci].out_data_allocated - (G_connections[ci].p_content - G_connections[ci].out_data) )    /* the whole data will fit */
    {
        memcpy(G_connections[ci].p_content, data, len);
        G_connections[ci].p_content += len;
    }
    else    /* resize output buffer and try again */
    {
        unsigned used = G_connections[ci].p_content - G_connections[ci].out_data;
        char *tmp = (char*)realloc(G_connections[ci].out_data_alloc, G_connections[ci].out_data_allocated*2);
        if ( !tmp )
        {
            ERR("Couldn't reallocate output buffer for ci=%d, tried %u bytes", ci, G_connections[ci].out_data_allocated*2);
            return;
        }
        G_connections[ci].out_data_alloc = tmp;
        G_connections[ci].out_data = G_connections[ci].out_data_alloc;
        G_connections[ci].out_data_allocated = G_connections[ci].out_data_allocated * 2;
        G_connections[ci].p_content = G_connections[ci].out_data + used;
        INF("Reallocated output buffer for ci=%d, new size = %u bytes", ci, G_connections[ci].out_data_allocated);
        npp_eng_out_check_realloc_bin(ci, data, len);       /* call itself! */
    }
}


/* --------------------------------------------------------------------------
   Return request header value
-------------------------------------------------------------------------- */
char *npp_eng_get_header(int ci, const char *header)
{
static char value[NPP_MAX_VALUE_LEN+1];
    char uheader[NPP_MAX_LABEL_LEN+1];

    strcpy(uheader, npp_upper(header));

    if ( 0==strcmp(uheader, "CONTENT-TYPE") )
    {
        strcpy(value, G_connections[ci].in_ctypestr);
        return value;
    }
    else if ( 0==strcmp(uheader, "AUTHORIZATION") )
    {
        strcpy(value, G_connections[ci].authorization);
        return value;
    }
    else if ( 0==strcmp(uheader, "COOKIE") )
    {
        strcpy(value, G_connections[ci].in_cookie);
        return value;
    }
    else
    {
        return NULL;
    }
}


/* --------------------------------------------------------------------------
   HTTP calls -- pass request header value from the original request
-------------------------------------------------------------------------- */
void npp_eng_call_http_pass_header(int ci, const char *key)
{
    char value[NPP_MAX_VALUE_LEN+1];

    strcpy(value, npp_eng_get_header(ci, key));

    if ( value[0] )
        CALL_HTTP_HEADER_SET(key, value);
}