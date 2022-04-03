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
    nodepp.org

-------------------------------------------------------------------------- */

#ifndef NPP_H
#define NPP_H


/* --------------------------------------------------------------------------
   includes
-------------------------------------------------------------------------- */

#ifdef _WIN32   /* Windows */
#ifdef _MSC_VER /* Microsoft compiler */
/* libraries */
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "psapi.lib")   /* GetProcessMemoryInfo */
/* __VA_ARGS__ issue */
#define EXPAND_VA(x) x
/* access function */
#define F_OK    0       /* test for existence of file */
#define X_OK    0x01    /* test for execute or search permission */
#define W_OK    0x02    /* test for write permission */
#define R_OK    0x04    /* test for read permission */
#endif  /* _MSC_VER */
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_VISTA  /* Windows Vista or higher required */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <psapi.h>
#define CLOCK_MONOTONIC 0   /* dummy */
#undef OUT
#endif  /* _WIN32 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>   /* struct timeval */
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>     /* INT_MAX */


#ifndef _WIN32
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <netdb.h>
#include <sys/shm.h>
#endif  /* _WIN32 */

#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>

#ifdef __cplusplus
#include <iostream>
#include <cctype>
#else   /* C */
#include <ctype.h>
typedef char                            bool;
#define false                           (char)0
#define true                            (char)1
#endif  /* __cplusplus */



/* --------------------------------------------------------------------------
   macros
-------------------------------------------------------------------------- */

#define NPP_VERSION                     "2.1.0"


#ifndef FALSE
#define FALSE                           false
#endif
#ifndef TRUE
#define TRUE                            true
#endif


/* basics */

#define OK                              0
#define SUCCEED                         OK
#define FAIL                            -1

#ifndef EOS
#define EOS                             (char)0         /* End Of String */
#endif


#ifdef __linux__
#define MONOTONIC_CLOCK_NAME            CLOCK_MONOTONIC_RAW
#else
#define MONOTONIC_CLOCK_NAME            CLOCK_MONOTONIC
#endif


/* these can be useful in npp_app.h */

#define MAX_URI_VAL_LEN                 255             /* max value length received in URI -- sufficient for 99.99% cases */
#define MAX_LONG_URI_VAL_LEN            65535           /* max long value length received in URI -- 64 KiB - 1 B */

#define NPP_QSBUF                       MAX_URI_VAL_LEN+1
#define NPP_QSBUF_TEXT                  MAX_LONG_URI_VAL_LEN+1

#define NPP_LOGIN_LEN                   30
#define NPP_EMAIL_LEN                   120
#define NPP_UNAME_LEN                   120
#define NPP_PHONE_LEN                   30
#define NPP_ABOUT_LEN                   250

#define NPP_LANG_LEN                    5


#define NPP_SQLBUF                      4096            /* SQL query buffer size */


/* Silgy compatibility */

#define LOGIN_LEN                       NPP_LOGIN_LEN
#define EMAIL_LEN                       NPP_EMAIL_LEN
#define UNAME_LEN                       NPP_UNAME_LEN
#define PHONE_LEN                       NPP_PHONE_LEN
#define ABOUT_LEN                       NPP_ABOUT_LEN
#define LANG_LEN                        NPP_LANG_LEN
#define SQLBUF                          NPP_SQLBUF


/* executable types */

#ifndef NPP_APP
#ifndef NPP_SVC
#ifndef NPP_CLIENT
#define NPP_CLIENT          /* default executable type */
#endif
#endif
#endif


/* Query String Value */

typedef char                            QSVAL[NPP_QSBUF];
typedef char                            QSVAL_TEXT[NPP_QSBUF_TEXT];


/* application settings */

#include "npp_app.h"


/* socket monitoring method */

#ifdef _WIN32           /* Windows ----------------------------------------------------------- */
    #define NPP_FD_MON_SELECT   /* default (WSAPoll doesn't seem to be a reliable alternative) */
#elif defined __linux__ /* Linux ------------------------------------------------------------- */
    #ifdef NPP_FD_MON_LINUX_SELECT
        #define NPP_FD_MON_SELECT
    #elif defined NPP_FD_MON_LINUX_POLL
        #define NPP_FD_MON_POLL
    #else
        #define NPP_FD_MON_EPOLL /* default */
    #endif
#else                   /* macOS & other Unixes ---------------------------------------------- */
    #ifdef NPP_FD_MON_OTHERS_SELECT
        #define NPP_FD_MON_SELECT
    #elif defined NPP_FD_MON_OTHERS_EPOLL
        #define NPP_FD_MON_EPOLL
    #else
        #define NPP_FD_MON_POLL /* default */
    #endif
#endif


/* Silgy compatibility */

#ifdef NPP_SILGY_COMPATIBILITY
#include "npp_silgy.h"
#endif


/* C++ only */

#ifdef NPP_CPP_STRINGS
#ifndef __cplusplus
#undef NPP_CPP_STRINGS
#endif
#endif


#ifdef _WIN32
#ifdef NPP_ASYNC
#undef NPP_ASYNC
#endif
#endif  /* _WIN32 */


#ifdef __APPLE__
#ifdef NPP_ASYNC
#undef NPP_ASYNC
#endif
#endif  /* __APPLE__ */


/* some executable types can't use or don't need certain modules */

#ifdef NPP_CLIENT
#ifdef NPP_USERS
#undef NPP_USERS
#endif
#ifdef NPP_ASYNC
#undef NPP_ASYNC
#endif
#endif  /* NPP_CLIENT */


#ifdef NPP_WATCHER  /* only basic support */
#ifdef NPP_HTTPS
#undef NPP_HTTPS
#endif
#ifdef NPP_MYSQL
#undef NPP_MYSQL
#endif
#ifdef NPP_ICONV
#undef NPP_ICONV
#endif
#endif  /* NPP_WATCHER */


#ifdef NPP_UPDATE
#ifdef NPP_MYSQL
#undef NPP_MYSQL
#endif
#ifdef NPP_ICONV
#undef NPP_ICONV
#endif
#endif  /* NPP_UPDATE */



#ifdef NPP_HTTPS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif


#ifdef NPP_ASYNC
#include <mqueue.h>
#endif


#ifdef NPP_MYSQL
#include <mysql.h>
#include <mysqld_error.h>
#ifdef __cplusplus
#include "npp_mysql.h"
#endif
#endif  /* NPP_MYSQL */


#define NPP_APP_NAME_LEN                63
#define NPP_CONTENT_TYPE_LEN            63
#define NPP_CONTENT_DISP_LEN            127


/* web client types (user agent) */

#define NPP_UA_TYPE_DSK                 (char)0         /* desktop (large screen) */
#define NPP_UA_TYPE_TAB                 (char)1         /* tablet (medium screen) */
#define NPP_UA_TYPE_MOB                 (char)2         /* phone (small screen) */


#define NPP_PROTOCOL                    (NPP_CONN_IS_SECURE(G_connections[ci].flags)?"https":"http")


/* defaults */

#ifndef NPP_MEM_TINY
#ifndef NPP_MEM_MEDIUM
#ifndef NPP_MEM_LARGE
#ifndef NPP_MEM_XLARGE
#ifndef NPP_MEM_XXLARGE
#ifndef NPP_MEM_XXXLARGE
#ifndef NPP_MEM_XXXXLARGE
#ifndef NPP_MEM_XXXXXLARGE
#ifndef NPP_MEM_XXXXXXLARGE
#ifndef NPP_MEM_SMALL
#define NPP_MEM_SMALL                   /* default memory model */
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

#ifndef NPP_OUT_FAST
#ifndef NPP_OUT_CHECK
#define NPP_OUT_CHECK_REALLOC           /* default output type */
#endif
#endif

#ifndef QS_DEF_SQL_ESCAPE
#ifndef QS_DEF_DONT_ESCAPE
#define QS_DEF_HTML_ESCAPE              /* default query string security */
#endif
#endif


/* response headers */

/* HTTP header -- resets respbuf! */
#define PRINT_HTTP_STATUS(val)              (sprintf(G_tmp, "HTTP/1.1 %d %s\r\n", val, get_http_descr(val)), HOUT(G_tmp))
#define PRINT_HTTP2_STATUS(val)             http2_hdr_status(ci, val)

/* date */
#define PRINT_HTTP_DATE                     (sprintf(G_tmp, "Date: %s\r\n", G_header_date), HOUT(G_tmp))
#define PRINT_HTTP2_DATE                    http2_hdr_date(ci)

/* redirection */
#define PRINT_HTTP_LOCATION                 (sprintf(G_tmp, "Location: %s\r\n", G_connections[ci].location), HOUT(G_tmp))
#define PRINT_HTTP2_LOCATION                http2_hdr_location(ci)

/* content type */
#define PRINT_HTTP_CONTENT_TYPE(val)        (sprintf(G_tmp, "Content-Type: %s\r\n", val), HOUT(G_tmp))
#define PRINT_HTTP2_CONTENT_TYPE(val)       http2_hdr_content_type(ci, val)

/* content disposition */
#define PRINT_HTTP_CONTENT_DISP(val)        (sprintf(G_tmp, "Content-Disposition: %s\r\n", val), HOUT(G_tmp))
#define PRINT_HTTP2_CONTENT_DISP(val)       http2_hdr_content_disp(ci, val)

/* cache control */
#define PRINT_HTTP_CACHE_PUBLIC             HOUT("Cache-Control: public, max-age=31536000\r\n")
#define PRINT_HTTP2_CACHE_PUBLIC            http2_hdr_cache_ctrl_public(ci)

#define PRINT_HTTP_NO_CACHE                 HOUT("Cache-Control: private, must-revalidate, no-store, no-cache, max-age=0\r\n")
#define PRINT_HTTP2_NO_CACHE                http2_hdr_cache_ctrl_private(ci)

#define PRINT_HTTP_EXPIRES_STATICS          (sprintf(G_tmp, "Expires: %s\r\n", M_expires_stat), HOUT(G_tmp))
#define PRINT_HTTP2_EXPIRES_STATICS         http2_hdr_expires_statics(ci)

#define PRINT_HTTP_EXPIRES_GENERATED        (sprintf(G_tmp, "Expires: %s\r\n", M_expires_gen), HOUT(G_tmp))
#define PRINT_HTTP2_EXPIRES_GENERATED       http2_hdr_expires_gen(ci)

#define PRINT_HTTP_LAST_MODIFIED(val)       (sprintf(G_tmp, "Last-Modified: %s\r\n", val), HOUT(G_tmp))
#define PRINT_HTTP2_LAST_MODIFIED(val)      http2_hdr_last_modified(ci, val)

/* connection */
#define PRINT_HTTP_CONNECTION               (sprintf(G_tmp, "Connection: %s\r\n", NPP_CONN_IS_KEEP_ALIVE(G_connections[ci].flags)?"keep-alive":"close"), HOUT(G_tmp))

/* vary */
#define PRINT_HTTP_VARY_DYN                 HOUT("Vary: Accept-Encoding, User-Agent\r\n")
#define PRINT_HTTP2_VARY_DYN                http2_hdr_vary(ci, "Accept-Encoding, User-Agent")

#define PRINT_HTTP_VARY_STAT                HOUT("Vary: Accept-Encoding\r\n")
#define PRINT_HTTP2_VARY_STAT               http2_hdr_vary(ci, "Accept-Encoding")

#define PRINT_HTTP_VARY_UIR                 HOUT("Vary: Upgrade-Insecure-Requests\r\n")
#define PRINT_HTTP2_VARY_UIR                http2_hdr_vary(ci, "Upgrade-Insecure-Requests")

/* content language */
#define PRINT_HTTP_LANGUAGE(val)            (sprintf(G_tmp, "Content-Language: %s\r\n", val), HOUT(G_tmp))
#define PRINT_HTTP2_LANGUAGE(val)           http2_hdr_content_lang(ci, val)

/* content length */
#define PRINT_HTTP_CONTENT_LEN(len)         (sprintf(G_tmp, "Content-Length: %u\r\n", len), HOUT(G_tmp))
#define PRINT_HTTP2_CONTENT_LEN(len)        http2_hdr_content_len(ci, len)

/* content encoding */
#define PRINT_HTTP_CONTENT_ENCODING_DEFLATE HOUT("Content-Encoding: deflate\r\n")
#define PRINT_HTTP2_CONTENT_ENCODING_DEFLATE http2_hdr_content_enc_deflate(ci)

/* Security ------------------------------------------------------------------ */

/* HSTS */

#ifndef NPP_HSTS_MAX_AGE
#define NPP_HSTS_MAX_AGE                    31536000    /* a year */
#endif

#ifdef NPP_HSTS_INCLUDE_SUBDOMAINS
#define PRINT_HTTP_HSTS                     (sprintf(G_tmp, "Strict-Transport-Security: max-age=%d; includesubdomains\r\n", NPP_HSTS_MAX_AGE), HOUT(G_tmp))
#define PRINT_HTTP2_HSTS
#else
#define PRINT_HTTP_HSTS                     (sprintf(G_tmp, "Strict-Transport-Security: max-age=%d\r\n", NPP_HSTS_MAX_AGE), HOUT(G_tmp))
#define PRINT_HTTP2_HSTS
#endif

#ifdef NPP_HTTPS
#ifndef NPP_NO_HSTS
#if NPP_HSTS_MAX_AGE > 0
#define NPP_HSTS_ON
#endif
#endif
#endif  /* NPP_HTTPS */

/* cookie */

#ifdef NPP_HSTS_ON

#define PRINT_HTTP_COOKIE_A(ci)             (sprintf(G_tmp, "Set-Cookie: as=%s; %shttponly\r\n", G_connections[ci].cookie_out_a, G_test?"":"secure; "), HOUT(G_tmp))
#define PRINT_HTTP2_COOKIE_A(ci)            (sprintf(G_tmp, "as=%s; %shttponly", G_connections[ci].cookie_out_a, G_test?"":"secure; "), http2_hdr_set_cookie(ci, G_tmp))

#define PRINT_HTTP_COOKIE_L(ci)             (sprintf(G_tmp, "Set-Cookie: ls=%s; %shttponly\r\n", G_connections[ci].cookie_out_l, G_test?"":"secure; "), HOUT(G_tmp))
#define PRINT_HTTP2_COOKIE_L(ci)            (sprintf(G_tmp, "ls=%s; %shttponly", G_connections[ci].cookie_out_l, G_test?"":"secure; "), http2_hdr_set_cookie(ci, G_tmp))

#define PRINT_HTTP_COOKIE_A_EXP(ci)         (sprintf(G_tmp, "Set-Cookie: as=%s; expires=%s; %shttponly\r\n", G_connections[ci].cookie_out_a, G_connections[ci].cookie_out_a_exp, G_test?"":"secure; "), HOUT(G_tmp))
#define PRINT_HTTP2_COOKIE_A_EXP(ci)        (sprintf(G_tmp, "as=%s; expires=%s; %shttponly", G_connections[ci].cookie_out_a, G_connections[ci].cookie_out_a_exp, G_test?"":"secure; "), http2_hdr_set_cookie(ci, G_tmp))

#define PRINT_HTTP_COOKIE_L_EXP(ci)         (sprintf(G_tmp, "Set-Cookie: ls=%s; expires=%s; %shttponly\r\n", G_connections[ci].cookie_out_l, G_connections[ci].cookie_out_l_exp, G_test?"":"secure; "), HOUT(G_tmp))
#define PRINT_HTTP2_COOKIE_L_EXP(ci)        (sprintf(G_tmp, "ls=%s; expires=%s; %shttponly", G_connections[ci].cookie_out_l, G_connections[ci].cookie_out_l_exp, G_test?"":"secure; "), http2_hdr_set_cookie(ci, G_tmp))

#else   /* HSTS is off */

#define PRINT_HTTP_COOKIE_A(ci)             (sprintf(G_tmp, "Set-Cookie: as=%s; httponly\r\n", G_connections[ci].cookie_out_a), HOUT(G_tmp))
#define PRINT_HTTP2_COOKIE_A(ci)            (sprintf(G_tmp, "as=%s; httponly", G_connections[ci].cookie_out_a), http2_hdr_set_cookie(ci, G_tmp))

#define PRINT_HTTP_COOKIE_L(ci)             (sprintf(G_tmp, "Set-Cookie: ls=%s; httponly\r\n", G_connections[ci].cookie_out_l), HOUT(G_tmp))
#define PRINT_HTTP2_COOKIE_L(ci)            (sprintf(G_tmp, "ls=%s; httponly", G_connections[ci].cookie_out_l), http2_hdr_set_cookie(ci, G_tmp))

#define PRINT_HTTP_COOKIE_A_EXP(ci)         (sprintf(G_tmp, "Set-Cookie: as=%s; expires=%s; httponly\r\n", G_connections[ci].cookie_out_a, G_connections[ci].cookie_out_a_exp), HOUT(G_tmp))
#define PRINT_HTTP2_COOKIE_A_EXP(ci)        (sprintf(G_tmp, "as=%s; expires=%s; httponly", G_connections[ci].cookie_out_a, G_connections[ci].cookie_out_a_exp), http2_hdr_set_cookie(ci, G_tmp))

#define PRINT_HTTP_COOKIE_L_EXP(ci)         (sprintf(G_tmp, "Set-Cookie: ls=%s; expires=%s; httponly\r\n", G_connections[ci].cookie_out_l, G_connections[ci].cookie_out_l_exp), HOUT(G_tmp))
#define PRINT_HTTP2_COOKIE_L_EXP(ci)        (sprintf(G_tmp, "ls=%s; expires=%s; httponly", G_connections[ci].cookie_out_l, G_connections[ci].cookie_out_l_exp), http2_hdr_set_cookie(ci, G_tmp))

#endif  /* NPP_HSTS_ON */

/* framing */
#define PRINT_HTTP_SAMEORIGIN               HOUT("X-Frame-Options: SAMEORIGIN\r\n")
#define PRINT_HTTP2_SAMEORIGIN              //http2_hdr_sameorigin(ci)

/* content type guessing */
#define PRINT_HTTP_NOSNIFF                  HOUT("X-Content-Type-Options: nosniff\r\n")
#define PRINT_HTTP2_NOSNIFF                 //http2_hdr_nosniff(ci)

/* identity */
#define PRINT_HTTP_SERVER                   HOUT("Server: Node++\r\n")
#define PRINT_HTTP2_SERVER                  http2_hdr_server(ci)

/* HTTP/2 */
#define PRINT_HTTP2_UPGRADE_CLEAR           HOUT("Connection: Upgrade\r\nUpgrade: h2c\r\n")

/* must be last! */
#define PRINT_HTTP_END_OF_HEADER            HOUT("\r\n")


#define NPP_HTTP2_ALPHA                     NPP_HTTP2


#define HTTP2_CLIENT_PREFACE                "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"

/* The server connection preface consists of a potentially empty
   SETTINGS frame that MUST be the first frame the server
   sends in the HTTP/2 connection. */

/* The SETTINGS frames received from a peer as part of the connection
   preface MUST be acknowledged after sending the
   connection preface. */

#define HTTP2_MAX_ENC_INT_LEN               8
#define HTTP2_INT_PREFIX_BITS               5

#define HTTP2_FRAME_HDR_LEN                 9

#define HTTP2_FRAME_TYPE_DATA               (char)0x0
#define HTTP2_FRAME_TYPE_HEADERS            (char)0x1
#define HTTP2_FRAME_TYPE_PRIORITY           (char)0x2
#define HTTP2_FRAME_TYPE_RST_STREAM         (char)0x3
#define HTTP2_FRAME_TYPE_SETTINGS           (char)0x4       /* SETTINGS frames always apply to a connection, never a single stream. */
#define HTTP2_FRAME_TYPE_PUSH_PROMISE       (char)0x5
#define HTTP2_FRAME_TYPE_PING               (char)0x6
#define HTTP2_FRAME_TYPE_GOAWAY             (char)0x7
#define HTTP2_FRAME_TYPE_WINDOW_UPDATE      (char)0x8
#define HTTP2_FRAME_TYPE_CONTINUATION       (char)0x9

#define HTTP2_FRAME_FLAG_ACK                (char)0x1       /* for SETTINGS & PING frames */
#define HTTP2_FRAME_FLAG_END_STREAM         (char)0x1
#define HTTP2_FRAME_FLAG_END_HEADERS        (char)0x4
#define HTTP2_FRAME_FLAG_PADDED             (char)0x8
#define HTTP2_FRAME_FLAG_PRIORITY           (char)0x20

#define HTTP2_SETTINGS_HEADER_TABLE_SIZE        0x1         /* default: 4,096 */
#define HTTP2_SETTINGS_ENABLE_PUSH              0x2         /* default: 1 */
#define HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS   0x3         /* default: unlimited */
#define HTTP2_SETTINGS_INITIAL_WINDOW_SIZE      0x4         /* default: 65,535 */
#define HTTP2_SETTINGS_MAX_FRAME_SIZE           0x5         /* default: 16,384 */
#define HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE     0x6         /* default: unlimited */

#define HTTP2_NO_ERROR                      0x0             /* Graceful shutdown */
#define HTTP2_PROTOCOL_ERROR                0x1             /* Protocol error detected */
#define HTTP2_INTERNAL_ERROR                0x2             /* Implementation fault */
#define HTTP2_FLOW_CONTROL_ERROR            0x3             /* Flow-control limits exceeded */
#define HTTP2_SETTINGS_TIMEOUT              0x4             /* Settings not acknowledged */
#define HTTP2_STREAM_CLOSED                 0x5             /* Frame received for closed stream */
#define HTTP2_FRAME_SIZE_ERROR              0x6             /* Frame size incorrect */
#define HTTP2_REFUSED_STREAM                0x7             /* Stream not processed */
#define HTTP2_CANCEL                        0x8             /* Stream cancelled */
#define HTTP2_COMPRESSION_ERROR             0x9             /* Compression state not updated */
#define HTTP2_CONNECT_ERROR                 0xA             /* TCP connection error for CONNECT method */
#define HTTP2_ENHANCE_YOUR_CALM             0xB             /* Processing capacity exceeded */
#define HTTP2_INADEQUATE_SECURITY           0xC             /* Negotiated TLS parameters not acceptable */
#define HTTP2_HTTP_1_1_REQUIRED             0xD             /* Use HTTP/1.1 for the request */

#define HTTP2_HDR_AUTHORITY                 1
#define HTTP2_HDR_METHOD_GET                2
#define HTTP2_HDR_METHOD_POST               3
#define HTTP2_HDR_PATH_LANDING              4
#define HTTP2_HDR_PATH_INDEX                5
#define HTTP2_HDR_SCHEME_HTTP               6
#define HTTP2_HDR_SCHEME_HTTPS              7
#define HTTP2_HDR_STATUS_200                8
#define HTTP2_HDR_STATUS_204                9
#define HTTP2_HDR_STATUS_206                10
#define HTTP2_HDR_STATUS_304                11
#define HTTP2_HDR_STATUS_400                12
#define HTTP2_HDR_STATUS_404                13
#define HTTP2_HDR_STATUS_500                14
#define HTTP2_HDR_ACCEPT_CHARSET            15
#define HTTP2_HDR_ACCEPT_ENCODING           16
#define HTTP2_HDR_ACCEPT_LANGUAGE           17
#define HTTP2_HDR_ACCEPT_RANGES             18
#define HTTP2_HDR_ACCEPT                    19
#define HTTP2_HDR_AC_ALLOW_ORIGIN           20
#define HTTP2_HDR_AGE                       21
#define HTTP2_HDR_ALLOW                     22
#define HTTP2_HDR_AUTHORIZATION             23
#define HTTP2_HDR_CACHE_CONTROL             24
#define HTTP2_HDR_CONTENT_DISPOSITION       25
#define HTTP2_HDR_CONTENT_ENCODING          26
#define HTTP2_HDR_CONTENT_LANGUAGE          27
#define HTTP2_HDR_CONTENT_LENGTH            28
#define HTTP2_HDR_CONTENT_LOCATION          29
#define HTTP2_HDR_CONTENT_RANGE             30
#define HTTP2_HDR_CONTENT_TYPE              31
#define HTTP2_HDR_COOKIE                    32
#define HTTP2_HDR_DATE                      33
#define HTTP2_HDR_ETAG                      34
#define HTTP2_HDR_EXPECT                    35
#define HTTP2_HDR_EXPIRES                   36
#define HTTP2_HDR_FROM                      37
#define HTTP2_HDR_HOST                      38
#define HTTP2_HDR_IF_MATCH                  39
#define HTTP2_HDR_IF_MODIFIED_SINCE         40
#define HTTP2_HDR_IF_NONE_MATCH             41
#define HTTP2_HDR_IF_RANGE                  42
#define HTTP2_HDR_IF_UNMODIFIED_SINCE       43
#define HTTP2_HDR_LAST_MODIFIED             44
#define HTTP2_HDR_LINK                      45
#define HTTP2_HDR_LOCATION                  46
#define HTTP2_HDR_MAX_FORWARDS              47
#define HTTP2_HDR_PROXY_AUTHENTICATE        48
#define HTTP2_HDR_PROXY_AUTHORIZATION       49
#define HTTP2_HDR_RANGE                     50
#define HTTP2_HDR_REFERER                   51
#define HTTP2_HDR_REFRESH                   52
#define HTTP2_HDR_RETRY_AFTER               53
#define HTTP2_HDR_SERVER                    54
#define HTTP2_HDR_SET_COOKIE                55
#define HTTP2_HDR_STRICT_TRANSPORT_SECURITY 56
#define HTTP2_HDR_TRANSFER_ENCODING         57
#define HTTP2_HDR_USER_AGENT                58
#define HTTP2_HDR_VARY                      59
#define HTTP2_HDR_VIA                       60
#define HTTP2_HDR_WWW_AUTHENTICATE          61

#define HTTP2_DEFAULT_FRAME_SIZE            16384

#define HTTP2_SETTINGS_LEN                  NPP_MAX_VALUE_LEN



#define NPP_OUT_HEADER_BUFSIZE              4096            /* response header buffer length */


#define NPP_METHOD_LEN                      7               /* HTTP method length */


/* authorization levels */

#define AUTH_LEVEL_NONE                     (char)0         /* no session */
#define AUTH_LEVEL_ANONYMOUS                (char)1         /* anonymous session */
#define AUTH_LEVEL_LOGGED                   (char)2         /* logged in session with lowest authorization level */
#define AUTH_LEVEL_LOGGEDIN                 (char)2
#define AUTH_LEVEL_AUTHENTICATED            (char)2
#define AUTH_LEVEL_USER                     (char)10
#define AUTH_LEVEL_CUSTOMER                 (char)20
#define AUTH_LEVEL_STAFF                    (char)30
#define AUTH_LEVEL_MODERATOR                (char)40
#define AUTH_LEVEL_ADMIN                    (char)50
#define AUTH_LEVEL_ROOT                     (char)100
#define AUTH_LEVEL_NOBODY                   (char)125       /* for resources' whitelisting */

#define CUSTOMER                            (SESSION.auth_level==AUTH_LEVEL_CUSTOMER)
#define STAFF                               (SESSION.auth_level==AUTH_LEVEL_STAFF)
#define MODERATOR                           (SESSION.auth_level==AUTH_LEVEL_MODERATOR)
#define ADMIN                               (SESSION.auth_level==AUTH_LEVEL_ADMIN)
#define ROOT                                (SESSION.auth_level==AUTH_LEVEL_ROOT)

#define LOGGED                              (SESSION.auth_level>AUTH_LEVEL_ANONYMOUS)
#define LOGGEDIN                            (SESSION.auth_level>AUTH_LEVEL_ANONYMOUS)
#define AUTHENTICATED                       (SESSION.auth_level>AUTH_LEVEL_ANONYMOUS)
#define UID                                 SESSION.user_id


/* backward compatibility */

#ifdef NPP_DEFAULT_REQUIRED_AUTH_LEVEL
#define NPP_REQUIRED_AUTH_LEVEL             NPP_DEFAULT_REQUIRED_AUTH_LEVEL
#endif


/* API authorization flags */

#define AUTH_NONE                           0x00
#define AUTH_CREATE                         0x01
#define AUTH_READ                           0x02
#define AUTH_UPDATE                         0x04
#define AUTH_DELETE                         0x08
#define AUTH_FULL                           0xFF

#define IS_AUTH_CREATE(flags)               (((flags) & AUTH_CREATE) == AUTH_CREATE)
#define IS_AUTH_READ(flags)                 (((flags) & AUTH_READ) == AUTH_READ)
#define IS_AUTH_UPDATE(flags)               (((flags) & AUTH_UPDATE) == AUTH_UPDATE)
#define IS_AUTH_DELETE(flags)               (((flags) & AUTH_DELETE) == AUTH_DELETE)



/* APP-configurable settings */

#ifndef NPP_APP_NAME
#define NPP_APP_NAME                        "Node++ Web Application"
#endif

#ifndef NPP_APP_DOMAIN
#define NPP_APP_DOMAIN                      ""
#endif

#ifndef NPP_RESOURCE_LEVELS
#define NPP_RESOURCE_LEVELS                 3
#endif

#ifndef NPP_LOGIN_URI
#define NPP_LOGIN_URI                       "/login"
#endif

#ifndef NPP_CUST_HDR_LEN
#define NPP_CUST_HDR_LEN                    255
#endif

#ifndef NPP_MAX_HOST_LEN
#define NPP_MAX_HOST_LEN                    127             /* current request Host header */
#endif

#ifndef NPP_MAX_HOSTS
#define NPP_MAX_HOSTS                       15              /* G_hosts size for NPP_MULTI_HOST */
#endif

#ifndef NPP_MAX_PAYLOAD_SIZE
#define NPP_MAX_PAYLOAD_SIZE                16777216        /* max incoming payload data length (16 MiB) */
#endif

#ifndef NPP_MAX_LOG_STR_LEN
#define NPP_MAX_LOG_STR_LEN                 4095            /* max log string length */
#endif

#ifndef NPP_MAX_URI_LEN
#define NPP_MAX_URI_LEN                     2047            /* max request URI length */
#endif

#ifndef NPP_MAX_LABEL_LEN
#define NPP_MAX_LABEL_LEN                   255             /* max request header label length */
#endif

#ifndef NPP_MAX_VALUE_LEN
#define NPP_MAX_VALUE_LEN                   255             /* max request header value length */
#endif

#ifndef NPP_MAX_RESOURCE_LEN
#define NPP_MAX_RESOURCE_LEN                127             /* max resource's name length -- as a first part of URI */
#endif

#ifndef NPP_MAX_RESOURCES
#define NPP_MAX_RESOURCES                   1000            /* for M_auth_levels */
#endif

#ifndef NPP_CONNECTION_TIMEOUT
#define NPP_CONNECTION_TIMEOUT              180             /* idle connection timeout in seconds */
#endif

#ifndef NPP_SESSION_TIMEOUT
#define NPP_SESSION_TIMEOUT                 600             /* anonymous session timeout in seconds */
#endif

#ifndef NPP_MAX_BLACKLIST
#define NPP_MAX_BLACKLIST                   10000           /* IP blacklist length */
#endif

#ifndef NPP_MAX_WHITELIST
#define NPP_MAX_WHITELIST                   1000            /* IP whitelist length */
#endif

#ifndef NPP_SESSID_LEN
#define NPP_SESSID_LEN                      15              /* session id length (15 gives ~ 89 bits of entropy) */
#endif

#ifndef NPP_CSRFT_LEN
#define NPP_CSRFT_LEN                       7               /* CSRF token length */
#endif

#ifndef NPP_CIPHER_LIST_LEN
#define NPP_CIPHER_LIST_LEN                 1023            /* cipher list length for SSL (cipherList in npp.conf) */
#endif

#ifndef NPP_MAX_BOUNDARY_LEN
#define NPP_MAX_BOUNDARY_LEN                63              /* RFC 2046 mentions 70 characters, however no major HTTP client comes even close to 63 */
#endif

/* response caching */

#ifndef NPP_EXPIRES_STATICS
#define NPP_EXPIRES_STATICS                 90              /* days */
#endif

#ifndef NPP_EXPIRES_GENERATED
#define NPP_EXPIRES_GENERATED               30              /* days */
#endif

/* compression settings */

#ifndef NPP_COMPRESS_TRESHOLD
#define NPP_COMPRESS_TRESHOLD               500             /* bytes */
#endif

#ifndef NPP_COMPRESS_LEVEL
#define NPP_COMPRESS_LEVEL                  Z_BEST_SPEED
#endif

/* static resources */

#ifndef NPP_MAX_STATICS
#define NPP_MAX_STATICS                     1000            /* max static resources */
#endif

#ifndef NPP_MAX_SNIPPETS
#define NPP_MAX_SNIPPETS                    1000            /* max snippets */
#endif

/* menu */

#ifndef NPP_MAX_MENU_ITEMS
#define NPP_MAX_MENU_ITEMS                  100             /* max menu items */
#endif

#ifndef NPP_REQUIRED_AUTH_LEVEL
#define NPP_REQUIRED_AUTH_LEVEL             AUTH_LEVEL_NONE /* default resource authorization level */
#endif

/* asynchronous calls */

#ifndef NPP_ASYNC_MQ_MAXMSG
#define NPP_ASYNC_MQ_MAXMSG                 10              /* max messages in a message queue */
#endif

#ifndef NPP_ASYNC_MAX_REQUESTS
#define NPP_ASYNC_MAX_REQUESTS              NPP_MAX_SESSIONS /* max simultaneous async requests */
#endif

#ifndef NPP_ASYNC_REQ_MSG_SIZE
#define NPP_ASYNC_REQ_MSG_SIZE              8192            /* request message size */
#endif

#ifndef NPP_ASYNC_RES_MSG_SIZE
#define NPP_ASYNC_RES_MSG_SIZE              8192            /* response message size */
#endif

#ifndef NPP_ASYNC_HIGHEST_CALL_ID
#define NPP_ASYNC_HIGHEST_CALL_ID           1000000000      /* highest call id before reset */
#endif

#ifndef NPP_ASYNC_SHM_ACCESS_TRIES
#define NPP_ASYNC_SHM_ACCESS_TRIES          10              /* how many time try to obtain access to shared memory */
#endif



/* memory models' specs */

#ifdef NPP_MEM_TINY
#define NPP_IN_BUFSIZE                      4096            /* incoming request buffer length */
#define NPP_OUT_BUFSIZE                     65536           /* initial HTTP response buffer length */
#define NPP_MAX_CONNECTIONS                 10              /* max TCP connections */
#define NPP_MAX_SESSIONS                    5               /* max user sessions */
#elif defined NPP_MEM_MEDIUM
#define NPP_IN_BUFSIZE                      8192            /* incoming request buffer length */
#define NPP_OUT_BUFSIZE                     262144          /* initial HTTP response buffer length */
#define NPP_MAX_CONNECTIONS                 200             /* max TCP connections (2 per user session) */
#define NPP_MAX_SESSIONS                    100             /* max user sessions */
#elif defined NPP_MEM_LARGE
#define NPP_IN_BUFSIZE                      8192            /* incoming request buffer length */
#define NPP_OUT_BUFSIZE                     262144          /* initial HTTP response buffer length */
#define NPP_MAX_CONNECTIONS                 1000            /* max TCP connections */
#define NPP_MAX_SESSIONS                    500             /* max user sessions */
#elif defined NPP_MEM_XLARGE
#define NPP_IN_BUFSIZE                      8192            /* incoming request buffer length */
#define NPP_OUT_BUFSIZE                     262144          /* initial HTTP response buffer length */
#define NPP_MAX_CONNECTIONS                 5000            /* max TCP connections */
#define NPP_MAX_SESSIONS                    2500            /* max user sessions */
#elif defined NPP_MEM_XXLARGE
#define NPP_IN_BUFSIZE                      8192            /* incoming request buffer length */
#define NPP_OUT_BUFSIZE                     262144          /* initial HTTP response buffer length */
#define NPP_MAX_CONNECTIONS                 10000           /* max TCP connections */
#define NPP_MAX_SESSIONS                    5000            /* max user sessions */
#elif defined NPP_MEM_XXXLARGE
#define NPP_IN_BUFSIZE                      8192            /* incoming request buffer length */
#define NPP_OUT_BUFSIZE                     262144          /* initial HTTP response buffer length */
#define NPP_MAX_CONNECTIONS                 20000           /* max TCP connections */
#define NPP_MAX_SESSIONS                    10000           /* max user sessions */
#elif defined NPP_MEM_XXXXLARGE
#define NPP_IN_BUFSIZE                      8192            /* incoming request buffer length */
#define NPP_OUT_BUFSIZE                     262144          /* initial HTTP response buffer length */
#define NPP_MAX_CONNECTIONS                 50000           /* max TCP connections */
#define NPP_MAX_SESSIONS                    25000           /* max user sessions */
#elif defined NPP_MEM_XXXXXLARGE
#define NPP_IN_BUFSIZE                      8192            /* incoming request buffer length */
#define NPP_OUT_BUFSIZE                     262144          /* initial HTTP response buffer length */
#define NPP_MAX_CONNECTIONS                 100000          /* max TCP connections */
#define NPP_MAX_SESSIONS                    50000           /* max user sessions */
#elif defined NPP_MEM_XXXXXXLARGE
#define NPP_IN_BUFSIZE                      8192            /* incoming request buffer length */
#define NPP_OUT_BUFSIZE                     262144          /* initial HTTP response buffer length */
#define NPP_MAX_CONNECTIONS                 200000          /* max TCP connections */
#define NPP_MAX_SESSIONS                    100000          /* max user sessions */
#else   /* NPP_MEM_SMALL -- default */
#define NPP_IN_BUFSIZE                      8192            /* incoming request buffer length */
#define NPP_OUT_BUFSIZE                     131072          /* initial HTTP response buffer length */
#define NPP_MAX_CONNECTIONS                 20              /* max TCP connections */
#define NPP_MAX_SESSIONS                    10              /* max user sessions */
#endif

#ifndef NPP_TMP_BUFSIZE
#define NPP_TMP_BUFSIZE                     NPP_OUT_BUFSIZE /* temporary string buffer size */
#endif

#define NPP_TMP_STR_LEN                     NPP_TMP_BUFSIZE-1

#ifdef NPP_SVC
#undef NPP_MAX_CONNECTIONS
#define NPP_MAX_CONNECTIONS                 1               /* G_connections, G_sessions & G_app_session_data still as arrays to keep the same macros */
#undef NPP_MAX_SESSIONS
#define NPP_MAX_SESSIONS                    1
#endif  /* NPP_SVC */

#ifdef NPP_FD_MON_SELECT
#if NPP_MAX_CONNECTIONS > FD_SETSIZE-2
#undef NPP_MAX_CONNECTIONS
#define NPP_MAX_CONNECTIONS FD_SETSIZE-2
#endif
#endif  /* NPP_FD_MON_SELECT */

#define NPP_CLOSING_SESSION_CI              NPP_MAX_CONNECTIONS+1

#define NPP_NOT_CONNECTED                   -1

/* connection / request flags */

#define NPP_CONN_FLAG_SECURE                0x01
#define NPP_CONN_FLAG_PAYLOAD               0x02
#define NPP_CONN_FLAG_BOT                   0x04
#define NPP_CONN_FLAG_ACCEPT_DEFLATE        0x08
#define NPP_CONN_FLAG_KEEP_ALIVE            0x10
#define NPP_CONN_FLAG_UPGRADE_TO_HTTPS      0x20
#define NPP_CONN_FLAG_KEEP_CONTENT          0x40
#define NPP_CONN_FLAG_DONT_CACHE            0x80

#define NPP_CONN_IS_SECURE(flags)           ((flags & NPP_CONN_FLAG_SECURE) == NPP_CONN_FLAG_SECURE)
#define NPP_CONN_IS_PAYLOAD(flags)          ((flags & NPP_CONN_FLAG_PAYLOAD) == NPP_CONN_FLAG_PAYLOAD)
#define NPP_CONN_IS_BOT(flags)              ((flags & NPP_CONN_FLAG_BOT) == NPP_CONN_FLAG_BOT)
#define NPP_CONN_IS_ACCEPT_DEFLATE(flags)   ((flags & NPP_CONN_FLAG_ACCEPT_DEFLATE) == NPP_CONN_FLAG_ACCEPT_DEFLATE)
#define NPP_CONN_IS_KEEP_ALIVE(flags)       ((flags & NPP_CONN_FLAG_KEEP_ALIVE) == NPP_CONN_FLAG_KEEP_ALIVE)
#define NPP_CONN_IS_UPGRADE_TO_HTTPS(flags) ((flags & NPP_CONN_FLAG_UPGRADE_TO_HTTPS) == NPP_CONN_FLAG_UPGRADE_TO_HTTPS)
#define NPP_CONN_IS_KEEP_CONTENT(flags)     ((flags & NPP_CONN_FLAG_KEEP_CONTENT) == NPP_CONN_FLAG_KEEP_CONTENT)
#define NPP_CONN_IS_DONT_CACHE(flags)       ((flags & NPP_CONN_FLAG_DONT_CACHE) == NPP_CONN_FLAG_DONT_CACHE)

/* connection / request state */

#define CONN_STATE_DISCONNECTED             '0'
#define CONN_STATE_CONNECTED                '1'
#define CONN_STATE_READY_FOR_PARSE          'p'
#define CONN_STATE_READY_FOR_PROCESS        'P'
#define CONN_STATE_READING_DATA             'd'
#define CONN_STATE_WAITING_FOR_ASYNC        'A'
#define CONN_STATE_READY_TO_SEND_RESPONSE   'R'
#define CONN_STATE_SENDING_CONTENT          'S'

/* HTTP/2 */

#define CONN_STATE_READY_TO_SEND_SETTINGS   '2'
#define CONN_STATE_SETTINGS_SENT            's'
#define CONN_STATE_READY_FOR_CLIENT_PREFACE 'C'

/* ASYNC flags */

#define NPP_ASYNC_FLAG_WANT_RESPONSE         0x01
#define NPP_ASYNC_FLAG_PAYLOAD_IN_SHM        0x02

#define NPP_ASYNC_IS_WANT_RESPONSE(flags)    ((flags & NPP_ASYNC_FLAG_WANT_RESPONSE) == NPP_ASYNC_FLAG_WANT_RESPONSE)
#define NPP_ASYNC_IS_PAYLOAD_IN_SHM(flags)   ((flags & NPP_ASYNC_FLAG_PAYLOAD_IN_SHM) == NPP_ASYNC_FLAG_PAYLOAD_IN_SHM)


#define NPP_VALID_RELOAD_CONF_REQUEST       (REQ("npp_reload_conf") && REQ_POST && 0==strcmp(G_connections[ci].ip, "127.0.0.1"))


#define SHOULD_BE_COMPRESSED(len, type)     (len > NPP_COMPRESS_TRESHOLD && (type==NPP_CONTENT_TYPE_HTML || type==NPP_CONTENT_TYPE_CSS || type==NPP_CONTENT_TYPE_JS || type==NPP_CONTENT_TYPE_SVG || type==NPP_CONTENT_TYPE_JSON || type==NPP_CONTENT_TYPE_MD || type==NPP_CONTENT_TYPE_PDF || type==NPP_CONTENT_TYPE_XML || type==NPP_CONTENT_TYPE_EXE || type==NPP_CONTENT_TYPE_BMP || type==NPP_CONTENT_TYPE_TEXT))


/* errors */

/* 0 always means OK */
#define ERR_INVALID_REQUEST             1
#define ERR_UNAUTHORIZED                2
#define ERR_FORBIDDEN                   3
#define ERR_NOT_FOUND                   4
#define ERR_METHOD                      5
#define ERR_INT_SERVER_ERROR            6
#define ERR_SERVER_TOOBUSY              7
#define ERR_FILE_TOO_BIG                8
#define ERR_REDIRECTION                 9
#define ERR_ASYNC_NO_SUCH_SERVICE       10
#define ERR_ASYNC_TIMEOUT               11
#define ERR_REMOTE_CALL                 12
#define ERR_REMOTE_CALL_STATUS          13
#define ERR_REMOTE_CALL_DATA            14
#define ERR_CSRFT                       15
#define ERR_RECORD_NOT_FOUND            16
/* ------------------------------------- */
#define ERR_MAX_ENGINE_ERROR            99
/* ------------------------------------- */


#define MSG_CAT_OK                      "OK"
#define MSG_CAT_ERROR                   "err"
#define MSG_CAT_WARNING                 "war"
#define MSG_CAT_MESSAGE                 "msg"


/* statics */

#define NPP_NOT_STATIC                  -1

#define NPP_STATIC_PATH_LEN             1023

#define STATIC_SOURCE_INTERNAL          '0'
#define STATIC_SOURCE_RES               '1'
#define STATIC_SOURCE_RESMIN            '2'
#define STATIC_SOURCE_SNIPPETS          '3'


/* asynchronous calls */

#define NPP_ASYNC_STATE_FREE            '0'
#define NPP_ASYNC_STATE_SENT            '1'

#define NPP_ASYNC_REQ_QUEUE             "/npp_req"              /* request queue name */
#define NPP_ASYNC_RES_QUEUE             "/npp_res"              /* response queue name */
#define NPP_ASYNC_DEF_TIMEOUT           60                      /* in seconds */
#define NPP_ASYNC_MAX_TIMEOUT           1800                    /* in seconds ==> 30 minutes */

#define NPP_SVC_NAME_LEN                63                      /* async service name length */


/* these are flags */

#define ASYNC_CHUNK_FIRST               0x040000
#define ASYNC_CHUNK_LAST                0x080000
#define ASYNC_CHUNK_IS_FIRST(n)         ((n & ASYNC_CHUNK_FIRST) == ASYNC_CHUNK_FIRST)
#define ASYNC_CHUNK_IS_LAST(n)          ((n & ASYNC_CHUNK_LAST) == ASYNC_CHUNK_LAST)

#ifdef NPP_SVC
#define SVC(svc)                        (0==strcmp(G_service, svc))
#define ASYNC_ERR_CODE                  G_error_code
#else
#define SVC(svc)                        (0==strcmp(G_connections[ci].service, svc))
#define ASYNC_ERR_CODE                  G_connections[ci].async_err_code
#endif  /* NPP_SVC */

#define CALL_ASYNC(svc)                 npp_eng_call_async(ci, svc, NULL, TRUE, G_ASYNCDefTimeout, 0)
#define CALL_ASYNC_TM(svc, tmout)       npp_eng_call_async(ci, svc, NULL, TRUE, tmout, 0)
#define CALL_ASYNC_NR(svc)              npp_eng_call_async(ci, svc, NULL, FALSE, 0, 0)

#define CALL_ASYNC_BIN(svc, data, size) npp_eng_call_async(ci, svc, data, TRUE, G_ASYNCDefTimeout, size)


/* resource / content types */

/* incoming */

#define NPP_CONTENT_TYPE_URLENCODED     'U'
#define NPP_CONTENT_TYPE_JSON           'J'
#define NPP_CONTENT_TYPE_MULTIPART      'M'
#define NPP_CONTENT_TYPE_OCTET_STREAM   'O'

/* outgoing */

#define NPP_CONTENT_TYPE_UNSET          '-'
#define NPP_CONTENT_TYPE_USER           '+'
#define NPP_CONTENT_TYPE_TEXT           't'
#define NPP_CONTENT_TYPE_HTML           'h'
#define NPP_CONTENT_TYPE_CSS            'c'
#define NPP_CONTENT_TYPE_JS             's'
#define NPP_CONTENT_TYPE_GIF            'g'
#define NPP_CONTENT_TYPE_JPG            'j'
#define NPP_CONTENT_TYPE_ICO            'i'
#define NPP_CONTENT_TYPE_PNG            'p'
#define NPP_CONTENT_TYPE_WOFF2          'w'
#define NPP_CONTENT_TYPE_SVG            'v'
//#define NPP_CONTENT_TYPE_JSON           'o'
#define NPP_CONTENT_TYPE_MD             'm'
#define NPP_CONTENT_TYPE_PDF            'a'
#define NPP_CONTENT_TYPE_XML            'x'
#define NPP_CONTENT_TYPE_AMPEG          '3'     /* mp3 */
#define NPP_CONTENT_TYPE_EXE            'e'
#define NPP_CONTENT_TYPE_ZIP            'z'
#define NPP_CONTENT_TYPE_GZIP           'k'
#define NPP_CONTENT_TYPE_BMP            'b'


/* request macros */

#define REQ_METHOD                      G_connections[ci].method
#define REQ_HEAD                        (0==strcmp(G_connections[ci].method, "HEAD"))
#define REQ_GET                         (0==strcmp(G_connections[ci].method, "GET"))
#define REQ_POST                        (0==strcmp(G_connections[ci].method, "POST"))
#define REQ_PUT                         (0==strcmp(G_connections[ci].method, "PUT"))
#define REQ_DELETE                      (0==strcmp(G_connections[ci].method, "DELETE"))
#define REQ_OPTIONS                     (0==strcmp(G_connections[ci].method, "OPTIONS"))

#define REQ_URI                         G_connections[ci].uri
#define REQ_CONTENT_TYPE                G_connections[ci].in_ctypestr
#define REQ_BOT                         NPP_CONN_IS_BOT(G_connections[ci].flags)
#define REQ_LANG                        G_connections[ci].lang

#define REQ_DSK                         (G_connections[ci].ua_type==NPP_UA_TYPE_DSK)
#define REQ_TAB                         (G_connections[ci].ua_type==NPP_UA_TYPE_TAB)
#define REQ_MOB                         (G_connections[ci].ua_type==NPP_UA_TYPE_MOB)
#define URI(uri)                        npp_eng_is_uri(ci, uri)
#define REQ(res)                        (0==strcmp(G_connections[ci].resource, res))
#define REQ0(res)                       (0==strcmp(G_connections[ci].resource, res))
#define REQ1(res)                       (0==strcmp(G_connections[ci].req1, res))
#define REQ2(res)                       (0==strcmp(G_connections[ci].req2, res))
#define REQ3(res)                       (0==strcmp(G_connections[ci].req3, res))
#define REQ4(res)                       (0==strcmp(G_connections[ci].req4, res))
#define REQ5(res)                       (0==strcmp(G_connections[ci].req5, res))
#define REQ_ID                          G_connections[ci].id
#define SESSION                         G_sessions[G_connections[ci].si]
#define SESSION_DATA                    G_app_session_data[G_connections[ci].si]
#define IS_SESSION                      (G_connections[ci].si>0)
#define HOST(str)                       (0==strcmp(G_connections[ci].host_normalized, npp_upper(str)))
#define REQ_GET_HEADER(header)          npp_eng_get_header(ci, header)

#define REQ_DATA                        G_connections[ci].in_data

#define CALL_HTTP_PASS_HEADER(header)   npp_eng_call_http_pass_header(ci, header)


/* response macros */

/* generate output as fast as possible */

#ifdef NPP_SVC

    #ifdef NPP_OUT_FAST
        #define OUTSS(str)                  (G_svc_p_content = stpcpy(G_svc_p_content, str))
        #define OUT_BIN(data, len)          (len=(len>G_async_res_data_size?G_async_res_data_size:len), memcpy(G_svc_p_content, data, len), G_svc_p_content += len)
    #elif defined (NPP_OUT_CHECK)
        #define OUTSS(str)                  npp_svc_out_check(str)
        #define OUT_BIN(data, len)          (len=(len>G_async_res_data_size?G_async_res_data_size:len), memcpy(G_svc_p_content, data, len), G_svc_p_content += len)
    #else   /* NPP_OUT_CHECK_REALLOC */
        #define OUTSS(str)                  npp_svc_out_check_realloc(str)
        #define OUT_BIN(data, len)          npp_svc_out_check_realloc_bin(data, len)
    #endif

#else   /* NPP_APP */

    #define HOUT(str)                   (G_connections[ci].p_header = stpcpy(G_connections[ci].p_header, str))
    #define HOUT_BIN(data, len)         (memcpy(G_connections[ci].p_header, data, len), G_connections[ci].p_header += len)

    #ifdef NPP_OUT_FAST
        #define OUTSS(str)                  (G_connections[ci].p_content = stpcpy(G_connections[ci].p_content, str))
        #define OUT_BIN(data, len)          (len=(len>NPP_OUT_BUFSIZE-NPP_OUT_HEADER_BUFSIZE?NPP_OUT_BUFSIZE-NPP_OUT_HEADER_BUFSIZE:len), memcpy(G_connections[ci].p_content, data, len), G_connections[ci].p_content += len)
    #else
        #ifdef NPP_OUT_CHECK
            #define OUTSS(str)                  npp_eng_out_check(ci, str)
            #define OUT_BIN(data, len)          (len=(len>NPP_OUT_BUFSIZE-NPP_OUT_HEADER_BUFSIZE?NPP_OUT_BUFSIZE-NPP_OUT_HEADER_BUFSIZE:len), memcpy(G_connections[ci].p_content, data, len), G_connections[ci].p_content += len)
        #else   /* NPP_OUT_CHECK_REALLOC */
            #define OUTSS(str)                  npp_eng_out_check_realloc(ci, str)
            #define OUT_BIN(data, len)          npp_eng_out_check_realloc_bin(ci, data, len)
        #endif
    #endif  /* NPP_OUT_FAST */

#endif  /* NPP_SVC */

#ifdef NPP_CPP_STRINGS
    #define OUT(str, ...)                   NPP_CPP_STRINGS_OUT(ci, str, ##__VA_ARGS__)
#else
#ifdef _MSC_VER /* Microsoft compiler */
    #define OUT(...)                        (sprintf(G_tmp, EXPAND_VA(__VA_ARGS__)), OUTSS(G_tmp))
#else   /* GCC */
    #define OUTM(str, ...)                  (sprintf(G_tmp, str, __VA_ARGS__), OUTSS(G_tmp))   /* OUT with multiple args */
    #define CHOOSE_OUT(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, NAME, ...) NAME          /* single or multiple? */
    #define OUT(...)                        CHOOSE_OUT(__VA_ARGS__, OUTM, OUTM, OUTM, OUTM, OUTM, OUTM, OUTM, OUTM, OUTM, OUTM, OUTM, OUTM, OUTM, OUTSS)(__VA_ARGS__)
#endif  /* _MSC_VER */
#endif  /* NPP_CPP_STRINGS */


#define RES_STATUS(val)                 npp_lib_set_res_status(ci, val)
#define RES_CONTENT_TYPE(str)           npp_lib_set_res_content_type(ci, str)
#define RES_LOCATION(str, ...)          npp_lib_set_res_location(ci, str, ##__VA_ARGS__)
#define RES_REDIRECT(str, ...)          RES_LOCATION(str, ##__VA_ARGS__)
#define RES_KEEP_CONTENT                (G_connections[ci].flags |= NPP_CONN_FLAG_KEEP_CONTENT)
#define RES_DONT_CACHE                  (G_connections[ci].flags |= NPP_CONN_FLAG_DONT_CACHE)
#define RES_CONTENT_DISPOSITION(str, ...) npp_lib_set_res_content_disposition(ci, str, ##__VA_ARGS__)

#define RES_CONTENT_TYPE_TEXT           (G_connections[ci].out_ctype = NPP_CONTENT_TYPE_TEXT)
#define RES_CONTENT_TYPE_HTML           (G_connections[ci].out_ctype = NPP_CONTENT_TYPE_HTML)
#define RES_CONTENT_TYPE_JSON           (G_connections[ci].out_ctype = NPP_CONTENT_TYPE_JSON)

#define REDIRECT_TO_LANDING             sprintf(G_connections[ci].location, "%s://%s", NPP_PROTOCOL, G_connections[ci].host)

#define APPEND_CSS(name, first)         npp_append_css(ci, name, first)
#define APPEND_SCRIPT(name, first)      npp_append_script(ci, name, first)


#define NPP_UA_IE                       (0==strncmp(G_connections[ci].uagent, "Mozilla/5.0 (Windows NT ", 24) && strstr(G_connections[ci].uagent, "; WOW64; Trident/7.0; rv:11.0) like Gecko"))


/* datetime strings (YYYY-MM-DD hh:mm:ss) */

#define DT_NULL                         "0000-00-00 00:00:00"               /* null datetime string */

#define DT_NOW_GMT                      G_dt_string_gmt                     /* current datetime string (GMT) */
#define DT_NOW_LOCAL                    time_epoch2db(npp_ua_time(ci))      /* current datetime string (user agent timezone) */

#define DT_TODAY_GMT                    npp_today_gmt()                     /* current date only (GMT) (YYYY-MM-DD) */
#define DT_TODAY_LOCAL                  npp_today_ua(ci)                    /* current date only (user agent timezone) */

#define DT_IS_NULL(dt)                  (0==strcmp(dt, DT_NULL))            /* is datetime string null? */

#define DT_NOW                          DT_NOW_GMT                          /* convenience alias */

/* RFC 2822 */

#define DT_HEADER                       G_header_date


/* CSRFT */

#define CSRFT_REFRESH                   npp_random(SESSION.csrft, NPP_CSRFT_LEN)
#define CSRFT_OUT_INPUT                 OUT("<input type=\"hidden\" name=\"csrft\" value=\"%s\">", SESSION.csrft)
#define OUT_CSRFT                       CSRFT_OUT_INPUT
#define CSRFT_OK                        npp_csrft_ok(ci)




/* --------------------------------------------------------------------------
   structures
-------------------------------------------------------------------------- */

/* HTTP status */

typedef struct {
    int     status;
    char    description[128];
} http_status_t;


/* date */

typedef struct {
    short   year;
    char    month;
    char    day;
} date_t;


/* hosts */

typedef struct {
    char host[NPP_MAX_HOST_LEN+1];
    char res[256];
    char resmin[256];
    char snippets[256];
    char required_auth_level;
    int  index_present;
} npp_host_t;


/* engine session data */

typedef struct {
    /* id */
#ifdef NPP_MULTI_HOST
    int     host_id;
#endif
    char    sessid[NPP_SESSID_LEN+1];
    /* connection data */
    char    ip[INET6_ADDRSTRLEN];
    char    uagent[NPP_MAX_VALUE_LEN+1];
    char    referer[NPP_MAX_VALUE_LEN+1];
    char    lang[NPP_LANG_LEN+1];
    char    formats;     /* date & numbers format */
    char    auth_level;
    /* users table record */
    int     user_id;
    char    login[NPP_LOGIN_LEN+1];
    char    email[NPP_EMAIL_LEN+1];
    char    name[NPP_UNAME_LEN+1];
    char    phone[NPP_PHONE_LEN+1];
    char    about[NPP_ABOUT_LEN+1];
    int     group_id;
    /* time zone info */
    short   tz_offset;
    bool    tz_set;
    /* CSRF token */
    char    csrft[NPP_CSRFT_LEN+1];
    /* internal */
    time_t  last_activity;
} eng_session_data_t;


/* sessions' index */

typedef struct {
#ifdef NPP_MULTI_HOST
    int     host_id;
#endif
    char    sessid[NPP_SESSID_LEN+1];
    int     si;
} sessions_idx_t;


/* counters */

typedef struct {
    unsigned req;            /* all parsed requests */
    unsigned req_dsk;        /* all requests with desktop UA */
    unsigned req_tab;        /* all requests with tablet UA */
    unsigned req_mob;        /* all requests with mobile UA */
    unsigned req_bot;        /* all requests with HTTP header indicating well-known search-engine bots */
    unsigned visits;         /* all visits to domain (Host=NPP_APP_DOMAIN) landing page (no action/resource), excl. bots that got 200 */
    unsigned visits_dsk;     /* like visits -- desktop only */
    unsigned visits_tab;     /* like visits -- tablets only */
    unsigned visits_mob;     /* like visits -- mobile only */
    unsigned blocked;        /* attempts from blocked IP */
    double   elapsed;        /* sum of elapsed time of all requests for calculating average */
    double   average;        /* average request elapsed */
} npp_counters_t;


/* asynchorous processing */

/* request */
/* we try hard to stay below default 8 KiB MQ limit */

typedef struct {
    unsigned call_id;
    int      ai;
    int      ci;
    char     service[NPP_SVC_NAME_LEN+1];
    /* pass some request details over */
    char     ip[INET6_ADDRSTRLEN];
    char     method[NPP_METHOD_LEN+1];
    char     uri[NPP_MAX_URI_LEN+1];
    char     resource[NPP_MAX_RESOURCE_LEN+1];
#if NPP_RESOURCE_LEVELS > 1
    char     req1[NPP_MAX_RESOURCE_LEN+1];
#if NPP_RESOURCE_LEVELS > 2
    char     req2[NPP_MAX_RESOURCE_LEN+1];
#if NPP_RESOURCE_LEVELS > 3
    char     req3[NPP_MAX_RESOURCE_LEN+1];
#if NPP_RESOURCE_LEVELS > 4
    char     req4[NPP_MAX_RESOURCE_LEN+1];
#if NPP_RESOURCE_LEVELS > 5
    char     req5[NPP_MAX_RESOURCE_LEN+1];
#endif  /* NPP_RESOURCE_LEVELS > 5 */
#endif  /* NPP_RESOURCE_LEVELS > 4 */
#endif  /* NPP_RESOURCE_LEVELS > 3 */
#endif  /* NPP_RESOURCE_LEVELS > 2 */
#endif  /* NPP_RESOURCE_LEVELS > 1 */
    char     id[NPP_MAX_RESOURCE_LEN+1];
    char     uagent[NPP_MAX_VALUE_LEN+1];
    char     ua_type;
    char     referer[NPP_MAX_VALUE_LEN+1];
    unsigned clen;
    char     in_cookie[NPP_MAX_VALUE_LEN+1];
    char     host[NPP_MAX_HOST_LEN+1];
#ifdef NPP_MULTI_HOST
    char     host_normalized[NPP_MAX_HOST_LEN+1];
    int      host_id;
#endif
    char     app_name[NPP_APP_NAME_LEN+1];
    char     lang[NPP_LANG_LEN+1];
    char     in_ctype;
    char     boundary[NPP_MAX_BOUNDARY_LEN+1];
    int      status;
    char     cust_headers[NPP_CUST_HDR_LEN+1];
    int      cust_headers_len;
    char     out_ctype;
    char     ctypestr[NPP_CONTENT_TYPE_LEN+1];
    char     cdisp[NPP_CONTENT_DISP_LEN+1];
    char     cookie_out_a[NPP_SESSID_LEN+1];
    char     cookie_out_a_exp[32];
    char     cookie_out_l[NPP_SESSID_LEN+1];
    char     cookie_out_l_exp[32];
    char     location[NPP_MAX_URI_LEN+1];
    int      si;
    char     flags;
    char     async_flags;
    eng_session_data_t eng_session_data;
#ifdef NPP_ASYNC_INCLUDE_SESSION_DATA
    app_session_data_t app_session_data;
#endif
    npp_counters_t cnts_today;
    npp_counters_t cnts_yesterday;
    npp_counters_t cnts_day_before;
    int      days_up;
    int      connections_cnt;
    int      connections_hwm;
    int      sessions_cnt;
    int      sessions_hwm;
    char     last_modified[32];
    int      blacklist_cnt;
} async_req_hdr_t;

typedef struct {
    async_req_hdr_t hdr;
    char            data[NPP_ASYNC_REQ_MSG_SIZE-sizeof(async_req_hdr_t)];
} async_req_t;


/* async requests stored on the npp_app's side */

typedef struct {
    int      ci;
    char     state;
    time_t   sent;
    int      timeout;
} areq_t;


/* response -- the first chunk */

typedef struct {
    int      err_code;
    int      status;
    char     cust_headers[NPP_CUST_HDR_LEN+1];
    int      cust_headers_len;
    char     out_ctype;
    char     ctypestr[NPP_CONTENT_TYPE_LEN+1];
    char     cdisp[NPP_CONTENT_DISP_LEN+1];
    char     cookie_out_a[NPP_SESSID_LEN+1];
    char     cookie_out_a_exp[32];
    char     cookie_out_l[NPP_SESSID_LEN+1];
    char     cookie_out_l_exp[32];
    char     location[NPP_MAX_URI_LEN+1];
    int      call_http_status;
    unsigned call_http_req_cnt;
    double   call_http_elapsed;
    eng_session_data_t eng_session_data;
#ifdef NPP_ASYNC_INCLUDE_SESSION_DATA
    app_session_data_t app_session_data;
#endif
    int      invalidate_uid;
    int      invalidate_ci;
    char     flags;
} async_res_hdr_t;

typedef struct {
    int             ai;
    int             chunk;
    int             ci;
    int             len;
    async_res_hdr_t hdr;
    char            data[NPP_ASYNC_RES_MSG_SIZE-sizeof(async_res_hdr_t)-sizeof(int)*4];
} async_res_t;


/* response -- the second type for the chunks > 1 */

typedef struct {
    int      ai;
    int      chunk;
    int      ci;
    int      len;
    char     data[NPP_ASYNC_RES_MSG_SIZE-sizeof(int)*4];
} async_res_data_t;


/* HTTP/2 frames */

typedef struct {
    unsigned char length[3];
    unsigned char type;
    unsigned char flags;
    int32_t       stream_id;
} http2_frame_hdr_t;


typedef struct {
    char    pad_len;
    int32_t stream_dep;
    uint8_t weight;
} http2_HEADERS_pld_t;

#define HTTP2_HEADERS_PLD_LEN 6


typedef struct {
    uint16_t id;
    uint32_t value;
} http2_SETTINGS_pld_t;

#define HTTP2_SETTINGS_PAIR_LEN 6


typedef struct {
    char    pad_len;
    int32_t stream_id;
} http2_PUSH_PROMISE_pld_t;


typedef struct {
    int32_t last_stream_id;
    int32_t error_code;
} http2_GOAWAY_pld_t;


typedef struct {
    int32_t wnd_size_inc;
} http2_WINDOW_SIZE_pld_t;


/* connection / request */

#ifdef NPP_SVC
typedef struct {                            /* request details for npp_svc */
    char     ip[INET6_ADDRSTRLEN];
    char     method[NPP_METHOD_LEN+1];
    char     uri[NPP_MAX_URI_LEN+1];
    char     resource[NPP_MAX_RESOURCE_LEN+1];
#if NPP_RESOURCE_LEVELS > 1
    char     req1[NPP_MAX_RESOURCE_LEN+1];
#if NPP_RESOURCE_LEVELS > 2
    char     req2[NPP_MAX_RESOURCE_LEN+1];
#if NPP_RESOURCE_LEVELS > 3
    char     req3[NPP_MAX_RESOURCE_LEN+1];
#if NPP_RESOURCE_LEVELS > 4
    char     req4[NPP_MAX_RESOURCE_LEN+1];
#if NPP_RESOURCE_LEVELS > 5
    char     req5[NPP_MAX_RESOURCE_LEN+1];
#endif  /* NPP_RESOURCE_LEVELS > 5 */
#endif  /* NPP_RESOURCE_LEVELS > 4 */
#endif  /* NPP_RESOURCE_LEVELS > 3 */
#endif  /* NPP_RESOURCE_LEVELS > 2 */
#endif  /* NPP_RESOURCE_LEVELS > 1 */
    char     id[NPP_MAX_RESOURCE_LEN+1];
    char     uagent[NPP_MAX_VALUE_LEN+1];
    char     ua_type;
    char     referer[NPP_MAX_VALUE_LEN+1];
    unsigned clen;
    char     in_cookie[NPP_MAX_VALUE_LEN+1];
    unsigned in_data_allocated;
    char     host[NPP_MAX_HOST_LEN+1];
#ifdef NPP_MULTI_HOST
    char     host_normalized[NPP_MAX_HOST_LEN+1];
    int      host_id;
#endif
    char     app_name[NPP_APP_NAME_LEN+1];
    char     lang[NPP_LANG_LEN+1];
    char     formats;
    char     in_ctype;
    char     boundary[NPP_MAX_BOUNDARY_LEN+1];
    char     *in_data;
    int      status;
    char     cust_headers[NPP_CUST_HDR_LEN+1];
    int      cust_headers_len;
    char     out_ctype;
    char     ctypestr[NPP_CONTENT_TYPE_LEN+1];
    char     cdisp[NPP_CONTENT_DISP_LEN+1];
    char     cookie_out_a[NPP_SESSID_LEN+1];
    char     cookie_out_a_exp[32];
    char     cookie_out_l[NPP_SESSID_LEN+1];
    char     cookie_out_l_exp[32];
    char     location[NPP_MAX_URI_LEN+1];
    int      si;
    char     flags;
} npp_connection_t;
#else   /* NPP_APP */
typedef struct {
    /* what comes in */
#ifdef _WIN32   /* Windows */
    SOCKET   fd;                                    /* file descriptor */
#else
    int      fd;                                    /* file descriptor */
#endif  /* _WIN32 */
    char     ip[INET6_ADDRSTRLEN];                  /* client IP */
    char     in[NPP_IN_BUFSIZE];                    /* the whole incoming request */
    char     method[NPP_METHOD_LEN+1];              /* HTTP method */
    unsigned was_read;                              /* request bytes read so far */
    /* parsed HTTP request starts here */
    char     uri[NPP_MAX_URI_LEN+1];                /* requested URI string */
    char     resource[NPP_MAX_RESOURCE_LEN+1];      /* from URI (REQ0) */
#if NPP_RESOURCE_LEVELS > 1
    char     req1[NPP_MAX_RESOURCE_LEN+1];          /* from URI -- level 1 */
#if NPP_RESOURCE_LEVELS > 2
    char     req2[NPP_MAX_RESOURCE_LEN+1];          /* from URI -- level 2 */
#if NPP_RESOURCE_LEVELS > 3
    char     req3[NPP_MAX_RESOURCE_LEN+1];          /* from URI -- level 3 */
#if NPP_RESOURCE_LEVELS > 4
    char     req4[NPP_MAX_RESOURCE_LEN+1];          /* from URI -- level 4 */
#if NPP_RESOURCE_LEVELS > 5
    char     req5[NPP_MAX_RESOURCE_LEN+1];          /* from URI -- level 5 */
#endif  /* NPP_RESOURCE_LEVELS > 5 */
#endif  /* NPP_RESOURCE_LEVELS > 4 */
#endif  /* NPP_RESOURCE_LEVELS > 3 */
#endif  /* NPP_RESOURCE_LEVELS > 2 */
#endif  /* NPP_RESOURCE_LEVELS > 1 */
    char     id[NPP_MAX_RESOURCE_LEN+1];            /* from URI -- last part */
    char     http_ver[4];                           /* HTTP request version */
#ifdef NPP_HTTP2
    bool     http2_upgrade_in_progress;
    /* settings */
    uint32_t http2_max_streams;
    uint32_t http2_wnd_size;
    uint32_t http2_max_frame_size;
    int32_t  http2_last_stream_id;
    /* outgoing frame */
//    char     http2_frame_hdr[sizeof(http2_frame_hdr_t)];
//    char     http2_settings_data[16];
    char     *http2_frame_start;
//    int32_t  http2_frame_len;
    unsigned http2_bytes_to_send;
#endif  /* NPP_HTTP2 */
    char     uagent[NPP_MAX_VALUE_LEN+1];           /* user agent string */
    char     ua_type;                               /* user agent type */
    char     referer[NPP_MAX_VALUE_LEN+1];
    unsigned clen;                                  /* incoming & outgoing content length */
    char     in_cookie[NPP_MAX_VALUE_LEN+1];        /* request cookie */
    char     cookie_in_a[NPP_SESSID_LEN+1];         /* anonymous sessid from cookie */
    char     cookie_in_l[NPP_SESSID_LEN+1];         /* logged in sessid from cookie */
    char     host[NPP_MAX_HOST_LEN+1];              /* request host */
#ifdef NPP_MULTI_HOST
    char     host_normalized[NPP_MAX_HOST_LEN+1];   /* SLD (second-level domain uppercase) */
    int      host_id;
#endif
    char     app_name[NPP_APP_NAME_LEN+1];
    char     lang[NPP_LANG_LEN+1];                  /* request language */
    char     formats;                               /* date & numbers format */
    time_t   if_mod_since;                          /* request If-Mod-Since */
    char     in_ctypestr[NPP_MAX_VALUE_LEN+1];      /* content type as an original string */
    char     in_ctype;                              /* content type */
    char     boundary[NPP_MAX_BOUNDARY_LEN+1];      /* for POST multipart/form-data type */
    char     authorization[NPP_MAX_VALUE_LEN+1];    /* Authorization header */
    /* POST data */
    char     *in_data;                              /* POST data */
    /* what goes out */
    unsigned out_hlen;                              /* outgoing header length */
    unsigned out_len;                               /* outgoing length (all) */
    char     *out_start;
#ifdef NPP_OUT_CHECK_REALLOC
    char     *out_data_alloc;                       /* allocated space for rendered content */
#else
    char     out_data_alloc[NPP_OUT_BUFSIZE];
#endif
    unsigned out_data_allocated;                    /* number of allocated bytes */
    char     *out_data;                             /* pointer to the data to send */
    int      status;                                /* HTTP status */
    char     cust_headers[NPP_CUST_HDR_LEN+1];
    int      cust_headers_len;
    unsigned data_sent;                             /* how many content bytes have been sent */
    char     out_ctype;                             /* content type */
    char     ctypestr[NPP_CONTENT_TYPE_LEN+1];      /* user (custom) content type */
    char     cdisp[NPP_CONTENT_DISP_LEN+1];         /* content disposition */
    time_t   modified;
    char     cookie_out_a[NPP_SESSID_LEN+1];
    char     cookie_out_a_exp[32];                  /* cookie expires */
    char     cookie_out_l[NPP_SESSID_LEN+1];
    char     cookie_out_l_exp[32];                  /* cookie expires */
    char     location[NPP_MAX_URI_LEN+1];           /* redirection */
    /* internal stuff */
    unsigned req;                                   /* request count */
    struct timespec proc_start;                     /* start processing time */
    double   elapsed;                               /* processing time in ms */
    char     state;                                 /* connection state (STATE_XXX) */
    char     *p_header;                             /* current header pointer */
    char     *p_content;                            /* current content pointer */
#ifdef NPP_HTTPS
    SSL      *ssl;
#endif
    int      ssl_err;
    int      si;                                    /* session index */
    int      static_res;                            /* static resource index in M_stat */
    time_t   last_activity;
#ifdef NPP_FD_MON_POLL
    int      pi;                                    /* M_pollfds array index */
#endif
#ifdef NPP_FD_MON_EPOLL
//    bool     epoll_out_ready;
#endif
#ifdef NPP_ASYNC
    char     service[NPP_SVC_NAME_LEN+1];
    int      async_err_code;
#endif
    char     flags;
    char     required_auth_level;                   /* required authorization level */
    bool     expect100;
} npp_connection_t;
#endif  /* NPP_SVC */


/* static resources */

typedef struct {
    char     host[NPP_MAX_HOST_LEN+1];
    int      host_id;
    char     name[NPP_STATIC_PATH_LEN+1];
    char     type;
    char     *data;
    unsigned len;
    char     *data_deflated;
    unsigned len_deflated;
    time_t   modified;
    char     source;
} static_res_t;


/* authorization levels */

typedef struct {
    char host[NPP_MAX_HOST_LEN+1];
    int  host_id;
    char path[NPP_STATIC_PATH_LEN+1];
    char level;
} auth_level_t;


/* snippets */

typedef struct {
    char     host[NPP_MAX_HOST_LEN+1];
    int      host_id;
    char     name[NPP_STATIC_PATH_LEN+1];
    char     type;
    char     *data;
    unsigned len;
    time_t   modified;
} snippet_t;


/* menu */

typedef struct {
    int  id;
    int  parent;
    char resource[256];
    char title[256];
    char snippet[256];
} menu_item_t;


/* admin info */

typedef struct {
    char sql[1024];
    char th[256];
    char type[32];
} admin_info_t;


/* counters formatted */

typedef struct {
    char req[64];
    char req_dsk[64];
    char req_tab[64];
    char req_mob[64];
    char req_bot[64];
    char visits[64];
    char visits_dsk[64];
    char visits_tab[64];
    char visits_mob[64];
    char blocked[64];
    char average[64];
} counters_fmt_t;



#include <npp_lib.h>

#ifdef NPP_USERS
#include <npp_usr.h>
#endif



/* --------------------------------------------------------------------------
   globals
-------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* read from the config file */

extern int          G_test;
extern int          G_logLevel;
extern int          G_logToStdout;
extern int          G_logCombined;
extern int          G_httpPort;
extern int          G_httpsPort;
extern char         G_cipherList[NPP_CIPHER_LIST_LEN+1];
extern char         G_certFile[256];
extern char         G_certChainFile[256];
extern char         G_keyFile[256];
extern char         G_dbHost[128];
extern int          G_dbPort;
extern char         G_dbName[128];
extern char         G_dbUser[128];
extern char         G_dbPassword[128];
extern int          G_usersRequireActivation;
extern char         G_IPBlackList[256];
extern char         G_IPWhiteList[256];
extern int          G_ASYNCId;
extern int          G_ASYNCSvcProcesses;
extern int          G_ASYNCDefTimeout;
extern int          G_callHTTPTimeout;

/* end of config params */

extern FILE         *G_log_fd;
extern bool         G_endianness;
extern int          G_pid;                                      /* pid */
extern char         G_appdir[256];                              /* application root dir */
extern int          G_days_up;                                  /* web server's days up */
extern npp_connection_t G_connections[NPP_MAX_CONNECTIONS+2];   /* HTTP connections & requests -- the main structure */
                                                                /* The extra 2 slots are for 503 processing and for NPP_CLOSING_SESSION_CI */
extern int          G_connections_cnt;                          /* number of open connections */
extern int          G_connections_hwm;                          /* highest number of open connections (high water mark) */
extern char         G_tmp[NPP_TMP_BUFSIZE];                     /* temporary string buffer */
extern eng_session_data_t G_sessions[NPP_MAX_SESSIONS+1];       /* engine session data -- they start from 1 (session 0 is never used) */
extern app_session_data_t G_app_session_data[NPP_MAX_SESSIONS+1]; /* app session data, using the same index (si) */
extern int          G_sessions_cnt;                             /* number of active user sessions */
extern int          G_sessions_hwm;                             /* highest number of active user sessions (high water mark) */
extern sessions_idx_t G_sessions_idx[NPP_MAX_SESSIONS];         /* G_sessions' index, this starts from 0 */

extern time_t       G_now;                                      /* current GMT time (epoch) */
extern struct tm    *G_ptm;                                     /* human readable current time */
extern char         G_last_modified[32];                        /* response header field with server's start time */
extern char         G_header_date[32];                          /* RFC 2822 datetime format */
extern bool         G_initialized;                              /* is server initialization complete? */
extern char         *G_strm;                                    /* for STRM macro */

/* hosts */
#ifdef NPP_MULTI_HOST
extern npp_host_t   G_hosts[NPP_MAX_HOSTS];
extern int          G_hosts_cnt;
#endif

/* messages */
extern npp_message_t G_messages[NPP_MAX_MESSAGES];
extern int          G_messages_cnt;

/* strings */
extern npp_string_t G_strings[NPP_MAX_STRINGS];
extern int          G_strings_cnt;

/* snippets */
extern snippet_t    G_snippets[NPP_MAX_SNIPPETS];
extern int          G_snippets_cnt;

#ifdef NPP_HTTPS
extern bool         G_ssl_lib_initialized;
#endif

#ifdef NPP_MYSQL
//#ifndef __cplusplus
extern MYSQL        *G_dbconn;                  /* database connection */
//#endif
#endif  /* NPP_MYSQL */

/* asynchorous processing */
#ifdef NPP_ASYNC
extern char         G_req_queue_name[256];
extern char         G_res_queue_name[256];
extern mqd_t        G_queue_req;                /* request queue */
extern mqd_t        G_queue_res;                /* response queue */
extern int          G_async_req_data_size;      /* how many bytes are left for data */
extern int          G_async_res_data_size;      /* how many bytes are left for data */
#endif  /* NPP_ASYNC */

extern char         G_dt_string_gmt[128];       /* datetime string for database or log (YYYY-MM-DD hh:mm:ss) */

extern menu_item_t  G_menu[NPP_MAX_MENU_ITEMS+1];
extern int          G_menu_cnt;

#ifdef NPP_SVC
extern async_req_t  G_svc_req;
extern async_res_t  G_svc_res;
#ifdef NPP_OUT_CHECK_REALLOC
extern char         *G_svc_out_data;
#endif
extern char         *G_svc_p_content;
extern char         G_service[NPP_SVC_NAME_LEN+1];
extern int          G_error_code;
extern int          G_svc_si;
#endif  /* NPP_SVC */

extern char         G_blacklist[NPP_MAX_BLACKLIST+1][INET6_ADDRSTRLEN];
extern int          G_blacklist_cnt;            /* G_blacklist length */

extern char         G_whitelist[NPP_MAX_WHITELIST+1][INET6_ADDRSTRLEN];
extern int          G_whitelist_cnt;            /* G_whitelist length */
/* counters */
extern npp_counters_t G_cnts_today;             /* today's counters */
extern npp_counters_t G_cnts_yesterday;         /* yesterday's counters */
extern npp_counters_t G_cnts_day_before;        /* day before's counters */
/* CALL_HTTP */
extern int          G_call_http_status;         /* last HTTP call response status */
extern unsigned     G_call_http_req_cnt;        /* HTTP calls counter */
extern double       G_call_http_elapsed;        /* HTTP calls elapsed for calculating average */
extern double       G_call_http_average;        /* HTTP calls average elapsed */
extern char         G_call_http_content_type[NPP_MAX_VALUE_LEN+1];
extern int          G_call_http_res_len;
extern int          G_new_user_id;
extern int          G_qs_len;


#ifdef __cplusplus
}   /* extern "C" */
#endif



/* --------------------------------------------------------------------------
   prototypes
-------------------------------------------------------------------------- */

#ifdef NPP_CPP_STRINGS
    void npp_require_auth(const char *host, const std::string& path, char level);
    void npp_require_auth(const std::string& host, const std::string& path, char level);

    void npp_add_to_static_res(const char *host, const std::string& name, const std::string& src);
    void npp_add_to_static_res(const std::string& host, const std::string& name, const std::string& src);

#ifdef NPP_APP_CUSTOM_ACTIVATION_EMAIL
    int  npp_app_custom_activation_email(int ci, int user_id, const std::string& email, const std::string& linkkey);
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

    /* public */

#ifndef NPP_CPP_STRINGS
    void npp_require_auth(const char *host, const char *path, char level);
    void npp_add_to_static_res(const char *host, const char *name, const char *src);
#endif

    /* public internal */

#ifdef NPP_HTTPS
    bool npp_eng_init_ssl(void);
#endif

    int  npp_eng_session_start(int ci, const char *sessid);

#ifdef NPP_MULTI_HOST
    int  npp_eng_find_si(int host_id, const char *sessid);
#else
    int  npp_eng_find_si(const char *sessid);
#endif

    void npp_eng_session_downgrade_by_uid(int user_id, int ci);
    bool npp_eng_call_async(int ci, const char *service, const char *data, bool want_response, int timeout, int size);
    void npp_eng_read_blocked_ips(void);
    void npp_eng_read_allowed_ips(void);
    void npp_eng_block_ip(const char *value, bool autoblocked);
    bool npp_eng_is_uri(int ci, const char *uri);
    void npp_eng_out_check(int ci, const char *str);
    void npp_eng_out_check_realloc(int ci, const char *str);
    void npp_eng_out_check_realloc_bin(int ci, const char *data, int len);
    char *npp_eng_get_header(int ci, const char *header);
    void npp_eng_call_http_pass_header(int ci, const char *header);
#ifdef NPP_SVC
    void npp_svc_out_check(const char *str);
    void npp_svc_out_check_realloc(const char *str);
    void npp_svc_out_check_realloc_bin(const char *data, int len);
#endif  /* NPP_SVC */

    /* public app */

#ifdef NPP_APP
    bool npp_app_init(int argc, char *argv[]);
    void npp_app_done(void);
    void npp_app_main(int ci);
    bool npp_app_session_init(int ci);
    void npp_app_session_done(int ci);
#ifdef NPP_APP_EVERY_SPARE_SECOND
    void npp_app_every_spare_second(void);
#endif
#ifdef NPP_APP_EVERY_MINUTE
    void npp_app_every_minute(void);
#endif
#endif  /* NPP_APP */

#ifdef NPP_SVC
    bool npp_svc_init(void);
    void npp_svc_main(int ci);
    void npp_svc_done(void);
#endif  /* NPP_SVC */

#ifdef NPP_APP_CUSTOM_MESSAGE_PAGE
    void npp_app_custom_message_page(int ci, int code);
#endif

#ifdef NPP_APP_CUSTOM_ACTIVATION_EMAIL
#ifndef NPP_CPP_STRINGS
    int  npp_app_custom_activation_email(int ci, int user_id, const char *email, const char *linkkey);
#endif
#endif

#ifdef NPP_USERS
    bool npp_app_user_login(int ci);
    void npp_app_user_logout(int ci);
#endif

#ifdef __cplusplus
}   /* extern "C" */
#endif



#endif  /* NPP_H */
