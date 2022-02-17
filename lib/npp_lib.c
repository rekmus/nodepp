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
    General purpose library
    nodepp.org

-------------------------------------------------------------------------- */

#include "npp.h"

#ifdef NPP_ICONV
#include <iconv.h>
#include <locale.h>
#endif



/* globals (see npp.h for comments) */

int         G_test=0;
int         G_logLevel=LOG_INF;
int         G_logToStdout=0;
int         G_logCombined=0;
int         G_callHTTPTimeout=CALL_HTTP_DEFAULT_TIMEOUT;

/* database */
char        G_dbHost[128]="";
int         G_dbPort=0;
char        G_dbName[128]="";
char        G_dbUser[128]="";
char        G_dbPassword[128]="";
#ifdef NPP_MYSQL
MYSQL       *G_dbconn=NULL;
#endif

FILE        *G_log_fd=NULL;
bool        G_endianness=ENDIANNESS_LITTLE;
int         G_pid=0;
char        G_appdir[256]="..";
char        G_tmp[NPP_TMP_BUFSIZE];
time_t      G_now=0;
struct tm   *G_ptm=NULL;
bool        G_initialized=0;
char        *G_strm=NULL;

/* hosts */
#ifdef NPP_MULTI_HOST
npp_host_t  G_hosts[NPP_MAX_HOSTS]={{"", "res", "resmin", "snippets", FALSE}};
int         G_hosts_cnt=1;
#endif

#if __GNUC__ < 6
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif

/* messages */
npp_message_t G_messages[NPP_MAX_MESSAGES]={0};
int         G_next_msg=0;
npp_lang_t  G_msg_lang[NPP_MAX_LANGUAGES]={0};
int         G_next_msg_lang=0;

/* strings */
npp_string_t G_strings[NPP_MAX_STRINGS]={0};
int         G_next_str=0;
npp_lang_t  G_str_lang[NPP_MAX_LANGUAGES]={0};
int         G_next_str_lang=0;

snippet_t   G_snippets[NPP_MAX_SNIPPETS]={0};
int         G_snippets_cnt=0;

#ifdef NPP_HTTPS
bool        G_ssl_lib_initialized=FALSE;
#endif

char        G_dt_string_gmt[128]="";
unsigned    G_call_http_req_cnt=0;
double      G_call_http_elapsed=0;
double      G_call_http_average=0;
int         G_call_http_status;
char        G_call_http_content_type[NPP_MAX_VALUE_LEN+1];
int         G_call_http_res_len=0;
int         G_qs_len=0;


/* locals */

static char *M_conf=NULL;               /* config file content */

static char *M_md_dest;
static char M_md_list_type;

#ifndef _WIN32
static int  M_shmid[NPP_MAX_SHM_SEGMENTS]={0}; /* SHM id-s */
#endif

#if __GNUC__ < 6
#pragma GCC diagnostic pop  /* end of -Wmissing-braces */
#endif

#ifdef _WIN32   /* Windows */
static WSADATA M_wsa;
static bool M_WSA_initialized=FALSE;
#endif

static call_http_header_t M_call_http_headers[CALL_HTTP_MAX_HEADERS];
static int M_call_http_headers_cnt=0;
#ifdef _WIN32   /* Windows */
static SOCKET M_call_http_socket;
#else
static int M_call_http_socket;
#endif  /* _WIN32 */
#ifdef NPP_HTTPS
static SSL_CTX *M_ssl_ctx=NULL;
static SSL *M_call_http_ssl=NULL;
#else
static void *M_call_http_ssl=NULL;    /* dummy */
#endif  /* NPP_HTTPS */
static char M_call_http_mode;

static bool M_call_http_proxy=FALSE;

static unsigned char M_random_numbers[NPP_RANDOM_NUMBERS];
static char M_random_initialized=0;


static void load_err_messages(void);
#ifndef NPP_CLIENT
static bool load_strings(void);
#endif


/* --------------------------------------------------------------------------
   Library init
-------------------------------------------------------------------------- */
bool npp_lib_init(bool start_log, const char *log_prefix)
{
    /* log file fd */

    G_log_fd = stdout;

    /* G_pid */

    G_pid = getpid();

    /* G_appdir */

    npp_lib_get_app_dir();

    /* time globals */

    npp_update_time_globals();

    /* messages */

    load_err_messages();

#ifndef NPP_CLIENT

    /* strings */

    if ( !load_strings() )
        return FALSE;

    /* snippets */

    int i;
    for ( i=0; i<NPP_MAX_SNIPPETS; ++i )
        strcpy(G_snippets[i].name, "-");

#endif  /* NPP_CLIENT */

    /* read the config file or set defaults */

    npp_lib_read_conf(TRUE);

    /* start log */

    if ( start_log && !npp_log_start(log_prefix, G_test, FALSE) )
        return FALSE;

    int conf_logLevel;

    if ( !start_log )   /* don't log too much here */
    {
        conf_logLevel = G_logLevel;

//#ifndef NPP_DEBUG
        G_logLevel = LOG_ERR;
//#endif
    }

    /* init random module */

    npp_lib_init_random_numbers();

    /* ICONV */

#ifdef NPP_ICONV
    setlocale(LC_ALL, "");
#endif

    /* database */

#ifdef NPP_MYSQL

    if ( !npp_open_db() )
        return FALSE;

#endif  /* NPP_MYSQL */

    /* restore configured log level */

    if ( !start_log )
        G_logLevel = conf_logLevel;

    return TRUE;
}


/* --------------------------------------------------------------------------
   Library clean up
-------------------------------------------------------------------------- */
void npp_lib_done()
{
    DBG("npp_lib_done");

#ifdef NPP_MYSQL
    npp_close_db();
#endif

#ifndef _WIN32

    int i;
    for ( i=0; i<NPP_MAX_SHM_SEGMENTS; ++i )
        npp_lib_shm_delete(i);

#endif  /* _WIN32 */

#ifdef _WIN32
    if ( M_WSA_initialized )
        WSACleanup();
#endif  /* _WIN32 */

    npp_log_finish();
}


/* --------------------------------------------------------------------------
   Copy 0-terminated UTF-8 string safely
   dst_len is in bytes and excludes terminating 0
   dst_len must be > 0
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
void npp_safe_copy(char *dst, const std::string& src_, size_t dst_len)
{
    const char *src = src_.c_str();
#else
void npp_safe_copy(char *dst, const char *src, size_t dst_len)
{
#endif
    DDBG("npp_safe_copy [%s], dst_len = %u", src, dst_len);

#if __GNUC__ > 7
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

    strncpy(dst, src, dst_len+1);

#if __GNUC__ > 7
#pragma GCC diagnostic pop
#endif

    if ( dst[dst_len] == EOS )
    {
        DDBG("not truncated");
        return;   /* not truncated */
    }

    /* ------------------------- */
    /* string has been truncated */

    if ( !UTF8_ANY(dst[dst_len]) )
    {
        dst[dst_len] = EOS;
        DDBG("truncated string won't break the UTF-8 sequence");
        return;
    }

    /* ------------------------------- */
    /* string ends with UTF-8 sequence */

    /* cut until beginning of the sequence found */

    while ( !UTF8_START(dst[dst_len]) )
    {
        DDBG("UTF-8 sequence byte (%x)", dst[dst_len]);

        if ( dst_len == 0 )
        {
            dst[0] = EOS;
            return;
        }

        dst_len--;
    }

    dst[dst_len] = EOS;
}


/* --------------------------------------------------------------------------
   Binary to string
---------------------------------------------------------------------------*/
void npp_bin2hex(char *dst, const unsigned char *src, int len)
{
    char *d=dst;
    char hex[4];
    int i;

    for ( i=0; i<len; ++i )
    {
        sprintf(hex, "%02x", src[i]);
        d = stpcpy(d, hex);
    }

    *d = EOS;
}




#ifndef NPP_CLIENT

/* --------------------------------------------------------- */
/* Time zone handling -------------------------------------- */

/* --------------------------------------------------------------------------
   Set client's time zone offset on the server
-------------------------------------------------------------------------- */
void npp_set_tz(int ci)
{
    DBG("npp_set_tz");

    /* Log user's time zone */

    QSVAL tz;

    if ( QS("tz", tz) )
        INF("Time zone: %s", tz);

    /* set it only once */

    SESSION.tz_set = TRUE;

    /* time zone offset */

    int tzo;

    if ( !QSI("tzo", &tzo) ) return;

    DBG("tzo=%d", tzo);

    if ( tzo < -900 || tzo > 900 ) return;

    SESSION.tz_offset = tzo * -1;
}


/* --------------------------------------------------------------------------
   Calculate client's local time
-------------------------------------------------------------------------- */
time_t npp_ua_time(int ci)
{
    return G_now + SESSION.tz_offset * 60;
}


/* --------------------------------------------------------------------------
   Return client's local today string
-------------------------------------------------------------------------- */
char *npp_today_ua(int ci)
{
static char today[11];
    strncpy(today, DT_NOW_LOCAL, 10);
    today[10] = EOS;
    return today;
}
#endif  /* NPP_CLIENT */


/* --------------------------------------------------------------------------
   Return today string
-------------------------------------------------------------------------- */
char *npp_today_gmt()
{
static char today[11];
    strncpy(today, DT_NOW_GMT, 10);
    today[10] = EOS;
    return today;
}



/* --------------------------------------------------------- */
/* MD parsing ---------------------------------------------- */

#define MD_TAG_NONE         '0'
#define MD_TAG_B            'b'
#define MD_TAG_I            'i'
#define MD_TAG_U            'u'
#define MD_TAG_CODE         'c'
#define MD_TAG_P            'p'
#define MD_TAG_H1           '1'
#define MD_TAG_H2           '2'
#define MD_TAG_H3           '3'
#define MD_TAG_H4           '4'
#define MD_TAG_LI           'l'
#define MD_TAG_ACC_BR       'B'
#define MD_TAG_EOD          '~'

#define MD_LIST_ORDERED     'O'
#define MD_LIST_UNORDERED   'U'

#define IS_TAG_BLOCK        (tag==MD_TAG_P || tag==MD_TAG_H1 || tag==MD_TAG_H2 || tag==MD_TAG_H3 || tag==MD_TAG_H4 || tag==MD_TAG_LI)


/* --------------------------------------------------------------------------
   Detect MD tag
-------------------------------------------------------------------------- */
static int detect_tag(const char *src, char *tag, bool start, bool newline, bool nested)
{
    int skip=0;

    if ( newline )
    {
        ++src;   /* skip '\n' */
        ++skip;
    }

    if ( start || newline || nested )
    {
        while ( *src=='\r' || *src==' ' || *src=='\t' )
        {
            ++src;
            ++skip;
        }
    }

    if ( *src=='*' )   /* bold, italic or list item */
    {
        if ( *(src+1)=='*' )
        {
            *tag = MD_TAG_B;
            skip += 2;
        }
        else if ( (start || newline) && *(src+1)==' ' )
        {
            *tag = MD_TAG_LI;
            skip += 2;
            M_md_list_type = MD_LIST_UNORDERED;
        }
        else    /* italic */
        {
            *tag = MD_TAG_I;
            skip += 1;
        }
    }
    else if ( *src=='_' )   /* underline */
    {
        if ( start || newline || *(src-1)==' ' )
        {
            *tag = MD_TAG_U;
            skip += 1;
        }
        else
            *tag = MD_TAG_NONE;
    }
    else if ( *src=='`' )   /* monospace */
    {
        *tag = MD_TAG_CODE;
        skip += 1;
    }
    else if ( (start || newline || nested) && isdigit(*src) && *(src+1)=='.' && *(src+2)==' ' )   /* single-digit ordered list */
    {
        *tag = MD_TAG_LI;
        skip += 3;
        M_md_list_type = MD_LIST_ORDERED;
    }
    else if ( (start || newline || nested) && isdigit(*src) && isdigit(*(src+1)) && *(src+2)=='.' && *(src+3)==' ' )   /* double-digit ordered list */
    {
        *tag = MD_TAG_LI;
        skip += 4;
        M_md_list_type = MD_LIST_ORDERED;
    }
    else if ( (start || newline || nested) && *src=='#' )    /* headers */
    {
        if ( *(src+1)=='#' )
        {
            if ( *(src+2)=='#' )
            {
                if ( *(src+3)=='#' )
                {
                    *tag = MD_TAG_H4;
                    skip += 5;
                }
                else
                {
                    *tag = MD_TAG_H3;
                    skip += 4;
                }
            }
            else
            {
                *tag = MD_TAG_H2;
                skip += 3;
            }
        }
        else
        {
            *tag = MD_TAG_H1;
            skip += 2;
        }
    }
    else if ( start || nested || (newline && *src=='\n') )    /* paragraph */
    {
        if ( start )
        {
            *tag = MD_TAG_P;
        }
        else if ( *(src+1) == EOS )
        {
            *tag = MD_TAG_EOD;
        }
        else if ( nested )
        {
            *tag = MD_TAG_P;
        }
        else    /* block tag begins */
        {
            skip += detect_tag(src, tag, false, true, true);
        }
    }
    else if ( *src )
    {
        *tag = MD_TAG_ACC_BR;   /* accidental line break perhaps */
    }
    else    /* end of document */
    {
        *tag = MD_TAG_EOD;
    }

    return skip;
}


/* --------------------------------------------------------------------------
   Open HTML tag
-------------------------------------------------------------------------- */
static int open_tag(char tag)
{
    int written=0;

    if ( tag == MD_TAG_B )
    {
        M_md_dest = stpcpy(M_md_dest, "<b>");
        written = 3;
    }
    else if ( tag == MD_TAG_I )
    {
        M_md_dest = stpcpy(M_md_dest, "<i>");
        written = 3;
    }
    else if ( tag == MD_TAG_U )
    {
        M_md_dest = stpcpy(M_md_dest, "<u>");
        written = 3;
    }
    else if ( tag == MD_TAG_CODE )
    {
        M_md_dest = stpcpy(M_md_dest, "<code>");
        written = 6;
    }
    else if ( tag == MD_TAG_P )
    {
        M_md_dest = stpcpy(M_md_dest, "<p>");
        written = 3;
    }
    else if ( tag == MD_TAG_H1 )
    {
        M_md_dest = stpcpy(M_md_dest, "<h1>");
        written = 4;
    }
    else if ( tag == MD_TAG_H2 )
    {
        M_md_dest = stpcpy(M_md_dest, "<h2>");
        written = 4;
    }
    else if ( tag == MD_TAG_H3 )
    {
        M_md_dest = stpcpy(M_md_dest, "<h3>");
        written = 4;
    }
    else if ( tag == MD_TAG_H4 )
    {
        M_md_dest = stpcpy(M_md_dest, "<h4>");
        written = 4;
    }
    else if ( tag == MD_TAG_LI )
    {
        M_md_dest = stpcpy(M_md_dest, "<li>");
        written = 4;
    }

    return written;
}


/* --------------------------------------------------------------------------
   Close HTML tag
-------------------------------------------------------------------------- */
static int close_tag(const char *src, char tag)
{
    int written=0;

    if ( tag == MD_TAG_B )
    {
        M_md_dest = stpcpy(M_md_dest, "</b>");
        written = 4;
    }
    else if ( tag == MD_TAG_I )
    {
        M_md_dest = stpcpy(M_md_dest, "</i>");
        written = 4;
    }
    else if ( tag == MD_TAG_U )
    {
        M_md_dest = stpcpy(M_md_dest, "</u>");
        written = 4;
    }
    else if ( tag == MD_TAG_CODE )
    {
        M_md_dest = stpcpy(M_md_dest, "</code>");
        written = 7;
    }
    else if ( tag == MD_TAG_P )
    {
        M_md_dest = stpcpy(M_md_dest, "</p>");
        written = 4;
    }
    else if ( tag == MD_TAG_H1 )
    {
        M_md_dest = stpcpy(M_md_dest, "</h1>");
        written = 5;
    }
    else if ( tag == MD_TAG_H2 )
    {
        M_md_dest = stpcpy(M_md_dest, "</h2>");
        written = 5;
    }
    else if ( tag == MD_TAG_H3 )
    {
        M_md_dest = stpcpy(M_md_dest, "</h3>");
        written = 5;
    }
    else if ( tag == MD_TAG_H4 )
    {
        M_md_dest = stpcpy(M_md_dest, "</h4>");
        written = 5;
    }
    else if ( tag == MD_TAG_LI )
    {
        M_md_dest = stpcpy(M_md_dest, "</li>");
        written = 5;
    }

    return written;
}


/* --------------------------------------------------------------------------
   Render simplified markdown to HTML
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
char *npp_render_md(char *dest, const std::string& src_, size_t dest_len)
{
    const char *src = src_.c_str();
#else
char *npp_render_md(char *dest, const char *src, size_t dest_len)
{
#endif
    int  pos=0;    /* source position */
    char tag;
    char tag_b=MD_TAG_NONE;   /* block */
    char tag_i=MD_TAG_NONE;   /* inline */
    int  skip;
    size_t written=0;
    bool list=FALSE;
    bool escape=FALSE;

    M_md_dest = dest;

    if ( dest_len < 40 )
    {
        *M_md_dest = EOS;
        return dest;
    }

    skip = detect_tag(src, &tag, true, false, false);

    DDBG("tag %c detected", tag);

    if ( skip )
    {
        src += skip;
        pos += skip;
    }

    if ( tag == MD_TAG_LI )
    {
        DDBG("Starting unordered list");

        M_md_dest = stpcpy(M_md_dest, "<ul>");
        written += 4;
        list = 1;
    }

    if ( IS_TAG_BLOCK )
    {
        tag_b = tag;
        written += open_tag(tag_b);
    }
    else    /* inline */
    {
        tag_b = MD_TAG_P;   /* there should always be a block */
        written += open_tag(tag_b);

        tag_i = tag;
        written += open_tag(tag_i);
    }

    const char *prev1=src, *prev2=src;  /* init only to keep -Wmaybe-uninitialized off */

    while ( *src && written < dest_len-18 )   /* worst case: </code></li></ul> */
    {
        DDBG("%c", *src);

        if ( pos > 0 )
        {
            prev1 = src - 1;

            if ( pos > 1 )
                prev2 = src - 2;
        }

        if ( *src == '\\' && !escape )
        {
            escape = true;
        }
        else if ( *src=='*' && (tag_i==MD_TAG_B || tag_i==MD_TAG_I) && !escape )
        {
            DDBG("Closing inline tag %c", tag_i);

            written += close_tag(src, tag_i);

            if ( tag_i==MD_TAG_B )    /* double-char code */
            {
                ++src;
                ++pos;
            }

            tag_i = MD_TAG_NONE;
        }
        else if ( *src=='_' && tag_i==MD_TAG_U && !escape )
        {
            DDBG("Closing inline tag %c", tag_i);
            written += close_tag(src, tag_i);
            tag_i = MD_TAG_NONE;
        }
        else if ( *src=='`' && tag_i==MD_TAG_CODE && !escape )
        {
            DDBG("Closing inline tag %c", tag_i);
            written += close_tag(src, tag_i);
            tag_i = MD_TAG_NONE;
        }
        else if ( (*src=='*' || *src=='_' || *src=='`') && !escape )    /* inline tags */
        {
            if ( !pos || *(src-1)=='\r' || *(src-1)=='\n' || *(src-1)==' ' || *(src-1)=='(' )    /* opening tag */
            {
                skip = detect_tag(src, &tag, false, false, false);

                DDBG("tag %c detected", tag);

                if ( skip )
                {
                    src += skip;
                    pos += skip;
                }

                if ( IS_TAG_BLOCK )
                {
                    tag_b = tag;
                    written += open_tag(tag_b);
                }
                else    /* inline */
                {
                    tag_i = tag;
                    written += open_tag(tag_i);
                }

                if ( tag != MD_TAG_NONE && pos )
                {
                    src--;
                    pos--;
                }
            }
            else    /* copy character to dest */
            {
                *M_md_dest++ = *src;
                ++written;
                escape = false;
            }
        }
        else if ( pos && *src=='-' && *prev1=='-' )   /* convert -- to ndash */
        {
            M_md_dest--;
            M_md_dest = stpcpy(M_md_dest, "–");
            written += 3;
        }
        else if ( *src=='\n' && pos>1 && *prev1==' ' && *prev2==' ' )   /* convert trailing double space to <br> */
        {
            M_md_dest -= 2;
            M_md_dest = stpcpy(M_md_dest, "<br>");
            written += 2;
        }
        else if ( *src=='\n' )   /* block tags */
        {
            skip = detect_tag(src, &tag, false, true, false);

            DDBG("tag %c detected", tag);

            if ( skip )
            {
                src += skip;
                pos += skip;
            }

            if ( tag != MD_TAG_NONE && tag != MD_TAG_ACC_BR && tag_b != MD_TAG_NONE )
            {
                DDBG("Closing block tag %c", tag_b);
                written += close_tag(src, tag_b);
                tag_b = MD_TAG_NONE;
            }

            if ( tag == MD_TAG_LI )
            {
                if ( !list )   /* start a list */
                {
                    DDBG("Starting %sordered list", M_md_list_type==MD_LIST_ORDERED?"":"un");

                    if ( M_md_list_type == MD_LIST_ORDERED )
                        M_md_dest = stpcpy(M_md_dest, "<ol>");
                    else
                        M_md_dest = stpcpy(M_md_dest, "<ul>");

                    list = 1;
                    written += 4;
                }
            }
            else if ( tag == MD_TAG_ACC_BR )   /* accidental line break */
            {
                M_md_dest = stpcpy(M_md_dest, " ");
                ++written;
            }
            else if ( list )   /* close the list */
            {
                DDBG("Closing %sordered list", M_md_list_type==MD_LIST_ORDERED?"":"un");

                if ( M_md_list_type == MD_LIST_ORDERED )
                    M_md_dest = stpcpy(M_md_dest, "</ol>");
                else
                    M_md_dest = stpcpy(M_md_dest, "</ul>");

                list = 0;
                written += 5;
            }

            if ( IS_TAG_BLOCK )
            {
                tag_b = tag;
                written += open_tag(tag_b);
            }
            else if ( tag != MD_TAG_NONE && tag != MD_TAG_ACC_BR && tag != MD_TAG_EOD )   /* inline */
            {
                tag_b = MD_TAG_P;   /* always open block tag */
                written += open_tag(tag_b);

                tag_i = tag;    /* open inline tag */
                written += open_tag(tag_i);
            }

            if ( pos )
            {
                src--;
                pos--;
            }
        }
        else    /* copy character to dest */
        {
            *M_md_dest++ = *src;
            ++written;
            escape = false;
        }

        ++src;
        ++pos;
    }

    if ( tag_b != MD_TAG_NONE )
    {
        DDBG("Closing block tag %c", tag_b);

        written += close_tag(src, tag_b);

        if ( list )    /* close a list */
        {
            DDBG("Closing %sordered list", M_md_list_type==MD_LIST_ORDERED?"":"un");

            if ( M_md_list_type == MD_LIST_ORDERED )
                M_md_dest = stpcpy(M_md_dest, "</ol>");
            else
                M_md_dest = stpcpy(M_md_dest, "</ul>");

            written += 5;
        }
    }

    *M_md_dest = EOS;

#ifdef NPP_DEBUG
    npp_log_long(dest, written, "npp_render_md result");
#endif

    return dest;
}


/* --------------------------------------------------------------------------
   Encode string for JSON
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
char *npp_json_escape_string(const std::string& src_)
{
    const char *src = src_.c_str();
#else
char *npp_json_escape_string(const char *src)
{
#endif
static char dst[NPP_JSON_STR_LEN*2+1];
    int cnt=0;

    while ( *src && cnt < NPP_JSON_STR_LEN*2-2 )
    {
        if ( *src=='\t' )
        {
            dst[cnt++] = '\\';
            dst[cnt++] = 't';
        }
        else if ( *src=='\n' )
        {
            dst[cnt++] = '\\';
            dst[cnt++] = 'n';
        }
        else if ( *src=='\r' )
        {
            /* ignore */
        }
        else
        {
            dst[cnt++] = *src;
        }

        ++src;
    }

    dst[cnt] = EOS;

    return dst;
}


/* --------------------------------------------------------------------------
   Verify CSRF token
-------------------------------------------------------------------------- */
bool npp_csrft_ok(int ci)
{
#ifndef NPP_CLIENT

    QSVAL csrft;

    if ( !QS("csrft", csrft) ) return FALSE;

    if ( 0 != strcmp(csrft, SESSION.csrft) ) return FALSE;

#endif  /* NPP_CLIENT */

    return TRUE;
}


/* --------------------------------------------------------------------------
   Load error messages
-------------------------------------------------------------------------- */
static void load_err_messages()
{
    npp_add_message(OK,                        "EN-US", "OK");
    npp_add_message(ERR_INVALID_REQUEST,       "EN-US", "Invalid HTTP request");
    npp_add_message(ERR_UNAUTHORIZED,          "EN-US", "Unauthorized");
    npp_add_message(ERR_FORBIDDEN,             "EN-US", "Forbidden");
    npp_add_message(ERR_NOT_FOUND,             "EN-US", "Page not found");
    npp_add_message(ERR_METHOD,                "EN-US", "Method not allowed");
    npp_add_message(ERR_INT_SERVER_ERROR,      "EN-US", "Apologies, this is our fault. Please try again later.");
    npp_add_message(ERR_SERVER_TOOBUSY,        "EN-US", "Apologies, we are experiencing very high demand right now, please try again in a few minutes.");
    npp_add_message(ERR_FILE_TOO_BIG,          "EN-US", "File too big");
    npp_add_message(ERR_REDIRECTION,           "EN-US", "Redirection required");
    npp_add_message(ERR_ASYNC_NO_SUCH_SERVICE, "EN-US", "No such service");
    npp_add_message(ERR_ASYNC_TIMEOUT,         "EN-US", "Asynchronous service timeout");
    npp_add_message(ERR_REMOTE_CALL,           "EN-US", "Couldn't call the remote service");
    npp_add_message(ERR_REMOTE_CALL_STATUS,    "EN-US", "Remote service call returned unsuccessful status");
    npp_add_message(ERR_REMOTE_CALL_DATA,      "EN-US", "Data returned from the remote service is invalid");
    npp_add_message(ERR_CSRFT,                 "EN-US", "Your previous session has expired. Please refresh this page before trying again.");
    npp_add_message(ERR_RECORD_NOT_FOUND,      "EN-US", "Record not found");
}


#ifndef NPP_CPP_STRINGS
/* --------------------------------------------------------------------------
   Add error message
-------------------------------------------------------------------------- */
void npp_add_message(int code, const char *lang, const char *message, ...)
{
    if ( G_next_msg >= NPP_MAX_MESSAGES )
    {
        ERR("NPP_MAX_MESSAGES (%d) has been reached", NPP_MAX_MESSAGES);
        return;
    }

    va_list plist;
    char buffer[NPP_MAX_MESSAGE_LEN+1];

    /* compile message with arguments into buffer */

    va_start(plist, message);
    vsprintf(buffer, message, plist);
    va_end(plist);

    G_messages[G_next_msg].code = code;
    if ( lang )
        strcpy(G_messages[G_next_msg].lang, npp_upper(lang));
    strcpy(G_messages[G_next_msg].message, buffer);

    ++G_next_msg;

    /* in case message was added after init */

    if ( G_initialized )
        npp_sort_messages();
}
#endif  /* NPP_CPP_STRINGS */


/* --------------------------------------------------------------------------
   Comparing function for messages
---------------------------------------------------------------------------*/
static int compare_messages(const void *a, const void *b)
{
    const npp_message_t *p1 = (npp_message_t*)a;
    const npp_message_t *p2 = (npp_message_t*)b;

    int res = strcmp(p1->lang, p2->lang);

    if ( res > 0 )
        return 1;
    else if ( res < 0 )
        return -1;

    /* same language then */

    if ( p1->code < p2->code )
        return -1;
    else if ( p1->code > p2->code )
        return 1;
    else
        return 0;
}


/* --------------------------------------------------------------------------
   Sort and index messages by languages
-------------------------------------------------------------------------- */
void npp_sort_messages()
{
    qsort(&G_messages, G_next_msg, sizeof(npp_message_t), compare_messages);

    int i;

    for ( i=0; i<G_next_msg; ++i )
    {
        if ( 0 != strcmp(G_messages[i].lang, G_msg_lang[G_next_msg_lang].lang) )
        {
            if ( G_next_msg_lang ) G_msg_lang[G_next_msg_lang-1].next_lang_index = i;

            strcpy(G_msg_lang[G_next_msg_lang].lang, G_messages[i].lang);
            G_msg_lang[G_next_msg_lang].first_index = i;
            ++G_next_msg_lang;
        }
    }

    G_msg_lang[G_next_msg_lang-1].next_lang_index = G_next_msg;
}


/* --------------------------------------------------------------------------
   Get message category
-------------------------------------------------------------------------- */
static char *get_msg_cat(int code)
{
static char cat[64];

    if ( code == OK )
    {
        strcpy(cat, MSG_CAT_OK);
    }
    else if ( code < ERR_MAX_ENGINE_ERROR )
    {
        strcpy(cat, MSG_CAT_ERROR);
    }
#ifdef NPP_USERS
    else if ( code < ERR_MAX_USR_LOGIN_ERROR )
    {
        strcpy(cat, MSG_CAT_USR_LOGIN);
    }
    else if ( code < ERR_MAX_USR_EMAIL_ERROR )
    {
        strcpy(cat, MSG_CAT_USR_EMAIL);
    }
    else if ( code < ERR_MAX_USR_PASSWORD_ERROR )
    {
        strcpy(cat, MSG_CAT_USR_PASSWORD);
    }
    else if ( code < ERR_MAX_USR_REPEAT_PASSWORD_ERROR )
    {
        strcpy(cat, MSG_CAT_USR_REPEAT_PASSWORD);
    }
    else if ( code < ERR_MAX_USR_OLD_PASSWORD_ERROR )
    {
        strcpy(cat, MSG_CAT_USR_OLD_PASSWORD);
    }
    else if ( code < ERR_MAX_USR_ERROR )
    {
        strcpy(cat, MSG_CAT_ERROR);
    }
    else if ( code < WAR_MAX_USR_WARNING )
    {
        strcpy(cat, MSG_CAT_WARNING);
    }
    else if ( code < MSG_MAX_USR_MESSAGE )
    {
        strcpy(cat, MSG_CAT_MESSAGE);
    }
#endif  /* NPP_USERS */
    else    /* app error */
    {
        strcpy(cat, MSG_CAT_ERROR);
    }

    return cat;
}


/* --------------------------------------------------------------------------
   Message category test
   Only 3 main categories are recognized:
   error (red), warning (yellow) and message (green)
-------------------------------------------------------------------------- */
bool npp_is_msg_main_cat(int code, const char *arg_cat)
{
    char cat[64];

    strcpy(cat, get_msg_cat(code));

    if ( 0==strcmp(cat, arg_cat) )
        return TRUE;

#ifdef NPP_USERS
    if ( 0==strcmp(arg_cat, MSG_CAT_ERROR) &&
            (0==strcmp(cat, MSG_CAT_USR_LOGIN) || 0==strcmp(cat, MSG_CAT_USR_EMAIL) || 0==strcmp(cat, MSG_CAT_USR_PASSWORD) || 0==strcmp(cat, MSG_CAT_USR_REPEAT_PASSWORD) || 0==strcmp(cat, MSG_CAT_USR_OLD_PASSWORD)) )
        return TRUE;
#endif  /* NPP_USERS */

    return FALSE;
}


#ifndef NPP_CLIENT
/* --------------------------------------------------------------------------
   Get error description for user in NPP_STRINGS_LANG
-------------------------------------------------------------------------- */
static char *lib_get_message_fallback(int code)
{
    int l, m;

    /* try in NPP_STRINGS_LANG */

    for ( l=0; l<G_next_msg_lang; ++l )   /* jump to the right language */
    {
        if ( 0==strcmp(G_msg_lang[l].lang, NPP_STRINGS_LANG) )
        {
            for ( m=G_msg_lang[l].first_index; m<G_msg_lang[l].next_lang_index; ++m )
                if ( G_messages[m].code == code )
                    return G_messages[m].message;
        }
    }

    /* try in any language */

    for ( m=0; m<G_next_msg; ++m )
        if ( G_messages[m].code == code )
            return G_messages[m].message;

    /* not found */

static char unknown[128];
    sprintf(unknown, "Unknown code: %d", code);
    return unknown;
}


/* --------------------------------------------------------------------------
   Get error description for user
   Pick the user session language if possible
   TODO: binary search
-------------------------------------------------------------------------- */
char *npp_get_message(int ci, int code)
{

    if ( 0==strcmp(SESSION.lang, NPP_STRINGS_LANG) )   /* no need to translate */
        return lib_get_message_fallback(code);

    if ( !SESSION.lang[0] )   /* unknown client language */
        return lib_get_message_fallback(code);

    int l, m;

    for ( l=0; l<G_next_msg_lang; ++l )   /* jump to the right language */
    {
        if ( 0==strcmp(G_msg_lang[l].lang, SESSION.lang) )
        {
            for ( m=G_msg_lang[l].first_index; m<G_msg_lang[l].next_lang_index; ++m )
                if ( G_messages[m].code == code )
                    return G_messages[m].message;
        }
    }

    /* if not found, ignore country code */

    for ( l=0; l<G_next_msg_lang; ++l )
    {
        if ( 0==strncmp(G_msg_lang[l].lang, SESSION.lang, 2) )
        {
            for ( m=G_msg_lang[l].first_index; m<G_msg_lang[l].next_lang_index; ++m )
                if ( G_messages[m].code == code )
                    return G_messages[m].message;
        }
    }

    /* fallback */

    return lib_get_message_fallback(code);
}


/* --------------------------------------------------------------------------
   Parse and set strings from data
-------------------------------------------------------------------------- */
static void parse_and_set_strings(const char *lang, const char *data)
{
    DBG("parse_and_set_strings, lang [%s]", lang);

    const char *p=data;
    int  j=0;
    char string_orig[NPP_MAX_STRING_LEN+1];
    char string_in_lang[NPP_MAX_STRING_LEN+1];
    bool now_key=1, now_val=0, now_com=0;

    if ( G_next_str_lang >= NPP_MAX_LANGUAGES )
    {
        ERR("NPP_MAX_LANGUAGES (%d) has been reached", NPP_MAX_LANGUAGES);
        return;
    }

    strcpy(G_str_lang[G_next_str_lang].lang, npp_upper(lang));
    G_str_lang[G_next_str_lang].first_index = G_next_str;

    while ( *p )
    {
        if ( *p=='#' )   /* comment */
        {
            now_key = 0;
            now_com = 1;

            if ( now_val )
            {
                now_val = 0;
                string_in_lang[j] = EOS;
                npp_lib_add_string(lang, string_orig, string_in_lang);
            }
        }
        else if ( now_key && *p==NPP_STRINGS_SEP )   /* separator */
        {
            now_key = 0;
            now_val = 1;
            string_orig[j] = EOS;
            j = 0;
        }
        else if ( *p=='\r' || *p=='\n' )
        {
            if ( now_val )
            {
                now_val = 0;
                string_in_lang[j] = EOS;
                npp_lib_add_string(lang, string_orig, string_in_lang);
            }
            else if ( now_com )
            {
                now_com = 0;
            }

            j = 0;

            now_key = 1;
        }
        else if ( now_key )
        {
            string_orig[j++] = *p;
        }
        else if ( now_val )
        {
            string_in_lang[j++] = *p;
        }

        ++p;
    }

    if ( now_val )
    {
        string_in_lang[j] = EOS;
        npp_lib_add_string(lang, string_orig, string_in_lang);
    }

    G_str_lang[G_next_str_lang].next_lang_index = G_next_str;
    ++G_next_str_lang;
}


/* --------------------------------------------------------------------------
   Load strings
-------------------------------------------------------------------------- */
static bool load_strings()
{
    int     len;
    char    bindir[NPP_STATIC_PATH_LEN+1];      /* full path to bin */
    char    namewpath[NPP_STATIC_PATH_LEN*2];   /* full path including file name */
    DIR     *dir;
    struct dirent *dirent;
    FILE    *fd;
    char    *data=NULL;

    DBG("load_strings");

    if ( G_appdir[0] == EOS ) return TRUE;

#ifdef _WIN32
    sprintf(bindir, "%s\\bin", G_appdir);
#else
    sprintf(bindir, "%s/bin", G_appdir);
#endif
    if ( (dir=opendir(bindir)) == NULL )
    {
        DBG("Couldn't open directory [%s]", bindir);
        return FALSE;
    }

    while ( (dirent=readdir(dir)) )
    {
        if ( 0 != strncmp(dirent->d_name, "strings.", 8) )
            continue;

#ifdef _WIN32
        sprintf(namewpath, "%s\\%s", bindir, dirent->d_name);
#else
        sprintf(namewpath, "%s/%s", bindir, dirent->d_name);
#endif
        DBG("namewpath [%s]", namewpath);

#ifdef _WIN32   /* Windows */
        if ( NULL == (fd=fopen(namewpath, "rb")) )
#else
        if ( NULL == (fd=fopen(namewpath, "r")) )
#endif  /* _WIN32 */
            ERR("Couldn't open %s", namewpath);
        else
        {
            fseek(fd, 0, SEEK_END);     /* determine the file size */
            len = ftell(fd);
            rewind(fd);

            if ( NULL == (data=(char*)malloc(len+1)) )
            {
                ERR("Couldn't allocate %d bytes for %s", len, dirent->d_name);
                fclose(fd);
                closedir(dir);
                return FALSE;
            }

            if ( fread(data, len, 1, fd) != 1 )
            {
                ERR("Couldn't read from %s", dirent->d_name);
                fclose(fd);
                closedir(dir);
                free(data);
                data = NULL;
                return FALSE;
            }

            fclose(fd);
            *(data+len) = EOS;

            parse_and_set_strings(npp_get_file_ext(dirent->d_name), data);

            free(data);
            data = NULL;
        }
    }

    closedir(dir);

    return TRUE;
}


/* --------------------------------------------------------------------------
   Add string
-------------------------------------------------------------------------- */
void npp_lib_add_string(const char *lang, const char *str, const char *str_lang)
{
    if ( G_next_str >= NPP_MAX_STRINGS )
    {
        ERR("NPP_MAX_STRINGS (%d) has been reached", NPP_MAX_STRINGS);
        return;
    }

    strcpy(G_strings[G_next_str].lang, npp_upper(lang));
    strcpy(G_strings[G_next_str].string_upper, npp_upper(str));
    strcpy(G_strings[G_next_str].string_in_lang, str_lang);

    ++G_next_str;
}


/* --------------------------------------------------------------------------
   Get a string
   Pick the user session language if possible
   If not, return given string
   TODO: binary search
-------------------------------------------------------------------------- */
const char *npp_lib_get_string(int ci, const char *str)
{
    if ( 0==strcmp(SESSION.lang, NPP_STRINGS_LANG) )   /* no need to translate */
        return str;

    if ( !SESSION.lang[0] )   /* unknown client language */
        return str;

    char str_upper[NPP_MAX_STRING_LEN+1];

    strcpy(str_upper, npp_upper(str));

    int l, s;

    for ( l=0; l<G_next_str_lang; ++l )   /* jump to the right language */
    {
        if ( 0==strcmp(G_str_lang[l].lang, SESSION.lang) )
        {
            for ( s=G_str_lang[l].first_index; s<G_str_lang[l].next_lang_index; ++s )
                if ( 0==strcmp(G_strings[s].string_upper, str_upper) )
                    return G_strings[s].string_in_lang;

            /* language found but not this string */
            return str;
        }
    }

    /* if not found, ignore country code */

    for ( l=0; l<G_next_str_lang; ++l )
    {
        if ( 0==strncmp(G_str_lang[l].lang, SESSION.lang, 2) )
        {
            for ( s=G_str_lang[l].first_index; s<G_str_lang[l].next_lang_index; ++s )
                if ( 0==strcmp(G_strings[s].string_upper, str_upper) )
                    return G_strings[s].string_in_lang;
        }
    }

    /* fallback */

    return str;
}
#endif  /* NPP_CLIENT */


/* --------------------------------------------------------------------------
   URI encoding
---------------------------------------------------------------------------*/
#ifdef NPP_CPP_STRINGS
char *npp_url_encode(const std::string& src_)
{
    const char *src = src_.c_str();
#else
char *npp_url_encode(const char *src)
{
#endif
static char     dest[NPP_LIB_STR_BUF];
    int         i, j=0;
    const char  hex[]="0123456789ABCDEF";

    for ( i=0; src[i] && j<NPP_LIB_STR_CHECK; ++i )
    {
        if ( (48 <= src[i] && src[i] <= 57) || (65 <= src[i] && src[i] <= 90) || (97 <= src[i] && src[i] <= 122)
                || src[i]=='-' || src[i]=='_' || src[i]=='.' || src[i]=='~' )
        {
            dest[j++] = src[i];
        }
        else
        {
            dest[j++] = '%';
            dest[j++] = hex[(unsigned char)(src[i]) >> 4];
            dest[j++] = hex[(unsigned char)(src[i]) & 15];
        }
    }

    dest[j] = EOS;

    return dest;
}


/* --------------------------------------------------------------------------
   Open database connection
-------------------------------------------------------------------------- */
bool npp_open_db()
{
    int ret=TRUE;

    DBG("npp_open_db");

#ifdef NPP_MYSQL

    if ( !G_dbName[0] )
    {
        ERR("dbName parameter is required in npp.conf");
        return FALSE;
    }

    if ( NULL == (G_dbconn=mysql_init(NULL)) )
    {
        ERR("%u: %s", mysql_errno(G_dbconn), mysql_error(G_dbconn));
        return FALSE;
    }

#ifdef NPP_MYSQL_RECONNECT
    my_bool reconnect=1;
    mysql_options(G_dbconn, MYSQL_OPT_RECONNECT, &reconnect);
#endif

//    unsigned long max_packet=33554432;  /* 32 MB */
//    mysql_options(G_dbconn, MYSQL_OPT_MAX_ALLOWED_PACKET, &max_packet);

    if ( NULL == mysql_real_connect(G_dbconn, G_dbHost[0]?G_dbHost:NULL, G_dbUser, G_dbPassword, G_dbName, G_dbPort, NULL, 0) )
    {
        ERR("%u: %s", mysql_errno(G_dbconn), mysql_error(G_dbconn));
        return FALSE;
    }

    /* for backward compatibility maintain two db connections for the time being */

#ifdef __cplusplus
    try
    {
        Cdb::DBOpen(G_dbName, G_dbUser, G_dbPassword);
    }
    catch (std::exception& e)
    {
        ERR(e.what());
        ret = FALSE;
    }
#endif  /* __cplusplus */

#endif  /* NPP_MYSQL */

    return ret;
}


/* --------------------------------------------------------------------------
   Close database connection
-------------------------------------------------------------------------- */
void npp_close_db()
{
#ifdef NPP_MYSQL
    if ( G_dbconn )
        mysql_close(G_dbconn);
#ifdef __cplusplus
    Cdb::DBClose();
#endif
#endif  /* NPP_MYSQL */
}


/* --------------------------------------------------------------------------
   Return TRUE if file exists and it's readable
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
bool npp_file_exists(const std::string& fname_)
{
    const char *fname = fname_.c_str();
#else
bool npp_file_exists(const char *fname)
{
#endif
    FILE *f=NULL;

    if ( NULL != (f=fopen(fname, "r")) )
    {
        fclose(f);
        return TRUE;
    }

    return FALSE;
}


/* --------------------------------------------------------------------------
   Get the last part of path
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
void npp_get_exec_name(char *dst, const std::string& path_)
{
    const char *path = path_.c_str();
#else
void npp_get_exec_name(char *dst, const char *path)
{
#endif
    const char *p=path;
    const char *pd=NULL;

    while ( *p )
    {
#ifdef _WIN32
        if ( *p == '\\' )
#else
        if ( *p == '/' )
#endif
        {
            if ( *(p+1) )   /* not EOS */
                pd = p+1;
        }
        ++p;
    }

    if ( pd )
        strcpy(dst, pd);
    else
        strcpy(dst, path);

//    DBG("exec name [%s]", dst);
}


/* --------------------------------------------------------------------------
   Update G_now, G_ptm and G_dt_string_gmt
-------------------------------------------------------------------------- */
void npp_update_time_globals()
{
    G_now = time(NULL);
    G_ptm = gmtime(&G_now);
    sprintf(G_dt_string_gmt, "%d-%02d-%02d %02d:%02d:%02d", G_ptm->tm_year+1900, G_ptm->tm_mon+1, G_ptm->tm_mday, G_ptm->tm_hour, G_ptm->tm_min, G_ptm->tm_sec);
}



#ifdef NPP_HTTPS
/* --------------------------------------------------------------------------
   Init SSL for a client
-------------------------------------------------------------------------- */
static bool init_ssl_client()
{
    const SSL_METHOD *method;

    DBG("init_ssl_client");

    if ( !G_ssl_lib_initialized )   /* it might have been initialized by the server */
    {
        DBG("Initializing SSL_lib...");

        /* libssl init */
        SSL_library_init();
        SSL_load_error_strings();

        /* libcrypto init */
        OpenSSL_add_all_algorithms();
        ERR_load_crypto_strings();

        G_ssl_lib_initialized = TRUE;
    }

    method = SSLv23_client_method();    /* negotiate the highest protocol version supported by both the server and the client */

    DDBG("before SSL_CTX_new");

    M_ssl_ctx = SSL_CTX_new(method);    /* create new context from method */

    DDBG("after SSL_CTX_new");

    if ( M_ssl_ctx == NULL )
    {
        ERR("SSL_CTX_new failed");
        return FALSE;
    }

//    const long flags = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
    const long flags = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;

    SSL_CTX_set_options(M_ssl_ctx, flags);

    /* temporarily ignore server cert errors */

    WAR("Ignoring remote server cert errors for HTTP calls");
//    SSL_CTX_set_verify(M_ssl_ctx, SSL_VERIFY_NONE, NULL);

    return TRUE;
}
#endif  /* NPP_HTTPS */


/* --------------------------------------------------------------------------
   Set socket as non-blocking
-------------------------------------------------------------------------- */
void npp_lib_setnonblocking(int sock)
{
#ifdef _WIN32   /* Windows */

    u_long mode = 1;  // 1 to enable non-blocking socket
    ioctlsocket(sock, FIONBIO, &mode);

#else   /* Linux / UNIX */

    int opts;

    opts = fcntl(sock, F_GETFL);

    if ( opts < 0 )
    {
        ERR("fcntl(F_GETFL) failed");
        return;
    }

    opts = (opts | O_NONBLOCK);

    if ( fcntl(sock, F_SETFL, opts) < 0 )
    {
        ERR("fcntl(F_SETFL) failed");
        return;
    }
#endif
}


#ifndef NPP_CLIENT
/* --------------------------------------------------------------------------
   Output standard HTML header
-------------------------------------------------------------------------- */
void npp_out_html_header(int ci)
{
    OUT("<!DOCTYPE html>");
    OUT("<html>");
    OUT("<head>");
    OUT("<title>%s</title>", NPP_APP_NAME);
#ifdef NPP_APP_DESCRIPTION
    OUT("<meta name=\"description\" content=\"%s\">", NPP_APP_DESCRIPTION);
#endif
#ifdef NPP_APP_KEYWORDS
    OUT("<meta name=\"keywords\" content=\"%s\">", NPP_APP_KEYWORDS);
#endif
    if ( REQ_MOB )  // if mobile request
        OUT("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
    OUT("</head>");
    OUT("<body>");
}


/* --------------------------------------------------------------------------
   Output standard HTML footer
-------------------------------------------------------------------------- */
void npp_out_html_footer(int ci)
{
    OUT("</body>");
    OUT("</html>");
}


/* --------------------------------------------------------------------------
   Find first free slot in G_snippets
-------------------------------------------------------------------------- */
static int first_free_snippet()
{
    int i=0;

    for ( i=0; i<NPP_MAX_SNIPPETS; ++i )
    {
        if ( G_snippets[i].name[0]=='-' || G_snippets[i].name[0]==EOS )
        {
            if ( i > G_snippets_cnt ) G_snippets_cnt = i;
            return i;
        }
    }

    ERR("NPP_MAX_SNIPPETS reached (%d)! You can set/increase NPP_MAX_SNIPPETS in npp_app.h.", NPP_MAX_SNIPPETS);

    return -1;   /* nothing's free, we ran out of snippets! */
}


/* --------------------------------------------------------------------------
   Read snippets from disk
   Unlike res or resmin, snippets need to be available
   in both npp_app and npp_svc processes
-------------------------------------------------------------------------- */
bool npp_lib_read_snippets(const char *host, const char *directory, bool first_scan, const char *path)
{
    int     i;
    char    resdir[NPP_STATIC_PATH_LEN+1];      /* full path to res */
    char    ressubdir[NPP_STATIC_PATH_LEN*2+2]; /* full path to res/subdir */
    char    namewpath[NPP_STATIC_PATH_LEN*2+2]; /* full path including file name */
    char    resname[NPP_STATIC_PATH_LEN+1];     /* relative path including file name */
    DIR     *dir;
    struct dirent *dirent;
    FILE    *fd;
    struct stat fstat;

#ifndef _WIN32
    if ( G_appdir[0] == EOS ) return TRUE;
#endif

    if ( first_scan && !path ) DBG("");

#ifdef NPP_DEBUG
    if ( first_scan )
    {
        if ( !path ) DBG_LINE_LONG;
        DBG("npp_lib_read_snippets");
    }
#endif  /* NPP_DEBUG */

#ifdef _WIN32   /* be more forgiving */

    if ( G_appdir[0] )
    {
        sprintf(resdir, "%s\\snippets", G_appdir);
    }
    else    /* no NPP_DIR */
    {
        sprintf(resdir, "..\\snippets");
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
        return TRUE;    /* don't panic, just no snippets will be used */
    }

    /* ------------------------------------------------------------------- */
    /* check removed files */

    if ( !first_scan && !path )   /* on the highest level only */
    {
//        DDBG("Checking removed files...");

        for ( i=0; i<=G_snippets_cnt; ++i )
        {
            if ( G_snippets[i].name[0]==EOS ) continue;   /* already removed */

            if ( 0 != strcmp(G_snippets[i].host, host) ) continue;

//            DDBG("Checking %s...", G_snippets[i].name);

            char fullpath[NPP_STATIC_PATH_LEN*2];
#ifdef _WIN32
            sprintf(fullpath, "%s\\%s", resdir, G_snippets[i].name);
#else
            sprintf(fullpath, "%s/%s", resdir, G_snippets[i].name);
#endif
            if ( !npp_file_exists(fullpath) )
            {
                INF("Removing %s from snippets", G_snippets[i].name);

                G_snippets[i].host[0] = EOS;
                G_snippets[i].name[0] = EOS;

                free(G_snippets[i].data);
                G_snippets[i].data = NULL;
                G_snippets[i].len = 0;
            }
        }
    }

    /* ------------------------------------------------------------------- */

//    DDBG("Reading %sfiles", first_scan?"":"new ");

    /* read the files into the memory */

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
            npp_lib_read_snippets(host, directory, first_scan, resname);
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

            for ( i=0; i<=G_snippets_cnt; ++i )
            {
                if ( G_snippets[i].name[0]==EOS ) continue;   /* removed */

                /* ------------------------------------------------------------------- */

                if ( 0==strcmp(G_snippets[i].host, host) && 0==strcmp(G_snippets[i].name, resname) )
                {
//                    DDBG("%s already read", resname);

                    if ( G_snippets[i].modified == fstat.st_mtime )
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

        /* find the first unused slot in G_snippets array */

        if ( !reread )
        {
            i = first_free_snippet();

            /* host -- already uppercase */

            strcpy(G_snippets[i].host, host);

            /* file name */

            strcpy(G_snippets[i].name, resname);
        }

        /* last modified */

        G_snippets[i].modified = fstat.st_mtime;

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
            G_snippets[i].len = ftell(fd);
            rewind(fd);

            /* allocate the final destination */

            if ( reread )
            {
                free(G_snippets[i].data);
                G_snippets[i].data = NULL;
            }

            G_snippets[i].data = (char*)malloc(G_snippets[i].len+1);

            if ( NULL == G_snippets[i].data )
            {
                ERR("Couldn't allocate %u bytes for %s", G_snippets[i].len+1, G_snippets[i].name);
                fclose(fd);
                closedir(dir);
                return FALSE;
            }

            if ( fread(G_snippets[i].data, G_snippets[i].len, 1, fd) != 1 )
            {
                ERR("Couldn't read from %s", G_snippets[i].name);
                fclose(fd);
                closedir(dir);
                return FALSE;
            }

            fclose(fd);

            *(G_snippets[i].data+G_snippets[i].len) = EOS;

            /* log file info ----------------------------------- */

            if ( G_logLevel > LOG_INF )
            {
                G_ptm = gmtime(&G_snippets[i].modified);
                char mod_time[128];
                sprintf(mod_time, "%d-%02d-%02d %02d:%02d:%02d", G_ptm->tm_year+1900, G_ptm->tm_mon+1, G_ptm->tm_mday, G_ptm->tm_hour, G_ptm->tm_min, G_ptm->tm_sec);
                G_ptm = gmtime(&G_now);     /* set it back */
                DBG("%s %s\t\t%u bytes", npp_add_spaces(G_snippets[i].name, 28), mod_time, G_snippets[i].len);
            }
        }
    }

    closedir(dir);

    if ( first_scan && !path ) DBG("");

    return TRUE;
}


/* --------------------------------------------------------------------------
   Get snippet index
-------------------------------------------------------------------------- */
static int get_snippet_idx(int ci, const char *name)
{
    int i;

#ifdef NPP_MULTI_HOST

    if ( !G_connections[ci].host_id )    /* main host */
    {
        for ( i=0; G_snippets[i].name[0] != '-'; ++i )
        {
            if ( !G_snippets[i].host[0] && 0==strcmp(G_snippets[i].name, name) )
                return i;
        }
    }
    else    /* side gig */
    {
        for ( i=0; G_snippets[i].name[0] != '-'; ++i )
        {
            if ( 0==strcmp(G_snippets[i].host, G_connections[ci].host_normalized) && 0==strcmp(G_snippets[i].name, name) )
                return i;
        }
    }

#else   /* NOT NPP_MULTI_HOST */

    for ( i=0; G_snippets[i].name[0] != '-'; ++i )
    {
        if ( 0==strcmp(G_snippets[i].name, name) )
            return i;
    }

#endif  /* NPP_MULTI_HOST */

    return -1;
}


/* --------------------------------------------------------------------------
   Get snippet
-------------------------------------------------------------------------- */
char *npp_get_snippet(int ci, const char *name)
{
    int i = get_snippet_idx(ci, name);

    if ( i != -1 )
        return G_snippets[i].data;

    return NULL;
}


/* --------------------------------------------------------------------------
   Get snippet length
-------------------------------------------------------------------------- */
unsigned npp_get_snippet_len(int ci, const char *name)
{
    int i = get_snippet_idx(ci, name);

    if ( i != -1 )
        return G_snippets[i].len;

    return 0;
}


/* --------------------------------------------------------------------------
   OUT snippet
-------------------------------------------------------------------------- */
void npp_out_snippet(int ci, const char *name)
{
    int i = get_snippet_idx(ci, name);

    if ( i != -1 )
        OUT_BIN(G_snippets[i].data, G_snippets[i].len);
}


/* --------------------------------------------------------------------------
   OUT markdown snippet
-------------------------------------------------------------------------- */
void npp_out_snippet_md(int ci, const char *name)
{
    int i = get_snippet_idx(ci, name);

    if ( i != -1 )
    {
        npp_render_md(G_tmp, G_snippets[i].data, NPP_TMP_STR_LEN);
        OUT(G_tmp);
    }
}


/* --------------------------------------------------------------------------
   Add CSS link to HTML head
-------------------------------------------------------------------------- */
void npp_append_css(int ci, const char *fname, bool first)
{
    if ( first )
    {
        DBG("first = TRUE; Defining ldlink()");
        OUT("function ldlink(n){var f=document.createElement('link');f.setAttribute(\"rel\",\"stylesheet\");f.setAttribute(\"type\",\"text/css\");f.setAttribute(\"href\",n);document.getElementsByTagName(\"head\")[0].appendChild(f);}");
    }
    OUT("ldlink('%s');", fname);
}


/* --------------------------------------------------------------------------
   Add script to HTML head
-------------------------------------------------------------------------- */
void npp_append_script(int ci, const char *fname, bool first)
{
    if ( first )
    {
        DBG("first = TRUE; Defining ldscript()");
        OUT("function ldscript(n){var f=document.createElement('script');f.setAttribute(\"type\",\"text/javascript\");f.setAttribute(\"src\",n);document.getElementsByTagName(\"head\")[0].appendChild(f);}");
    }
    OUT("ldscript('%s');", fname);
}


/* --------------------------------------------------------------------------
   URI-decode character
-------------------------------------------------------------------------- */
static int xctod(int c)
{
    if ( isdigit(c) )
        return c - '0';
    else if ( isupper(c) )
        return c - 'A' + 10;
    else if ( islower(c) )
        return c - 'a' + 10;
    else
        return 0;
}


/* --------------------------------------------------------------------------
   URI-decode src
-------------------------------------------------------------------------- */
static char *uri_decode(char *src, int srclen, char *dest, int maxlen)
{
    char *endp=src+srclen;
    char *srcp;
    char *destp=dest;
    int  written=0;

    for ( srcp=src; srcp<endp; ++srcp )
    {
        if ( *srcp == '+' )
            *destp++ = ' ';
        else if ( *srcp == '%' )
        {
            *destp++ = 16 * xctod(*(srcp+1)) + xctod(*(srcp+2));
            srcp += 2;
        }
        else
            *destp++ = *srcp;

        ++written;

        if ( written == maxlen )
        {
            WAR("URI val truncated");
            break;
        }
    }

    *destp = EOS;

    G_qs_len = written;

    return dest;
}


/* --------------------------------------------------------------------------
   Add a host and assign resource directories
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
bool npp_add_host(const std::string& host_, const std::string& res_, const std::string& resmin_, const std::string& snippets_)
{
    const char *host = host_.c_str();
    const char *res = res_.c_str();
    const char *resmin = resmin_.c_str();
    const char *snippets = snippets_.c_str();
#else
bool npp_add_host(const char *host, const char *res, const char *resmin, const char *snippets)
{
#endif
#ifdef NPP_MULTI_HOST

    if ( G_hosts_cnt >= NPP_MAX_HOSTS ) return FALSE;

    COPY(G_hosts[G_hosts_cnt].host, npp_upper(host), NPP_MAX_HOST_LEN);

    if ( res && res[0] )
        COPY(G_hosts[G_hosts_cnt].res, res, 255);
    if ( resmin && resmin[0] )
        COPY(G_hosts[G_hosts_cnt].resmin, resmin, 255);
    if ( snippets && snippets[0] )
        COPY(G_hosts[G_hosts_cnt].snippets, snippets, 255);

    ++G_hosts_cnt;

#endif  /* NPP_MULTI_HOST */

    return TRUE;
}


/* --------------------------------------------------------------------------
   Get the incoming param if Content-Type == JSON
-------------------------------------------------------------------------- */
static bool get_qs_param_json(int ci, const char *fieldname, char *retbuf, int maxlen)
{
static int prev_ci=-1;
static unsigned prev_req;
static JSON req={0};

    /* parse JSON only once per request */

    if ( ci != prev_ci || G_cnts_today.req != prev_req )
    {
        if ( !REQ_DATA )
            return FALSE;

        if ( !lib_json_from_string(&req, REQ_DATA, 0, 0) )
            return FALSE;

        prev_ci = ci;
        prev_req = G_cnts_today.req;
    }

    if ( !lib_json_present(&req, fieldname) )
        return FALSE;

    strncpy(retbuf, lib_json_get_str(&req, fieldname, 0), maxlen);
    retbuf[maxlen] = EOS;

    return TRUE;
}


/* --------------------------------------------------------------------------
   Get text value from multipart-form-data
-------------------------------------------------------------------------- */
static bool get_qs_param_multipart_txt(int ci, const char *fieldname, char *retbuf, size_t maxlen)
{
    unsigned char *p;
    size_t len;

    p = npp_lib_get_qs_param_multipart(ci, fieldname, &len, NULL);

    if ( !p ) return FALSE;

    DDBG("len = %d", len);

    if ( len > maxlen )
    {
        len = maxlen;
        DDBG("len reduced to %d", len);
    }

    COPY(retbuf, (char*)p, len);

    return TRUE;
}


/* --------------------------------------------------------------------------
   Get the query string value. Return TRUE if found.
-------------------------------------------------------------------------- */
static bool get_qs_param_raw(int ci, const char *fieldname, char *retbuf, size_t maxlen)
{
    char *qs, *end;

    G_qs_len = 0;

    DDBG("get_qs_param_raw: fieldname [%s]", fieldname);

    if ( NPP_CONN_IS_PAYLOAD(G_connections[ci].flags) )
    {
        if ( G_connections[ci].in_ctype == NPP_CONTENT_TYPE_JSON )
        {
            return get_qs_param_json(ci, fieldname, retbuf, maxlen);
        }
        else if ( G_connections[ci].in_ctype == NPP_CONTENT_TYPE_MULTIPART )
        {
            return get_qs_param_multipart_txt(ci, fieldname, retbuf, maxlen);
        }
        else if ( G_connections[ci].in_ctype != NPP_CONTENT_TYPE_URLENCODED && G_connections[ci].in_ctype != NPP_CONTENT_TYPE_UNSET )
        {
            WAR("Invalid Content-Type");
            if ( retbuf ) retbuf[0] = EOS;
            return FALSE;
        }
        qs = G_connections[ci].in_data;
        end = qs + G_connections[ci].clen;
    }
    else    /* GET */
    {
        qs = strchr(G_connections[ci].uri, '?');
    }

    if ( qs == NULL )
    {
        if ( retbuf ) retbuf[0] = EOS;
        return FALSE;
    }

    if ( !NPP_CONN_IS_PAYLOAD(G_connections[ci].flags) )   /* GET */
    {
        ++qs;      /* skip the question mark */
        end = qs + (strlen(G_connections[ci].uri) - (qs-G_connections[ci].uri));
        DDBG("get_qs_param_raw: qs len = %d", strlen(G_connections[ci].uri) - (qs-G_connections[ci].uri));
    }

    int fnamelen = strlen(fieldname);

    if ( fnamelen < 1 )
    {
        if ( retbuf ) retbuf[0] = EOS;
        return FALSE;
    }

    char *val = qs;

    bool found=FALSE;

    while ( val < end )
    {
        val = strstr(val, fieldname);

        if ( val == NULL )
        {
            if ( retbuf ) retbuf[0] = EOS;
            return FALSE;
        }

        if ( val != qs && *(val-1) != '&' )
        {
            ++val;
            continue;
        }

        val += fnamelen;

        if ( *val == '=' )   /* found */
        {
            found = TRUE;
            break;
        }
    }

    if ( !found )
    {
        if ( retbuf ) retbuf[0] = EOS;
        return FALSE;
    }

    ++val;  /* skip '=' */

    /* copy the value */

    size_t i = 0;

    while ( *val && *val != '&' && i<maxlen )
        retbuf[i++] = *val++;

    retbuf[i] = EOS;

#ifdef NPP_DEBUG
    npp_log_long(retbuf, i, "get_qs_param_raw: retbuf");
#endif

    G_qs_len = i;

    return TRUE;
}


/* --------------------------------------------------------------------------
   Get incoming request data. TRUE if found.
-------------------------------------------------------------------------- */
bool npp_lib_get_qs_param(int ci, const char *fieldname, char *retbuf, size_t maxlen, char esc_type)
{
static char interbuf[65536];

    if ( G_connections[ci].in_ctype == NPP_CONTENT_TYPE_URLENCODED )
    {
static char rawbuf[196608];    /* URL-encoded can have up to 3 times bytes count */

        if ( !get_qs_param_raw(ci, fieldname, rawbuf, maxlen*3-1) )
        {
            if ( retbuf ) retbuf[0] = EOS;
            return FALSE;
        }

        if ( retbuf )
            uri_decode(rawbuf, G_qs_len, interbuf, maxlen);
    }
    else    /* usually JSON or multipart */
    {
        if ( !get_qs_param_raw(ci, fieldname, interbuf, maxlen) )
        {
            if ( retbuf ) retbuf[0] = EOS;
            return FALSE;
        }
    }

    /* now we have URI-decoded string in interbuf */

    if ( retbuf )
    {
        if ( esc_type == NPP_ESC_HTML )
            npp_lib_escape_for_html(retbuf, interbuf, maxlen);
        else if ( esc_type == NPP_ESC_SQL )
            npp_lib_escape_for_sql(retbuf, interbuf, maxlen);
        else
        {
            strncpy(retbuf, interbuf, maxlen);
            retbuf[maxlen] = EOS;
        }
    }

    return TRUE;
}


/* --------------------------------------------------------------------------
   multipart-form-data receipt
   Return length or -1 if error
   If retfname is not NULL then assume binary data and it must be the last
   data element
-------------------------------------------------------------------------- */
unsigned char *npp_lib_get_qs_param_multipart(int ci, const char *fieldname, size_t *retlen, char *retfname)
{
    unsigned blen;           /* boundary length */
    char     *cp;            /* current pointer */
    char     *p;             /* tmp pointer */
    unsigned b;              /* tmp bytes count */
    char     fn[NPP_MAX_LABEL_LEN+1];    /* field name */
    char     *end;
    size_t   len;

    /* Couple of checks to make sure it's properly formatted multipart content */

    if ( G_connections[ci].in_ctype != NPP_CONTENT_TYPE_MULTIPART )
    {
        WAR("This is not multipart/form-data");
        return NULL;
    }

    if ( !G_connections[ci].in_data )
        return NULL;

    if ( G_connections[ci].clen < 10 )
    {
        WAR("Content length seems to be too small for multipart (%u)", G_connections[ci].clen);
        return NULL;
    }

    cp = G_connections[ci].in_data;

    if ( !G_connections[ci].boundary[0] )    /* find first end of line -- that would be end of boundary */
    {
        if ( NULL == (p=strchr(cp, '\n')) )
        {
            WAR("Request syntax error");
            return NULL;
        }

        b = p - cp - 2;     /* skip -- */

        if ( b < 2 )
        {
            WAR("Boundary appears to be too short (%u)", b);
            return NULL;
        }
        else if ( b > NPP_MAX_BOUNDARY_LEN )
        {
            WAR("Boundary appears to be too long (%u)", b);
            return NULL;
        }

        strncpy(G_connections[ci].boundary, cp+2, b);
        if ( G_connections[ci].boundary[b-1] == '\r' )
            G_connections[ci].boundary[b-1] = EOS;
        else
            G_connections[ci].boundary[b] = EOS;
    }

    blen = strlen(G_connections[ci].boundary);

    if ( G_connections[ci].in_data[G_connections[ci].clen-4] != '-' || G_connections[ci].in_data[G_connections[ci].clen-3] != '-' )
    {
        WAR("Content doesn't end with '--'");
        return NULL;
    }

    while (TRUE)    /* find the right section */
    {
        if ( NULL == (p=strstr(cp, G_connections[ci].boundary)) )
        {
            WAR("No (next) boundary found");
            return NULL;
        }

        b = p - cp + blen;
        cp += b;

        if ( NULL == (p=strstr(cp, "Content-Disposition: form-data;")) )
        {
            WAR("No Content-Disposition label");
            return NULL;
        }

        b = p - cp + 30;
        cp += b;

        if ( NULL == (p=strstr(cp, "name=\"")) )
        {
            WAR("No field name");
            return NULL;
        }

        b = p - cp + 6;
        cp += b;

//      DBG("field name starts from: [%s]", cp);

        if ( NULL == (p=strchr(cp, '"')) )
        {
            WAR("No field name closing quote");
            return NULL;
        }

        b = p - cp;

        if ( b > NPP_MAX_LABEL_LEN )
        {
            WAR("Field name too long (%u)", b);
            return NULL;
        }

        strncpy(fn, cp, b);
        fn[b] = EOS;

//      DBG("fn: [%s]", fn);

        if ( 0==strcmp(fn, fieldname) )     /* found */
            break;

        cp += b;
    }

    /* find a file name */

    if ( retfname )
    {
        if ( NULL == (p=strstr(cp, "filename=\"")) )
        {
            WAR("No file name");
            return NULL;
        }

        b = p - cp + 10;
        cp += b;

    //  DBG("file name starts from: [%s]", cp);

        if ( NULL == (p=strchr(cp, '"')) )
        {
            WAR("No file name closing quote");
            return NULL;
        }

        b = p - cp;

        if ( b > MAX_URI_VAL_LEN )
        {
            WAR("File name too long (%u)", b);
            return NULL;
        }

        strncpy(fn, cp, b);
        fn[b] = EOS;        /* fn now contains file name */

        cp += b;
    }

    /* now look for the section header end where the actual data begins */

    if ( NULL == (p=strstr(cp, "\r\n\r\n")) )
    {
        WAR("No section header end");
        return NULL;
    }

    b = p - cp + 4;
    cp += b;        /* cp now points to the actual data */

    /* find out data length */

    if ( !retfname )    /* text */
    {
        if ( NULL == (end=strstr(cp, G_connections[ci].boundary)) )
        {
            WAR("No closing boundary found");
            return NULL;
        }

        len = end - cp - 4;     /* minus CRLF-- */
    }
    else    /* potentially binary content -- calculate rather than use strstr */
    {
        len = G_connections[ci].clen - (cp - G_connections[ci].in_data) - blen - 8;  /* fast version */
                                                                /* Note that the file content must come as last! */
    }

    /* everything looks good so far */

    *retlen = len;

    if ( retfname )
        strcpy(retfname, fn);

    return (unsigned char*)cp;
}


/* --------------------------------------------------------------------------
   Get integer value from the query string
-------------------------------------------------------------------------- */
bool npp_lib_qsi(int ci, const char *fieldname, int *retbuf)
{
    QSVAL s;

    if ( get_qs_param_raw(ci, fieldname, s, MAX_URI_VAL_LEN) )
    {
        if ( retbuf )
            sscanf(s, "%d", retbuf);

        return TRUE;
    }

    return FALSE;
}


/* --------------------------------------------------------------------------
   Get unsigned value from the query string
-------------------------------------------------------------------------- */
bool npp_lib_qsu(int ci, const char *fieldname, unsigned *retbuf)
{
    QSVAL s;

    if ( get_qs_param_raw(ci, fieldname, s, MAX_URI_VAL_LEN) )
    {
        if ( retbuf )
            sscanf(s, "%u", retbuf);

        return TRUE;
    }

    return FALSE;
}


/* --------------------------------------------------------------------------
   Get long value from the query string
-------------------------------------------------------------------------- */
bool npp_lib_qsl(int ci, const char *fieldname, long *retbuf)
{
    QSVAL s;

    if ( get_qs_param_raw(ci, fieldname, s, MAX_URI_VAL_LEN) )
    {
        if ( retbuf )
            sscanf(s, "%ld", retbuf);

        return TRUE;
    }

    return FALSE;
}


/* --------------------------------------------------------------------------
   Get float value from the query string
-------------------------------------------------------------------------- */
bool npp_lib_qsf(int ci, const char *fieldname, float *retbuf)
{
    QSVAL s;

    if ( get_qs_param_raw(ci, fieldname, s, MAX_URI_VAL_LEN) )
    {
        if ( retbuf )
            sscanf(s, "%f", retbuf);

        return TRUE;
    }

    return FALSE;
}


/* --------------------------------------------------------------------------
   Get double value from the query string
-------------------------------------------------------------------------- */
bool npp_lib_qsd(int ci, const char *fieldname, double *retbuf)
{
    QSVAL s;

    if ( get_qs_param_raw(ci, fieldname, s, MAX_URI_VAL_LEN) )
    {
        if ( retbuf )
            sscanf(s, "%lf", retbuf);

        return TRUE;
    }

    return FALSE;
}


/* --------------------------------------------------------------------------
   Get bool value from the query string
-------------------------------------------------------------------------- */
bool npp_lib_qsb(int ci, const char *fieldname, bool *retbuf)
{
    QSVAL s;

    if ( get_qs_param_raw(ci, fieldname, s, MAX_URI_VAL_LEN) )
    {
        if ( retbuf )
        {
            if ( NPP_IS_THIS_TRUE(s[0]) || 0==strcmp(s, "on") )
                *retbuf = TRUE;
            else
                *retbuf = FALSE;
        }

        return TRUE;
    }

    return FALSE;
}


/* --------------------------------------------------------------------------
   Set response status
-------------------------------------------------------------------------- */
void npp_lib_set_res_status(int ci, int status)
{
    G_connections[ci].status = status;
}


/* --------------------------------------------------------------------------
   Set custom header
-------------------------------------------------------------------------- */
bool npp_lib_res_header(int ci, const char *hdr, const char *val)
{
    int hlen = strlen(hdr);
    int vlen = strlen(val);
    int all = hlen + vlen + 4;

    if ( all > NPP_CUST_HDR_LEN - G_connections[ci].cust_headers_len )
    {
        WAR("Couldn't add %s to custom headers: no space", hdr);
        return FALSE;
    }

    strcat(G_connections[ci].cust_headers, hdr);
    strcat(G_connections[ci].cust_headers, ": ");
    strcat(G_connections[ci].cust_headers, val);
    strcat(G_connections[ci].cust_headers, "\r\n");

    G_connections[ci].cust_headers_len += all;

    return TRUE;
}


/* --------------------------------------------------------------------------
   Get request cookie
-------------------------------------------------------------------------- */
bool npp_lib_get_cookie(int ci, const char *key, char *value)
{
    char nkey[256];
    char *v;

    sprintf(nkey, "%s=", key);

    v = strstr(G_connections[ci].in_cookie, nkey);

    if ( !v ) return FALSE;

    /* key present */

    if ( value )
    {
        int j=0;

        while ( *v!='=' ) ++v;

        ++v;    /* skip '=' */

        while ( *v && *v!=';' )
            value[j++] = *(v++);

        value[j] = EOS;
    }

    return TRUE;
}


/* --------------------------------------------------------------------------
   Set cookie
-------------------------------------------------------------------------- */
bool npp_lib_set_cookie(int ci, const char *key, const char *value, int days)
{
    char v[NPP_CUST_HDR_LEN+1];

    if ( days )
        sprintf(v, "%s=%s; Expires=%s;", key, value, time_epoch2http(G_now + 3600*24*days));
    else    /* current session only */
        sprintf(v, "%s=%s", key, value);

    return npp_lib_res_header(ci, "Set-Cookie", v);
}


/* --------------------------------------------------------------------------
   Set response content type
   Mirrored print_content_type
-------------------------------------------------------------------------- */
void npp_lib_set_res_content_type(int ci, const char *str)
{
    if ( 0==strcmp(str, "text/html; charset=utf-8") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_HTML;
    else if ( 0==strcmp(str, "text/plain") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_TEXT;
    else if ( 0==strcmp(str, "text/css") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_CSS;
    else if ( 0==strcmp(str, "application/javascript") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_JS;
    else if ( 0==strcmp(str, "image/gif") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_GIF;
    else if ( 0==strcmp(str, "image/jpeg") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_JPG;
    else if ( 0==strcmp(str, "image/x-icon") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_ICO;
    else if ( 0==strcmp(str, "image/png") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_PNG;
    else if ( 0==strcmp(str, "image/bmp") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_BMP;
    else if ( 0==strcmp(str, "image/svg+xml") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_SVG;
    else if ( 0==strcmp(str, "application/json") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_JSON;
    else if ( 0==strcmp(str, "text/markdown") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_MD;
    else if ( 0==strcmp(str, "application/pdf") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_PDF;
    else if ( 0==strcmp(str, "application/xml") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_XML;
    else if ( 0==strcmp(str, "audio/mpeg") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_AMPEG;
    else if ( 0==strcmp(str, "application/x-msdownload") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_EXE;
    else if ( 0==strcmp(str, "application/zip") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_ZIP;
    else if ( 0==strcmp(str, "application/gzip") )
        G_connections[ci].out_ctype = NPP_CONTENT_TYPE_GZIP;
    else    /* custom */
    {
        if ( 0==strncmp(str, "text/html", 9) )
            G_connections[ci].out_ctype = NPP_CONTENT_TYPE_HTML;
        else if ( 0==strncmp(str, "text/plain", 10) )
            G_connections[ci].out_ctype = NPP_CONTENT_TYPE_TEXT;
        else if ( !str[0] )
            G_connections[ci].out_ctype = NPP_CONTENT_TYPE_UNSET;
        else
            G_connections[ci].out_ctype = NPP_CONTENT_TYPE_USER;

        COPY(G_connections[ci].ctypestr, str, NPP_CONTENT_TYPE_LEN);
    }
}


/* --------------------------------------------------------------------------
   Set location
-------------------------------------------------------------------------- */
void npp_lib_set_res_location(int ci, const char *str, ...)
{
    va_list plist;

    va_start(plist, str);
    vsprintf(G_connections[ci].location, str, plist);
    va_end(plist);
}


/* --------------------------------------------------------------------------
   Set response content disposition
-------------------------------------------------------------------------- */
void npp_lib_set_res_content_disposition(int ci, const char *str, ...)
{
    va_list plist;

    va_start(plist, str);
    vsprintf(G_connections[ci].cdisp, str, plist);
    va_end(plist);
}


/* --------------------------------------------------------------------------
   Send message description as plain, pipe-delimited text as follows:
   <code>|<category>|<description>
-------------------------------------------------------------------------- */
void npp_lib_send_msg_description(int ci, int code)
{
    char cat[64];
    char msg[1024];

    strcpy(cat, get_msg_cat(code));
    strcpy(msg, npp_message(code));

#ifdef NPP_MSG_DESCRIPTION_PIPES
    OUT("%d|%s|%s", code, cat, msg);
    RES_CONTENT_TYPE_TEXT;
#else   /* JSON */
    OUT("{\"code\":%d,\"category\":\"%s\",\"message\":\"%s\"}", code, cat, msg);
    RES_CONTENT_TYPE_JSON;
#endif  /* NPP_MSG_DESCRIPTION_PIPES */

    RES_KEEP_CONTENT;

    RES_DONT_CACHE;
}


/* --------------------------------------------------------------------------
   Format counters
-------------------------------------------------------------------------- */
static void format_counters(int ci, counters_fmt_t *s, npp_counters_t *n)
{
    DDBG("format_counters");

    strcpy(s->req, INT(n->req));
    strcpy(s->req_dsk, INT(n->req_dsk));
    strcpy(s->req_tab, INT(n->req_tab));
    strcpy(s->req_mob, INT(n->req_mob));
    strcpy(s->req_bot, INT(n->req_bot));
    strcpy(s->visits, INT(n->visits));
    strcpy(s->visits_dsk, INT(n->visits_dsk));
    strcpy(s->visits_tab, INT(n->visits_tab));
    strcpy(s->visits_mob, INT(n->visits_mob));
    strcpy(s->blocked, INT(n->blocked));
    strcpy(s->average, AMT(n->average));
}


#ifdef NPP_MYSQL
#ifdef NPP_USERS
/* --------------------------------------------------------------------------
   Users info
-------------------------------------------------------------------------- */
static void users_info(int ci, char activity, int rows, admin_info_t ai[], int ai_cnt)
{
    char        sql[NPP_SQLBUF];
    MYSQL_RES   *result;
    MYSQL_ROW   row;

    char ai_sql[NPP_SQLBUF]="";

    if ( ai && ai_cnt )
    {
        int i;
        for ( i=0; i<ai_cnt; ++i )
        {
            strcat(ai_sql, ", (");
            strcat(ai_sql, ai[i].sql);
            strcat(ai_sql, ")");
        }
    }

//    sprintf(sql, "SELECT id, login, email, name, status, created, last_login, visits%s FROM users ORDER BY last_login DESC, created DESC", ai_sql);
    sprintf(sql, "SELECT id, login, email, name, status, created, last_login, visits%s FROM users", ai_sql);

    char activity_desc[64];
    int days;

    if ( activity == AI_USERS_YAU )
    {
        strcpy(activity_desc, "yearly active");
        days = 366;
    }
    else if ( activity == AI_USERS_MAU )
    {
        strcpy(activity_desc, "monthly active");
        days = 31;
    }
    else if ( activity == AI_USERS_DAU )
    {
        strcpy(activity_desc, "daily active");
        days = 2;
    }
    else    /* all */
    {
        strcpy(activity_desc, "all");
        days = 0;
    }

    char tmp[256];

    if ( days==366 || days==31 )
    {
        sprintf(tmp, " WHERE status=%d AND visits>0 AND DATEDIFF('%s', last_login)<%d", USER_STATUS_ACTIVE, DT_NOW_GMT, days);
        strcat(sql, tmp);
    }
    else if ( days==2 )   /* last 24 hours */
    {
        sprintf(tmp, " WHERE status=%d AND visits>0 AND TIME_TO_SEC(TIMEDIFF('%s', last_login))<86401", USER_STATUS_ACTIVE, DT_NOW_GMT);
        strcat(sql, tmp);
    }

    strcat(sql, " ORDER BY last_login DESC, created DESC");

    DBG("sql: %s", sql);

    mysql_query(G_dbconn, sql);

    result = mysql_store_result(G_dbconn);

    if ( !result )
    {
        ERR("%u: %s", mysql_errno(G_dbconn), mysql_error(G_dbconn));
        OUT("<p>Error %u: %s</p>", mysql_errno(G_dbconn), mysql_error(G_dbconn));
        RES_STATUS(500);
        return;
    }

    int records = mysql_num_rows(result);

    INF("admin_info: %d %s user(s)", records, activity_desc);

    int last_to_show = records<rows?records:rows;

    char formatted1[64];
    char formatted2[64];

    strcpy(formatted1, INT(records));
    strcpy(formatted2, INT(last_to_show));
    OUT("<p>%s %s users, showing %s of last seen</p>", formatted1, activity_desc, formatted2);

    OUT("<table cellpadding=4 border=1 style=\"margin-bottom:3em;\">");

    char ai_th[1024]="";

    if ( ai && ai_cnt )
    {
        int i;
        for ( i=0; i<ai_cnt; ++i )
        {
            strcat(ai_th, "<th>");
            strcat(ai_th, ai[i].th);
            strcat(ai_th, "</th>");
        }
    }

    OUT("<tr>");

    if ( REQ_DSK )
        OUT("<th>id</th><th>login</th><th>email</th><th>name</th><th>created</th><th>last_login</th><th>visits</th>%s", ai_th);
    else
        OUT("<th>id</th><th>login</th><th>email</th><th>last_login</th><th>visits</th>%s", ai_th);

    OUT("</tr>");

//    int     id;                     /* row[0] */
//    char    login[NPP_LOGIN_LEN+1];     /* row[1] */
//    char    email[NPP_EMAIL_LEN+1];     /* row[2] */
//    char    name[NPP_UNAME_LEN+1];      /* row[3] */
//    char    status;                 /* row[4] */
//    char    created[32];            /* row[5] */
//    char    last_login[32];         /* row[6] */
//    int     visits;                 /* row[7] */

    char fmt0[64];  /* id */
    char fmt7[64];  /* visits */

    int  i;
    char trstyle[16];

    char ai_td[NPP_LIB_STR_BUF]="";
    double ai_double;

    for ( i=0; i<last_to_show; ++i )
    {
        row = mysql_fetch_row(result);

        strcpy(fmt0, INT(atoi(row[0])));    /* id */
        strcpy(fmt7, INT(atoi(row[7])));    /* visits */

        if ( atoi(row[4]) != USER_STATUS_ACTIVE )
            strcpy(trstyle, " class=g");
        else
            trstyle[0] = EOS;

        OUT("<tr%s>", trstyle);

        if ( ai && ai_cnt )
        {
            ai_td[0] = EOS;

            int j;
            for ( j=0; j<ai_cnt; ++j )
            {
                if ( 0==strcmp(ai[j].type, "int") )
                {
                    strcat(ai_td, "<td class=r>");
                    strcat(ai_td, INT(atoi(row[j+8])));
                }
                else if ( 0==strcmp(ai[j].type, "long") )
                {
                    strcat(ai_td, "<td class=r>");
                    strcat(ai_td, INT(atol(row[j+8])));
                }
                else if ( 0==strcmp(ai[j].type, "float") || 0==strcmp(ai[j].type, "double") )
                {
                    strcat(ai_td, "<td class=r>");
                    sscanf(row[j+8], "%lf", &ai_double);
                    strcat(ai_td, AMT(ai_double));
                }
                else    /* string */
                {
                    strcat(ai_td, "<td>");
                    strcat(ai_td, row[j+8]);
                }

                strcat(ai_td, "</td>");
            }
        }

        if ( REQ_DSK )
            OUT("<td class=r>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td class=r>%s</td>%s", fmt0, row[1], row[2], row[3], row[5], row[6], fmt7, ai_td);
        else
            OUT("<td class=r>%s</td><td>%s</td><td>%s</td><td>%s</td><td class=r>%s</td>%s", fmt0, row[1], row[2], row[6], fmt7, ai_td);

        OUT("</tr>");
    }

    OUT("</table>");

    mysql_free_result(result);
}
#endif  /* NPP_USERS */
#endif  /* NPP_MYSQL */


/* --------------------------------------------------------------------------
   Admin dashboard
-------------------------------------------------------------------------- */
void npp_admin_info(int ci, int users, admin_info_t ai[], int ai_cnt, bool header_n_footer)
{
#ifdef NPP_USERS
    if ( SESSION.auth_level < AUTH_LEVEL_ADMIN )
    {
        ERR("npp_admin_info: user authorization level < %d", AUTH_LEVEL_ADMIN);
        RES_STATUS(404);
        return;
    }
#endif  /* NPP_USERS */

    /* ------------------------------------------------------------------- */

    INF("admin_info: --------------------");
    INF("admin_info: %s", DT_NOW_GMT);

    if ( header_n_footer )
    {
        OUT_HTML_HEADER;

        OUT("<style>");
        OUT("body{font-family:monospace;font-size:10pt;}");
        OUT(".r{text-align:right;}");
        OUT(".g{color:grey;}");
        OUT("</style>");
    }
    else
    {
        OUT("<style>");
        OUT(".r{text-align:right;}");
        OUT(".g{color:grey;}");
        OUT("</style>");
    }

    OUT("<h1>Admin Info</h1>");

    OUT("<h2>Server</h2>");

    /* ------------------------------------------------------------------- */
    /* Server info */

    OUT("<p>Server started on %s (%s day(s) up) Node++ %s</p>", G_last_modified, INT(G_days_up), NPP_VERSION);

    /* ------------------------------------------------------------------- */
    /* Memory */

    OUT("<h2>Memory</h2>");

    int  mem_used;
    char mem_used_kib[64];
    char mem_used_mib[64];
    char mem_used_gib[64];

    mem_used = npp_get_memory();

    strcpy(mem_used_kib, INT(mem_used));
    strcpy(mem_used_mib, AMT((double)mem_used/1024));
    strcpy(mem_used_gib, AMT((double)mem_used/1024/1024));

    OUT("<p>HWM: %s KiB (%s MiB / %s GiB)</p>", mem_used_kib, mem_used_mib, mem_used_gib);

    OUT("<h2>Counters</h2>");

    OUT("<p>%d open connection(s), HWM: %d</p>", G_connections_cnt, G_connections_hwm);

    OUT("<p>%d session(s), HWM: %d</p>", G_sessions_cnt, G_sessions_hwm);

    /* ------------------------------------------------------------------- */
    /* Counters */

    counters_fmt_t t;       /* today */
    counters_fmt_t y;       /* yesterday */
    counters_fmt_t b;       /* the day before */

    format_counters(ci, &t, &G_cnts_today);
    format_counters(ci, &y, &G_cnts_yesterday);
    if ( REQ_DSK )
        format_counters(ci, &b, &G_cnts_day_before);

    OUT("<table cellpadding=4 border=1>");

    if ( REQ_DSK )  /* desktop -- 3 days' stats */
    {
        OUT("<tr><th>counter</th><th colspan=4>the day before</th><th colspan=4>yesterday</th><th colspan=4>today</th></tr>");
        OUT("<tr><td rowspan=2>all traffic (parsed requests)</td><td>all</td><td>dsk</td><td>tab</td><td>mob</td><td>all</td><td>dsk</td><td>tab</td><td>mob</td><td>all</td><td>dsk</td><td>tab</td><td>mob</td></tr>");
        OUT("<tr><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td></tr>", b.req, b.req_dsk, b.req_tab, b.req_mob, y.req, y.req_dsk, y.req_tab, y.req_mob, t.req, t.req_dsk, t.req_tab, t.req_mob);
        OUT("<tr><td>bots</td><td colspan=4 class=r>%s</td><td colspan=4 class=r>%s</td><td colspan=4 class=r>%s</td></tr>", b.req_bot, y.req_bot, t.req_bot);
        OUT("<tr><td rowspan=2>visits</td><td>all</td><td>dsk</td><td>tab</td><td>mob</td><td>all</td><td>dsk</td><td>tab</td><td>mob</td><td>all</td><td>dsk</td><td>tab</td><td>mob</td></tr>");
        OUT("<tr><td class=r><b>%s</b></td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r><b>%s</b></td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r><b>%s</b></td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td></tr>", b.visits, b.visits_dsk, b.visits_tab, b.visits_mob, y.visits, y.visits_dsk, y.visits_tab, y.visits_mob, t.visits, t.visits_dsk, t.visits_tab, t.visits_mob);
        OUT("<tr><td>attempts blocked</td><td colspan=4 class=r>%s</td><td colspan=4 class=r>%s</td><td colspan=4 class=r>%s</td></tr>", b.blocked, y.blocked, t.blocked);
        OUT("<tr><td>average</td><td colspan=4 class=r>%s ms</td><td colspan=4 class=r>%s ms</td><td colspan=4 class=r>%s ms</td></tr>", b.average, y.average, t.average);
    }
    else    /* mobile -- 2 days' stats */
    {
        OUT("<tr><th>counter</th><th colspan=4>yesterday</th><th colspan=4>today</th></tr>");
        OUT("<tr><td rowspan=2>all traffic (parsed requests)</td><td>all</td><td>dsk</td><td>tab</td><td>mob</td><td>all</td><td>dsk</td><td>tab</td><td>mob</td></tr>");
        OUT("<tr><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td></tr>", y.req, y.req_dsk, y.req_tab, y.req_mob, t.req, t.req_dsk, t.req_tab, t.req_mob);
        OUT("<tr><td>bots</td><td colspan=4 class=r>%s</td><td colspan=4 class=r>%s</td></tr>", y.req_bot, t.req_bot);
        OUT("<tr><td rowspan=2>visits</td><td>all</td><td>dsk</td><td>tab</td><td>mob</td><td>all</td><td>dsk</td><td>tab</td><td>mob</td></tr>");
        OUT("<tr><td class=r><b>%s</b></td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td><td class=r><b>%s</b></td><td class=r>%s</td><td class=r>%s</td><td class=r>%s</td></tr>", y.visits, y.visits_dsk, y.visits_tab, y.visits_mob, t.visits, t.visits_dsk, t.visits_tab, t.visits_mob);
        OUT("<tr><td>attempts blocked</td><td colspan=4 class=r>%s</td><td colspan=4 class=r>%s</td></tr>", y.blocked, t.blocked);
        OUT("<tr><td>average</td><td colspan=4 class=r>%s ms</td><td colspan=4 class=r>%s ms</td></tr>", y.average, t.average);
    }

    OUT("</table>");

    /* ------------------------------------------------------------------- */
    /* IP blacklist */

    if ( G_blacklist_cnt )
    {
        OUT("<h2>Blacklist</h2>");

        OUT("<p>%s blacklisted IPs</p>", INT(G_blacklist_cnt));
//        OUT("<p><form action=\"add2blocked\" method=\"post\"><input type=\"text\" name=\"ip\"> <input type=\"submit\" onClick=\"wait();\" value=\"Block\"></form></p>");
    }

    /* ------------------------------------------------------------------- */
    /* Logs */

//    OUT("<p><a href=\"logs\">Logs</a></p>");

    /* ------------------------------------------------------------------- */
    /* Users */
#ifdef NPP_USERS
    if ( users > 0 )
    {
        OUT("<h2>Users</h2>");
        users_info(ci, AI_USERS_ALL, users, ai, ai_cnt);
        users_info(ci, AI_USERS_YAU, users, ai, ai_cnt);
        users_info(ci, AI_USERS_MAU, users, ai, ai_cnt);
        users_info(ci, AI_USERS_DAU, users, ai, ai_cnt);
    }
#endif  /* NPP_USERS */

    if ( header_n_footer )
        OUT_HTML_FOOTER;

    RES_DONT_CACHE;
}
#endif  /* NPP_CLIENT */


/* --------------------------------------------------------------------------
   HTTP call -- reset request headers
-------------------------------------------------------------------------- */
void npp_call_http_headers_reset()
{
    M_call_http_headers_cnt = 0;
}


/* --------------------------------------------------------------------------
   HTTP call -- set request header value
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
void npp_call_http_header_set(const std::string& key_, const std::string& value_)
{
    const char *key = key_.c_str();
    const char *value = value_.c_str();
#else
void npp_call_http_header_set(const char *key, const char *value)
{
#endif
    int i;

    for ( i=0; i<M_call_http_headers_cnt; ++i )
    {
        if ( M_call_http_headers[i].key[0]==EOS )
        {
            strncpy(M_call_http_headers[M_call_http_headers_cnt].key, key, CALL_HTTP_HEADER_KEY_LEN);
            M_call_http_headers[M_call_http_headers_cnt].key[CALL_HTTP_HEADER_KEY_LEN] = EOS;
            strncpy(M_call_http_headers[i].value, value, CALL_HTTP_HEADER_VAL_LEN);
            M_call_http_headers[i].value[CALL_HTTP_HEADER_VAL_LEN] = EOS;
            return;
        }
        else if ( 0==strcmp(M_call_http_headers[i].key, key) )
        {
            strncpy(M_call_http_headers[i].value, value, CALL_HTTP_HEADER_VAL_LEN);
            M_call_http_headers[i].value[CALL_HTTP_HEADER_VAL_LEN] = EOS;
            return;
        }
    }

    if ( M_call_http_headers_cnt >= CALL_HTTP_MAX_HEADERS ) return;

    strncpy(M_call_http_headers[M_call_http_headers_cnt].key, key, CALL_HTTP_HEADER_KEY_LEN);
    M_call_http_headers[M_call_http_headers_cnt].key[CALL_HTTP_HEADER_KEY_LEN] = EOS;
    strncpy(M_call_http_headers[M_call_http_headers_cnt].value, value, CALL_HTTP_HEADER_VAL_LEN);
    M_call_http_headers[M_call_http_headers_cnt].value[CALL_HTTP_HEADER_VAL_LEN] = EOS;
    ++M_call_http_headers_cnt;
}


/* --------------------------------------------------------------------------
   HTTP call -- unset request header value
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
void npp_call_http_header_unset(const std::string& key_)
{
    const char *key = key_.c_str();
#else
void npp_call_http_header_unset(const char *key)
{
#endif
    int i;

    for ( i=0; i<M_call_http_headers_cnt; ++i )
    {
        if ( 0==strcmp(M_call_http_headers[i].key, key) )
        {
            M_call_http_headers[i].key[0] = EOS;
            M_call_http_headers[i].value[0] = EOS;
            return;
        }
    }
}


/* --------------------------------------------------------------------------
   HTTP call / close connection
-------------------------------------------------------------------------- */
static void close_conn(int sock)
{
#ifdef _WIN32   /* Windows */
    closesocket(sock);
#else
    close(sock);
#endif  /* _WIN32 */
}


/* --------------------------------------------------------------------------
   HTTP call / parse URL
-------------------------------------------------------------------------- */
static bool call_http_parse_url(const char *url, char *host, char *port, char *uri, bool *secure)
{
    int len = strlen(url);

    if ( len < 1 )
    {
        ERR("url too short (1)");
        return FALSE;
    }

//    if ( url[len-1] == '/' ) endingslash = TRUE;

    if ( len > 6 && url[0]=='h' && url[1]=='t' && url[2]=='t' && url[3]=='p' && url[4]==':' )
    {
        url += 7;
        len -= 7;
        if ( len < 1 )
        {
            ERR("url too short (2)");
            return FALSE;
        }
    }
    else if ( len > 7 && url[0]=='h' && url[1]=='t' && url[2]=='t' && url[3]=='p' && url[4]=='s' && url[5]==':' )
    {
#ifdef NPP_HTTPS
        *secure = TRUE;

        url += 8;
        len -= 8;
        if ( len < 1 )
        {
            ERR("url too short (2)");
            return FALSE;
        }

        if ( !M_ssl_ctx && !init_ssl_client() )   /* first time */
        {
            ERR("init_ssl_client failed");
            return FALSE;
        }
#else
        ERR("HTTPS is not enabled");
        return FALSE;
#endif  /* NPP_HTTPS */
    }

    DDBG("url [%s]", url);

    char *colon, *slash;

    colon = strchr((char*)url, ':');
    slash = strchr((char*)url, '/');

    if ( colon )    /* port specified */
    {
        strncpy(host, url, colon-url);
        host[colon-url] = EOS;

        if ( slash )
        {
            strncpy(port, colon+1, slash-colon-1);
            port[slash-colon-1] = EOS;
            strcpy(uri, slash+1);
        }
        else    /* only host name & port */
        {
            strcpy(port, colon+1);
            uri[0] = EOS;
        }
    }
    else    /* no port specified */
    {
        if ( slash )
        {
            strncpy(host, url, slash-url);
            host[slash-url] = EOS;
            strcpy(uri, slash+1);
        }
        else    /* only host name */
        {
            strcpy(host, url);
            uri[0] = EOS;
        }
#ifdef NPP_HTTPS
        if ( *secure )
            strcpy(port, "443");
        else
#endif  /* NPP_HTTPS */
        strcpy(port, "80");
    }

#ifdef NPP_DEBUG
    DBG("secure=%d", *secure);
    DBG("host [%s]", host);
    DBG("port [%s]", port);
    DBG(" uri [%s]", uri);
#endif  /* NPP_DEBUG */

    return TRUE;
}


/* --------------------------------------------------------------------------
   HTTP call / return true if header is already present
-------------------------------------------------------------------------- */
static bool call_http_header_present(const char *key)
{
    int i;
    char uheader[NPP_MAX_LABEL_LEN+1];

    strcpy(uheader, npp_upper(key));

    for ( i=0; i<M_call_http_headers_cnt; ++i )
    {
        if ( 0==strcmp(npp_upper(M_call_http_headers[i].key), uheader) )
        {
            return TRUE;
        }
    }

    return FALSE;
}


/* --------------------------------------------------------------------------
   HTTP call / set proxy
-------------------------------------------------------------------------- */
void call_http_set_proxy(bool value)
{
    M_call_http_proxy = value;
}


/* --------------------------------------------------------------------------
   HTTP call / render request
-------------------------------------------------------------------------- */
static int call_http_render_req(char *buffer, const char *method, const char *host, const char *uri, const void *req, bool json, bool keep)
{
    char *p=buffer;     /* stpcpy is faster than strcat */

    /* header */

    p = stpcpy(p, method);

    if ( M_call_http_proxy )
        p = stpcpy(p, " ");
    else
        p = stpcpy(p, " /");

    p = stpcpy(p, uri);
    p = stpcpy(p, " HTTP/1.1\r\n");
    p = stpcpy(p, "Host: ");
    p = stpcpy(p, host);
    p = stpcpy(p, "\r\n");

    char jtmp[NPP_JSON_BUFSIZE];

    if ( 0 != strcmp(method, "GET") && req )
    {
        if ( json )     /* JSON -> string conversion */
        {
            if ( !call_http_header_present("Content-Type") )
                p = stpcpy(p, "Content-Type: application/json; charset=utf-8\r\n");

            strcpy(jtmp, lib_json_to_string((JSON*)req));
        }
        else
        {
            if ( !call_http_header_present("Content-Type") )
                p = stpcpy(p, "Content-Type: application/x-www-form-urlencoded; charset=utf-8\r\n");
        }
        char tmp[64];
        sprintf(tmp, "Content-Length: %ld\r\n", (long)strlen(json?jtmp:(char*)req));
        p = stpcpy(p, tmp);
    }

    if ( json && !call_http_header_present("Accept") )
        p = stpcpy(p, "Accept: application/json\r\n");

    int i;

    for ( i=0; i<M_call_http_headers_cnt; ++i )
    {
        if ( M_call_http_headers[i].key[0] )
        {
            p = stpcpy(p, M_call_http_headers[i].key);
            p = stpcpy(p, ": ");
            p = stpcpy(p, M_call_http_headers[i].value);
            p = stpcpy(p, "\r\n");
        }
    }

    if ( keep )
        p = stpcpy(p, "Connection: keep-alive\r\n");
    else
        p = stpcpy(p, "Connection: close\r\n");

#ifndef NPP_NO_IDENTITY
    if ( !call_http_header_present("User-Agent") )
#ifdef NPP_AS_BOT
        p = stpcpy(p, "User-Agent: Node++ Bot\r\n");
#else
        p = stpcpy(p, "User-Agent: Node++\r\n");
#endif
#endif  /* NPP_NO_IDENTITY */

    /* end of headers */

    p = stpcpy(p, "\r\n");

    /* body */

    if ( 0 != strcmp(method, "GET") && req )
        p = stpcpy(p, json?jtmp:(char*)req);

    *p = EOS;

    return p - buffer;
}


/* --------------------------------------------------------------------------
   Finish socket operation with timeout
-------------------------------------------------------------------------- */
static int lib_finish_with_timeout(int sock, char oper, char readwrite, char *buffer, int len, int *msec, const void *ssl, int level)
{
    int             sockerr;
    struct timeval  timeout;
    fd_set          readfds;
    fd_set          writefds;
    int             socks=0;
#ifdef NPP_HTTPS
    int             bytes;
    int             ssl_err;
#endif

#ifdef NPP_DEBUG
    if ( oper == NPP_OPER_READ )
        DBG("lib_finish_with_timeout NPP_OPER_READ, level=%d", level);
    else if ( oper == NPP_OPER_WRITE )
        DBG("lib_finish_with_timeout NPP_OPER_WRITE, level=%d", level);
    else if ( oper == NPP_OPER_CONNECT )
        DBG("lib_finish_with_timeout NPP_OPER_CONNECT, level=%d", level);
    else if ( oper == NPP_OPER_SHUTDOWN )
        DBG("lib_finish_with_timeout NPP_OPER_SHUTDOWN, level=%d", level);
    else
        ERR("lib_finish_with_timeout -- unknown operation: %d", oper);
#endif  /* NPP_DEBUG */

    if ( level > 20 )   /* just in case */
    {
        ERR("lib_finish_with_timeout -- too many levels");
        return -1;
    }

    /* get the error code ------------------------------------------------ */
    /* note: during SSL operations it will be 0                            */

#ifdef _WIN32   /* Windows */
    sockerr = WSAGetLastError();
#else
    sockerr = errno;
#endif

    if ( !ssl )
    {
#ifdef _WIN32   /* Windows */

//        sockerr = WSAGetLastError();

        if ( sockerr != WSAEWOULDBLOCK )
        {
            wchar_t *s = NULL;
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, sockerr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&s, 0, NULL);
            ERR("%d (%S)", sockerr, s);
            LocalFree(s);
            return -1;
        }

#else   /* Linux */

//        sockerr = errno;

        if ( sockerr != EWOULDBLOCK && sockerr != EINPROGRESS )
        {
            ERR("errno = %d (%s)", sockerr, strerror(sockerr));
            return -1;
        }

#endif  /* _WIN32 */
    }

    /* set up timeout for select ----------------------------------------- */

    if ( *msec < 1000 )
    {
        timeout.tv_sec = 0;
        timeout.tv_usec = *msec*1000;
    }
    else    /* 1000 ms or more */
    {
        timeout.tv_sec = *msec/1000;
        timeout.tv_usec = (*msec-((int)(*msec/1000)*1000))*1000;
    }

    /* update remaining timeout ------------------------------------------ */

    struct timespec start;
#ifdef _WIN32
    clock_gettime_win(&start);
#else
    clock_gettime(MONOTONIC_CLOCK_NAME, &start);
#endif

    /* set up fd-s and wait ---------------------------------------------- */

    if ( readwrite == NPP_OPER_READ )
    {
        DDBG("NPP_OPER_READ, waiting on select()...");
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        socks = select(sock+1, &readfds, NULL, NULL, &timeout);
    }
    else if ( readwrite == NPP_OPER_WRITE )
    {
        DDBG("NPP_OPER_WRITE, waiting on select()...");
        FD_ZERO(&writefds);
        FD_SET(sock, &writefds);
        socks = select(sock+1, NULL, &writefds, NULL, &timeout);
    }
    else if ( readwrite == NPP_OPER_CONNECT || readwrite == NPP_OPER_SHUTDOWN )   /* SSL only! */
    {
#ifdef NPP_HTTPS
        DDBG("SSL_connect or SSL_shutdown, determining the next step...");

        DDBG("len = %d", len);

        ssl_err = SSL_get_error((SSL*)ssl, len);

        if ( ssl_err==SSL_ERROR_WANT_READ )
            return lib_finish_with_timeout(sock, oper, NPP_OPER_READ, buffer, len, msec, ssl, level+1);
        else if ( ssl_err==SSL_ERROR_WANT_WRITE )
            return lib_finish_with_timeout(sock, oper, NPP_OPER_WRITE, buffer, len, msec, ssl, level+1);
        else if ( !npp_lib_check_ssl_error(ssl_err) )   /* not cool */
        {
            DBG("SSL_connect or SSL_shutdown error %d", ssl_err);
            return -1;
        }
        else
        {
            DBG("SSL_connect or SSL_shutdown seems OK");
            return 1;
        }
#endif  /* NPP_HTTPS */
    }
    else
    {
        ERR("lib_finish_with_timeout -- invalid readwrite (%d) for this operation (%d)", readwrite, oper);
    }

    *msec -= npp_elapsed(&start);
    if ( *msec < 1 ) *msec = 1;
    DDBG("msec reduced to %d ms", *msec);

    /* process select result --------------------------------------------- */

    if ( socks < 0 )
    {
        ERR("select failed, errno = %d (%s)", errno, strerror(errno));
        return -1;
    }
    else if ( socks == 0 )
    {
        WAR("lib_finish_with_timeout timeouted (was waiting for %.2f ms)", npp_elapsed(&start));
        return -1;
    }
    else    /* socket is ready for I/O */
    {
        DDBG("lib_finish_with_timeout socks > 0");

        if ( readwrite == NPP_OPER_READ )
        {
#ifdef NPP_HTTPS
            if ( ssl )
            {
                if ( buffer )
                    bytes = SSL_read((SSL*)ssl, buffer, len);
                else if ( oper == NPP_OPER_CONNECT )
                    bytes = SSL_connect((SSL*)ssl);
                else    /* NPP_OPER_SHUTDOWN */
                    bytes = SSL_shutdown((SSL*)ssl);

                if ( bytes > 0 )
                {
                    return bytes;
                }
                else
                {
                    DDBG("bytes = %d", bytes);

                    ssl_err = SSL_get_error((SSL*)ssl, bytes);

                    if ( ssl_err==SSL_ERROR_WANT_READ )
                        return lib_finish_with_timeout(sock, oper, NPP_OPER_READ, buffer, len, msec, ssl, level+1);
                    else if ( ssl_err==SSL_ERROR_WANT_WRITE )
                        return lib_finish_with_timeout(sock, oper, NPP_OPER_WRITE, buffer, len, msec, ssl, level+1);
                    else if ( !npp_lib_check_ssl_error(ssl_err) )   /* not cool */
                    {
                        DBG("SSL_read error %d", ssl_err);
                        return -1;
                    }
                }
            }
            else
#endif  /* NPP_HTTPS */
                return recv(sock, buffer, len, 0);
        }
        else if ( readwrite == NPP_OPER_WRITE )
        {
#ifdef NPP_HTTPS
            if ( ssl )
            {
                if ( buffer )
                    bytes = SSL_write((SSL*)ssl, buffer, len);
                else if ( oper == NPP_OPER_CONNECT )
                    bytes = SSL_connect((SSL*)ssl);
                else    /* NPP_OPER_SHUTDOWN */
                    bytes = SSL_shutdown((SSL*)ssl);

                if ( bytes > 0 )
                {
                    return bytes;
                }
                else
                {
                    DDBG("bytes = %d", bytes);

                    ssl_err = SSL_get_error((SSL*)ssl, bytes);

                    if ( ssl_err==SSL_ERROR_WANT_WRITE )
                        return lib_finish_with_timeout(sock, oper, NPP_OPER_WRITE, buffer, len, msec, ssl, level+1);
                    else if ( ssl_err==SSL_ERROR_WANT_READ )
                        return lib_finish_with_timeout(sock, oper, NPP_OPER_READ, buffer, len, msec, ssl, level+1);
                    else if ( !npp_lib_check_ssl_error(ssl_err) )   /* not cool */
                    {
                        DBG("SSL_write error %d", ssl_err);
                        return -1;
                    }
                }
            }
            else
#endif  /* NPP_HTTPS */
                return send(sock, buffer, len, 0);
        }
        else
        {
            ERR("lib_finish_with_timeout -- should have never reached this!");
            return -1;
        }
    }

    DBG("lib_finish_with_timeout reached the end (returning 1)");

    return 1;
}


/* --------------------------------------------------------------------------
   HTTP call / connect
-------------------------------------------------------------------------- */
static bool call_http_connect(const char *host, const char *port, struct timespec *start, int *timeout_remain, bool secure)
{
static struct {
    char host[NPP_MAX_HOST_LEN+1];
    char port[16];
    struct addrinfo addr;
    struct sockaddr ai_addr;
} addresses[CALL_HTTP_ADDRESSES_CACHE_SIZE];
static int addresses_cnt=0, addresses_last=0;
    int  i;
    bool addr_cached=FALSE;

    DBG("call_http_connect [%s:%s]", host, port);

#ifdef _WIN32   /* Windows */

    if ( !M_WSA_initialized )
    {
        DBG("Initializing Winsock...");

        if ( WSAStartup(MAKEWORD(2,2), &M_wsa) != 0 )
        {
            ERR("WSAStartup failed. Error Code = %d", WSAGetLastError());
            return FALSE;
        }

        M_WSA_initialized = TRUE;
    }

#endif  /* _WIN32 */

    struct addrinfo *result=NULL;

#ifndef CALL_HTTP_DONT_CACHE_ADDRINFO

    DBG("Trying cache...");

    DDBG("addresses_cnt=%d", addresses_cnt);

    for ( i=0; i<addresses_cnt; ++i )
    {
        if ( 0==strcmp(addresses[i].host, host) && 0==strcmp(addresses[i].port, port) )
        {
            DBG("Host [%s:%s] found in cache (%d)", host, port, i);
            addr_cached = TRUE;
            result = &addresses[i].addr;
            break;
        }
    }

#endif  /* CALL_HTTP_DONT_CACHE_ADDRINFO */

    if ( !addr_cached )
    {
#ifndef CALL_HTTP_DONT_CACHE_ADDRINFO
        DBG("Host [%s:%s] not found in cache", host, port);
#endif
        DBG("getaddrinfo...");   /* TODO: change to asynchronous, i.e. getaddrinfo_a */

        struct addrinfo hints;
        int s;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        if ( (s=getaddrinfo(host, port, &hints, &result)) != 0 )
        {
            ERR("getaddrinfo: %s", gai_strerror(s));
            return FALSE;
        }

        DDBG("elapsed after getaddrinfo: %.3lf ms", npp_elapsed(start));

        *timeout_remain = G_callHTTPTimeout - npp_elapsed(start);

        if ( *timeout_remain < 1 ) *timeout_remain = 1;

        /* getaddrinfo() returns a list of address structures.
           Try each address until we successfully connect */
    }

    DBG("Trying to connect...");

    struct addrinfo *rp;

    for ( rp=result; rp!=NULL; rp=rp->ai_next )
    {
        DDBG("Trying socket...");

        M_call_http_socket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

#ifdef _WIN32
        if ( M_call_http_socket == INVALID_SOCKET ) continue;
#else
        if ( M_call_http_socket == -1 ) continue;
#endif

#ifdef NPP_DEBUG
        DBG("socket succeeded");
        DBG("elapsed after socket: %.3lf ms", npp_elapsed(start));
#endif

        *timeout_remain = G_callHTTPTimeout - npp_elapsed(start);
        if ( *timeout_remain < 1 ) *timeout_remain = 1;

        /* Windows timeout option is a s**t -- go for non-blocking I/O */

        npp_lib_setnonblocking(M_call_http_socket);

        int timeout_tmp = G_callHTTPTimeout/5;

        DDBG("timeout_tmp = %d", timeout_tmp);

        /* plain socket connection --------------------------------------- */

#ifdef NPP_DEBUG
        int finish_res;
#endif

        if ( connect(M_call_http_socket, rp->ai_addr, rp->ai_addrlen) != -1 )
        {
            DDBG("Immediate connect success");
            break;  /* immediate success */
        }
#ifdef NPP_DEBUG
        else if ( (finish_res=lib_finish_with_timeout(M_call_http_socket, NPP_OPER_CONNECT, NPP_OPER_WRITE, NULL, 0, &timeout_tmp, NULL, 0)) == 0 )
#else
        else if ( lib_finish_with_timeout(M_call_http_socket, NPP_OPER_CONNECT, NPP_OPER_WRITE, NULL, 0, &timeout_tmp, NULL, 0) == 0 )
#endif
        {
            DDBG("Success within timeout");
            break;  /* success within timeout */
        }
        else
        {
#ifdef NPP_DEBUG
            int last_errno = errno;
#endif
            ERR("Couldn't connect with lib_finish_with_timeout");
#ifdef NPP_DEBUG
            DDBG("finish_res = %d", finish_res);
            if ( finish_res == -1 )
                DDBG("errno = %d (%s)", last_errno, strerror(last_errno));
#endif
        }

        close_conn(M_call_http_socket);   /* no cigar */
    }

    if ( rp == NULL )   /* no address succeeded */
    {
        ERR("Could not connect");
        if ( result && !addr_cached ) freeaddrinfo(result);
        return FALSE;
    }

    /* -------------------------------------------------------------------------- */

    *timeout_remain = G_callHTTPTimeout - npp_elapsed(start);
    if ( *timeout_remain < 1 ) *timeout_remain = 1;

    /* -------------------------------------------------------------------------- */

    if ( !addr_cached )   /* add to cache */
    {
#ifndef CALL_HTTP_DONT_CACHE_ADDRINFO
        strcpy(addresses[addresses_last].host, host);
        strcpy(addresses[addresses_last].port, port);
        memcpy(&addresses[addresses_last].addr, rp, sizeof(struct addrinfo));
        /* addrinfo contains pointers -- mind the shallow copy! */
        memcpy(&addresses[addresses_last].ai_addr, rp->ai_addr, sizeof(struct sockaddr));
        addresses[addresses_last].addr.ai_addr = &addresses[addresses_last].ai_addr;
        addresses[addresses_last].addr.ai_next = NULL;

        /* get the remote address */
        char remote_addr[INET_ADDRSTRLEN]="";
        struct sockaddr_in *remote_addr_struct = (struct sockaddr_in*)rp->ai_addr;
#ifdef _WIN32   /* Windows */
        strcpy(remote_addr, inet_ntoa(remote_addr_struct->sin_addr));
#else
        inet_ntop(AF_INET, &(remote_addr_struct->sin_addr), remote_addr, INET_ADDRSTRLEN);
#endif
        INF("Connected to %s", remote_addr);

        DBG("Host [%s:%s] added to cache (%d)", host, port, addresses_last);

        if ( addresses_cnt < CALL_HTTP_ADDRESSES_CACHE_SIZE-1 )   /* first round */
        {
            ++addresses_cnt;    /* cache usage */
            ++addresses_last;   /* next to use index */
        }
        else    /* cache full -- reuse it from start */
        {
            if ( addresses_last < CALL_HTTP_ADDRESSES_CACHE_SIZE-1 )
                ++addresses_last;
            else
                addresses_last = 0;
        }

#endif  /* CALL_HTTP_DONT_CACHE_ADDRINFO */

        freeaddrinfo(result);
    }

    DBG("Connected");

    DDBG("elapsed after plain connect: %.3lf ms", npp_elapsed(start));

    /* -------------------------------------------------------------------------- */

#ifdef NPP_HTTPS

    if ( secure )
    {
        DBG("Trying SSL_new...");

        M_call_http_ssl = SSL_new(M_ssl_ctx);

        if ( !M_call_http_ssl )
        {
            ERR("SSL_new failed");
            close_conn(M_call_http_socket);
            return FALSE;
        }

        DBG("Trying SSL_set_fd...");

        int ret = SSL_set_fd(M_call_http_ssl, M_call_http_socket);

        if ( ret <= 0 )
        {
            ERR("SSL_set_fd failed, ret = %d", ret);
            close_conn(M_call_http_socket);
            SSL_free(M_call_http_ssl);
            M_call_http_ssl = NULL;
            return FALSE;
        }

        DBG("Trying SSL_set_tlsext_host_name...");

        ret = SSL_set_tlsext_host_name(M_call_http_ssl, host);

        if ( ret <= 0 )
        {
            ERR("SSL_set_tlsext_host_name failed, ret = %d", ret);
            close_conn(M_call_http_socket);
            SSL_free(M_call_http_ssl);
            M_call_http_ssl = NULL;
            return FALSE;
        }

        DBG("Trying SSL_connect...");

        ret = SSL_connect(M_call_http_ssl);

        DDBG("ret = %d", ret);    /* 1 = success */

        if ( ret == 1 )
        {
            DBG("SSL_connect immediate success");
        }
        else if ( lib_finish_with_timeout(M_call_http_socket, NPP_OPER_CONNECT, NPP_OPER_CONNECT, NULL, ret, timeout_remain, M_call_http_ssl, 0) > 0 )
        {
            DBG("SSL_connect successful");
        }
        else
        {
            ERR("SSL_connect failed");
            close_conn(M_call_http_socket);
            SSL_free(M_call_http_ssl);
            M_call_http_ssl = NULL;
            return FALSE;
        }

        DDBG("elapsed after SSL connect: %.3lf ms", npp_elapsed(start));

//        M_ssl_session = SSL_get_session(M_call_http_ssl);

        X509 *server_cert;
        server_cert = SSL_get_peer_certificate(M_call_http_ssl);
        if ( server_cert )
        {
            DBG("Got server certificate");
            X509_NAME *certname;
            certname = X509_NAME_new();
            certname = X509_get_subject_name(server_cert);
            DBG("server_cert [%s]", X509_NAME_oneline(certname, NULL, 0));
            X509_free(server_cert);
        }
        else
            WAR("Couldn't get server certificate");
    }
#endif  /* NPP_HTTPS */

    return TRUE;
}


/* --------------------------------------------------------------------------
   HTTP call / disconnect
-------------------------------------------------------------------------- */
static void call_http_disconnect(int ssl_ret)
{
    DBG("call_http_disconnect");

#ifdef NPP_HTTPS
    if ( M_call_http_ssl )
    {
        bool shutdown=TRUE;

        if ( ssl_ret < 0 )
        {
            int ssl_err = SSL_get_error(M_call_http_ssl, ssl_ret);

            if ( ssl_err==SSL_ERROR_SYSCALL || ssl_err==SSL_ERROR_SSL )
                shutdown = FALSE;
        }

        if ( shutdown )
        {
            int timeout_tmp = G_callHTTPTimeout/5;

            int ret = SSL_shutdown(M_call_http_ssl);

            DDBG("SSL_shutdown returned %d (1)", ret);

            if ( ret == 1 )
            {
                DBG("SSL_shutdown immediate success");
            }
            else if ( ret == 0 )    /* shutdown not finished */
            {
                DBG("First SSL_shutdown looks fine, trying to complete the bidirectional shutdown handshake...");

                ret = SSL_shutdown(M_call_http_ssl);

                DDBG("SSL_shutdown returned %d (2)", ret);

                if ( ret == 1 )
                    DBG("SSL_shutdown success");
                else if ( lib_finish_with_timeout(M_call_http_socket, NPP_OPER_SHUTDOWN, NPP_OPER_SHUTDOWN, NULL, ret, &timeout_tmp, M_call_http_ssl, 0) > 0 )
                    DBG("SSL_shutdown successful");
                else
                    ERR("SSL_shutdown failed (2)");
            }
            else if ( lib_finish_with_timeout(M_call_http_socket, NPP_OPER_SHUTDOWN, NPP_OPER_SHUTDOWN, NULL, ret, &timeout_tmp, M_call_http_ssl, 0) > 0 )
            {
                DBG("SSL_shutdown successful");
            }
            else
            {
                ERR("SSL_shutdown failed (1)");
            }
        }

        SSL_free(M_call_http_ssl);

/*        if ( M_ssl_session )
            SSL_SESSION_free(M_ssl_session); */

        M_call_http_ssl = NULL;
    }
#endif  /* NPP_HTTPS */

    close_conn(M_call_http_socket);
}


/* --------------------------------------------------------------------------
   HTTP call / convert chunked to normal content
   Return number of bytes written to res_content
-------------------------------------------------------------------------- */
static int chunked2content(char *res_content, const char *buffer, size_t src_len, size_t res_len)
{
    char *res=res_content;
    char chunk_size_str[8];
    unsigned chunk_size=src_len;
    const char *p=buffer;
    unsigned was_read=0, written=0;

    while ( chunk_size > 0 )    /* chunk by chunk */
    {
        /* get the chunk size */

        unsigned i = 0;

        while ( *p!='\r' && *p!='\n' && i<6 && i<src_len )
            chunk_size_str[i++] = *p++;

        chunk_size_str[i] = EOS;

        DDBG("chunk_size_str [%s]", chunk_size_str);

        sscanf(chunk_size_str, "%x", &chunk_size);
        DBG("chunk_size = %u", chunk_size);

        was_read += i;

        /* --------------------------------------------------------------- */

        if ( chunk_size == 0 )    /* last one */
        {
            DBG("Last chunk");
            break;
        }
        else if ( chunk_size > res_len-written )
        {
            WAR("chunk_size > res_len-written");
            break;
        }

        /* --------------------------------------------------------------- */
        /* skip "\r\n" */

        DDBG("skip %d (should be 13)", *p);

        ++p;    /* skip '\r' */

        DDBG("skip %d (should be 10)", *p);

        ++p;    /* skip '\n' */

        was_read += 2;

        /* once again may be needed */

        if ( *p == '\r' )
        {
            DDBG("skip 13");
            ++p;    /* skip '\r' */
            ++was_read;
        }

        if ( *p == '\n' )
        {
            DDBG("skip 10");
            ++p;    /* skip '\n' */
            ++was_read;
        }

        DDBG("was_read = %d", was_read);

        /* --------------------------------------------------------------- */

        if ( was_read >= src_len || chunk_size > src_len-was_read )
        {
            WAR("Unexpected end of buffer");
            break;
        }

        /* --------------------------------------------------------------- */
        /* copy chunk to destination */

        /* stpncpy() returns a pointer to the terminating null byte in dest, or,
           if dest is not null-terminated, dest+n. */

        DBG("Appending %d bytes to the content buffer", chunk_size);

        DDBG("p starts with '%c'", *p);

        res = stpncpy(res, p, chunk_size);
        written += chunk_size;

        DDBG("written = %d", written);

        p += chunk_size;
        was_read += chunk_size;

        /* --------------------------------------------------------------- */

        while ( *p != '\n' && was_read<src_len-1 )
        {
            DDBG("skip %d (expected 13)", *p);
            ++p;
            ++was_read;
        }

        DDBG("skip %d (should be 10)", *p);

        ++p;    /* skip '\n' */

        ++was_read;
    }

    return written;
}


/* --------------------------------------------------------------------------
   HTTP call / get response content length
-------------------------------------------------------------------------- */
static int call_http_res_content_length(const char *u_res_header, int len)
{
    const char *p;

    if ( (p=strstr(u_res_header, "\nCONTENT-LENGTH: ")) == NULL )
        return -1;

    if ( len < (p-u_res_header) + 18 ) return -1;

    char result_str[8];
    int  i = 0;

    p += 17;

    while ( isdigit(*p) && i<7 )
    {
        result_str[i++] = *p++;
    }

    result_str[i] = EOS;

    DDBG("result_str [%s]", result_str);

    return atoi(result_str);
}



#define NPP_TRANSFER_MODE_NORMAL     '1'
#define NPP_TRANSFER_MODE_NO_CONTENT '2'
#define NPP_TRANSFER_MODE_CHUNKED    '3'
#define NPP_TRANSFER_MODE_ERROR      '4'


/* --------------------------------------------------------------------------
   HTTP call / parse response
-------------------------------------------------------------------------- */
static bool call_http_res_parse(char *res_header, int bytes)
{
    /* HTTP/1.1 200 OK <== 15 chars */

    char status[4];

    if ( bytes < 14 || 0 != strncmp(res_header, "HTTP/1.", 7) )
    {
        return FALSE;
    }

    res_header[bytes] = EOS;
#ifdef NPP_DEBUG
    DBG("");
    DBG("Got %d bytes of response [%s]", bytes, res_header);
#else
    DBG("Got %d bytes of response", bytes);
#endif  /* NPP_DEBUG */

    /* Status */

    strncpy(status, res_header+9, 3);
    status[3] = EOS;
    G_call_http_status = atoi(status);
    INF("CALL_HTTP response status: %s", status);

    char u_res_header[CALL_HTTP_RES_HEADER_LEN+1];   /* uppercase */
    strcpy(u_res_header, npp_upper(res_header));

    /* Content-Type */

    const char *p;

    if ( (p=strstr(u_res_header, "\nCONTENT-TYPE: ")) == NULL )
    {
        G_call_http_content_type[0] = EOS;
    }
    else if ( bytes < (p-res_header) + 16 )
    {
        G_call_http_content_type[0] = EOS;
    }
    else
    {
        int i = 0;

        p += 15;

        while ( *p != '\r' && *p != '\n' && *p && i<255 )
        {
            G_call_http_content_type[i++] = *p++;
        }

        G_call_http_content_type[i] = EOS;

        DBG("CALL_HTTP content type [%s]", G_call_http_content_type);
    }

    /* content length */

    G_call_http_res_len = call_http_res_content_length(u_res_header, bytes);

    if ( G_call_http_res_len > CALL_HTTP_MAX_RESPONSE_LEN-1 )
    {
        WAR("Response content is too big (%d)", G_call_http_res_len);
        return FALSE;
    }

    if ( G_call_http_res_len > 0 )     /* Content-Length present in response */
    {
        DBG("NPP_TRANSFER_MODE_NORMAL");
        M_call_http_mode = NPP_TRANSFER_MODE_NORMAL;
    }
    else if ( G_call_http_res_len == 0 )
    {
        DBG("NPP_TRANSFER_MODE_NO_CONTENT");
        M_call_http_mode = NPP_TRANSFER_MODE_NO_CONTENT;
    }
    else    /* content length == -1 */
    {
        if ( strstr(u_res_header, "\nTRANSFER-ENCODING: CHUNKED") != NULL )
        {
            DBG("NPP_TRANSFER_MODE_CHUNKED");
            M_call_http_mode = NPP_TRANSFER_MODE_CHUNKED;
        }
        else
        {
            WAR("NPP_TRANSFER_MODE_ERROR");
            M_call_http_mode = NPP_TRANSFER_MODE_ERROR;
            return FALSE;
        }
    }

    return TRUE;
}


/* --------------------------------------------------------------------------
   HTTP call
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
bool npp_call_http(const void *req, void *res, const std::string& method_, const std::string& url_, bool json, bool keep)
{
    const char *method = method_.c_str();
    const char *url = url_.c_str();
#else
bool npp_call_http(const void *req, void *res, const char *method, const char *url, bool json, bool keep)
{
#endif
    char     host[NPP_MAX_HOST_LEN+1];
    char     port[8];
    bool     secure=FALSE;
static char  prev_host[NPP_MAX_HOST_LEN+1];
static char  prev_port[8];
static bool  prev_secure=FALSE;
    char     uri[NPP_MAX_URI_LEN+1];
static bool  connected=FALSE;
static time_t connected_time=0;
    char     res_header[CALL_HTTP_RES_HEADER_LEN+1];
static char  buffer[CALL_HTTP_MAX_RESPONSE_LEN];
    int      bytes=0;
    char     *body;
    unsigned content_read=0, buffer_read=0;
    unsigned len;
    int      timeout_remain = G_callHTTPTimeout;

    DBG("npp_call_http [%s] [%s]", method, url);

    /* -------------------------------------------------------------------------- */

    if ( !call_http_parse_url(url, host, port, uri, &secure) ) return FALSE;

    if ( M_call_http_proxy )
        strcpy(uri, url);

    len = call_http_render_req(buffer, method, host, uri, req, json, keep);

#ifdef NPP_DEBUG
    DBG("------------------------------------------------------------");
    DBG("npp_call_http buffer [%s]", buffer);
    DBG("------------------------------------------------------------");
#endif  /* NPP_DEBUG */

    struct timespec start;
#ifdef _WIN32
    clock_gettime_win(&start);
#else
    clock_gettime(MONOTONIC_CLOCK_NAME, &start);
#endif

    /* -------------------------------------------------------------------------- */

    if ( connected
            && (secure!=prev_secure || 0 != strcmp(host, prev_host) || 0 != strcmp(port, prev_port) || G_now-connected_time > NPP_CONNECTION_TIMEOUT) )
    {
        /* reconnect anyway */
        DBG("different host, port or old connection, reconnecting");
        call_http_disconnect(0);
        connected = FALSE;
    }

    bool was_connected = connected;

    /* connect if necessary ----------------------------------------------------- */

    if ( !connected && !call_http_connect(host, port, &start, &timeout_remain, secure) ) return FALSE;

    /* -------------------------------------------------------------------------- */

    DBG("Sending request...");

    bool after_reconnect=0;

    while ( timeout_remain > 1 )
    {
#ifdef NPP_HTTPS
        if ( secure )
        {
/*            char first_char[2];
            first_char[0] = buffer[0];
            first_char[1] = EOS;

            bytes = SSL_write(M_call_http_ssl, first_char, 1);

            if ( bytes > 0 )
                bytes = SSL_write(M_call_http_ssl, buffer+1, len-1) + bytes; */

            bytes = SSL_write(M_call_http_ssl, buffer, len);
        }
        else
#endif  /* NPP_HTTPS */
            bytes = send(M_call_http_socket, buffer, len, 0);    /* try in one go */

        if ( !secure && bytes <= 0 )
        {
            if ( !was_connected || after_reconnect )
            {
                ERR("Send (after fresh connect) failed");
                call_http_disconnect(0);
                connected = FALSE;
                return FALSE;
            }

            DBG("Disconnected? Trying to reconnect...");
            call_http_disconnect(0);
            if ( !call_http_connect(host, port, &start, &timeout_remain, secure) ) return FALSE;
            after_reconnect = 1;
        }
        else if ( secure && bytes == -1 )
        {
            bytes = lib_finish_with_timeout(M_call_http_socket, NPP_OPER_WRITE, NPP_OPER_WRITE, buffer, len, &timeout_remain, M_call_http_ssl, 0);

            if ( bytes == -1 )
            {
                if ( !was_connected || after_reconnect )
                {
                    ERR("Send (after fresh connect) failed");
                    call_http_disconnect(-1);
                    connected = FALSE;
                    return FALSE;
                }

                DBG("Disconnected? Trying to reconnect...");
                call_http_disconnect(-1);
                if ( !call_http_connect(host, port, &start, &timeout_remain, secure) ) return FALSE;
                after_reconnect = 1;
            }
        }
        else    /* bytes > 0 ==> OK */
        {
            break;
        }
    }

    DDBG("Sent %d bytes", bytes);

    if ( bytes < 15 )
    {
        ERR("send failed, errno = %d (%s)", errno, strerror(errno));
        call_http_disconnect(bytes);
        connected = FALSE;
        return FALSE;
    }

    DDBG("elapsed after request: %.3lf ms", npp_elapsed(&start));

    /* -------------------------------------------------------------------------- */

    DBG("Reading response...");

#ifdef NPP_HTTPS
    if ( secure )
        bytes = SSL_read(M_call_http_ssl, res_header, CALL_HTTP_RES_HEADER_LEN);
    else
#endif  /* NPP_HTTPS */
        bytes = recv(M_call_http_socket, res_header, CALL_HTTP_RES_HEADER_LEN, 0);

    if ( bytes == -1 )
    {
        bytes = lib_finish_with_timeout(M_call_http_socket, NPP_OPER_READ, NPP_OPER_READ, res_header, CALL_HTTP_RES_HEADER_LEN, &timeout_remain, secure?M_call_http_ssl:NULL, 0);

        if ( bytes <= 0 )
        {
            ERR("recv failed, errno = %d (%s)", errno, strerror(errno));
            call_http_disconnect(bytes);
            connected = FALSE;
            return FALSE;
        }
    }

    DBG("Read %d bytes", bytes);

    DDBG("elapsed after first response read: %.3lf ms", npp_elapsed(&start));

    /* -------------------------------------------------------------------------- */
    /* parse the response                                                         */
    /* we assume that at least response header arrived at once                    */

    if ( !call_http_res_parse(res_header, bytes) )
    {
        ERR("No or invalid response");

#ifdef NPP_DEBUG
        if ( bytes >= 0 )
        {
            res_header[bytes] = EOS;
            DBG("Got %d bytes of response [%s]", bytes, res_header);
        }
#endif  /* NPP_DEBUG */

        G_call_http_status = 500;
        call_http_disconnect(bytes);
        connected = FALSE;
        return FALSE;
    }

    /* ------------------------------------------------------------------- */
    /* at this point we've got something that seems to be a HTTP header,
       possibly with content */

static char res_content[CALL_HTTP_MAX_RESPONSE_LEN];

    /* ------------------------------------------------------------------- */
    /* some content may have already been read                             */

    body = strstr(res_header, "\r\n\r\n");

    if ( body )
    {
        body += 4;

        int was_read = bytes - (body-res_header);

        if ( was_read > 0 )
        {
            if ( M_call_http_mode == NPP_TRANSFER_MODE_NORMAL )   /* not chunked goes directly to res_content */
            {
                content_read = was_read;
                strncpy(res_content, body, content_read);
            }
            else if ( M_call_http_mode == NPP_TRANSFER_MODE_CHUNKED )   /* chunked goes to buffer before res_content */
            {
                buffer_read = was_read;
                strncpy(buffer, body, buffer_read);
            }
        }
    }

    /* ------------------------------------------------------------------- */
    /* read content                                                        */

    if ( M_call_http_mode == NPP_TRANSFER_MODE_NORMAL )
    {
        while ( content_read < (unsigned)G_call_http_res_len && timeout_remain > 1 )   /* read whatever we can within timeout */
        {
            DDBG("trying again (content-length)");

#ifdef NPP_HTTPS
            if ( secure )
                bytes = SSL_read(M_call_http_ssl, res_content+content_read, CALL_HTTP_MAX_RESPONSE_LEN-content_read-1);
            else
#endif  /* NPP_HTTPS */
                bytes = recv(M_call_http_socket, res_content+content_read, CALL_HTTP_MAX_RESPONSE_LEN-content_read-1, 0);

            if ( bytes == -1 )
                bytes = lib_finish_with_timeout(M_call_http_socket, NPP_OPER_READ, NPP_OPER_READ, res_content+content_read, CALL_HTTP_MAX_RESPONSE_LEN-content_read-1, &timeout_remain, secure?M_call_http_ssl:NULL, 0);

            if ( bytes > 0 )
                content_read += bytes;
        }

        if ( bytes < 1 )
        {
            DBG("timeouted?");
            call_http_disconnect(bytes);
            connected = FALSE;
            return FALSE;
        }
    }
    else if ( M_call_http_mode == NPP_TRANSFER_MODE_CHUNKED )
    {
        /* for single-threaded process, I can't see better option than to read everything
           into a buffer and then parse and copy chunks into final res_content */

        /* there's no guarantee that every read = one chunk, so just read whatever comes in, until it does */

        while ( bytes > 0 && timeout_remain > 1 )   /* read whatever we can within timeout */
        {

#ifdef NPP_DEBUG
            DBG("Has the last chunk been read?");
            DBG("buffer_read = %u", buffer_read);
            if ( buffer_read > 5 )
            {
                unsigned ii;
                for ( ii=buffer_read-6; ii<buffer_read; ++ii )
                {
                    if ( buffer[ii] == '\r' )
                        DBG("buffer[%d] '\\r'", ii);
                    else if ( buffer[ii] == '\n' )
                        DBG("buffer[%d] '\\n'", ii);
                    else
                        DBG("buffer[%d] '%c'", ii, buffer[ii]);
                }
            }
#endif  /* NPP_DEBUG */

            if ( buffer_read>5 && buffer[buffer_read-6]=='\n' && buffer[buffer_read-5]=='0' && buffer[buffer_read-4]=='\r' && buffer[buffer_read-3]=='\n' && buffer[buffer_read-2]=='\r' && buffer[buffer_read-1]=='\n' )
            {
                DBG("Last chunk detected (with \\r\\n\\r\\n)");
                break;
            }
            else if ( buffer_read>3 && buffer[buffer_read-4]=='\n' && buffer[buffer_read-3]=='0' && buffer[buffer_read-2]=='\r' && buffer[buffer_read-1]=='\n' )
            {
                DBG("Last chunk detected (with \\r\\n)");
                break;
            }

            DDBG("trying again (chunked)");

#ifdef NPP_HTTPS
            if ( secure )
                bytes = SSL_read(M_call_http_ssl, buffer+buffer_read, CALL_HTTP_MAX_RESPONSE_LEN-buffer_read-1);
            else
#endif  /* NPP_HTTPS */
                bytes = recv(M_call_http_socket, buffer+buffer_read, CALL_HTTP_MAX_RESPONSE_LEN-buffer_read-1, 0);

            if ( bytes == -1 )
                bytes = lib_finish_with_timeout(M_call_http_socket, NPP_OPER_READ, NPP_OPER_READ, buffer+buffer_read, CALL_HTTP_MAX_RESPONSE_LEN-buffer_read-1, &timeout_remain, secure?M_call_http_ssl:NULL, 0);

            if ( bytes > 0 )
                buffer_read += bytes;
        }

        if ( buffer_read < 5 )
        {
            ERR("buffer_read is only %u, this can't be valid chunked content", buffer_read);
            call_http_disconnect(bytes);
            connected = FALSE;
            return FALSE;
        }

        content_read = chunked2content(res_content, buffer, buffer_read, CALL_HTTP_MAX_RESPONSE_LEN);
    }

    /* ------------------------------------------------------------------- */

    res_content[content_read] = EOS;

    DBG("Read %d bytes of content", content_read);

    G_call_http_res_len = content_read;

#ifdef NPP_DEBUG
    npp_log_long(res_content, content_read, "Content");
#endif

    /* ------------------------------------------------------------------- */

    if ( !keep || strstr(res_header, "\nConnection: close") != NULL || strstr(res_header, "\nConnection: Close") != NULL )
    {
        DBG("Closing connection");
        call_http_disconnect(bytes);
        connected = FALSE;
    }
    else    /* keep the connection open */
    {
#ifdef NPP_HTTPS
        prev_secure = secure;
#endif
        strcpy(prev_host, host);
        strcpy(prev_port, port);
        connected = TRUE;
        connected_time = G_now;
    }

    DDBG("elapsed after second response read: %.3lf ms", npp_elapsed(&start));

    /* -------------------------------------------------------------------------- */
    /* we expect JSON response in body                                            */

    if ( len && res )
    {
        if ( json )
            lib_json_from_string((JSON*)res, res_content, content_read, 0);
        else
            strcpy((char*)res, res_content);
    }

    /* -------------------------------------------------------------------------- */
    /* stats                                                                      */

    ++G_call_http_req_cnt;

    double elapsed = npp_elapsed(&start);

    DBG("CALL_HTTP finished in %.3lf ms", elapsed);

    G_call_http_elapsed += elapsed;

    G_call_http_average = G_call_http_elapsed / G_call_http_req_cnt;

    return TRUE;
}


/* --------------------------------------------------------------------------
   HTTP call
-------------------------------------------------------------------------- */
void npp_call_http_disconnect()
{
    DBG("npp_call_http_disconnect");
    call_http_disconnect(0);
}


/* --------------------------------------------------------------------------
   Log SSL error

   From openssl.h:

#define SSL_ERROR_NONE        (0)
#define SSL_ERROR_SSL         (1)
#define SSL_ERROR_WANT_READ   (2)
#define SSL_ERROR_WANT_WRITE  (3)
#define SSL_ERROR_SYSCALL     (5)
#define SSL_ERROR_ZERO_RETURN (6)

Return TRUE if everything is cool

Why return TRUE if ssl_err==SSL_ERROR_SYSCALL?

https://www.openssl.org/docs/man1.1.1/man3/SSL_get_error.html

The SSL_ERROR_SYSCALL with errno value of 0 indicates unexpected EOF
from the peer. This will be properly reported as SSL_ERROR_SSL with
reason code SSL_R_UNEXPECTED_EOF_WHILE_READING in the OpenSSL 3.0 release
because it is truly a TLS protocol error to terminate the connection without a SSL_shutdown().

The issue is kept unfixed in OpenSSL 1.1.1 releases because many applications
which choose to ignore this protocol error depend on the existing way of reporting the error.

-------------------------------------------------------------------------- */
bool npp_lib_check_ssl_error(int ssl_err)
{
#ifdef NPP_HTTPS

    if ( ssl_err != SSL_ERROR_SYSCALL ) return TRUE;

#ifdef _WIN32   /* Windows */

    int sockerr = WSAGetLastError();

    wchar_t *s = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, sockerr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&s, 0, NULL);
    DBG("ssl_err=SSL_ERROR_SYSCALL, errno=%d (%S)", sockerr, s);
    LocalFree(s);

#else   /* Linux */

    int sockerr = errno;

    DBG("ssl_err=SSL_ERROR_SYSCALL, errno=%d (%s)", sockerr, strerror(sockerr));

#endif  /* _WIN32 */

    if ( sockerr == 0 )
        return TRUE;
    else
        return FALSE;

#endif  /* NPP_HTTPS */

    return TRUE;
}


/* --------------------------------------------------------------------------
   Set G_appdir
-------------------------------------------------------------------------- */
void npp_lib_get_app_dir()
{
    char *appdir=NULL;

    if ( NULL != (appdir=getenv("NPP_DIR")) )
    {
        strcpy(G_appdir, appdir);
        int len = strlen(G_appdir);
        if ( G_appdir[len-1] == '/' ) G_appdir[len-1] = EOS;
    }
    else
    {
        G_appdir[0] = EOS;   /* not defined */
    }
}


/* --------------------------------------------------------------------------
   Calculate elapsed time
-------------------------------------------------------------------------- */
double npp_elapsed(struct timespec *start)
{
struct timespec end;
    double      elapsed;
#ifdef _WIN32
    clock_gettime_win(&end);
#else
    clock_gettime(MONOTONIC_CLOCK_NAME, &end);
#endif
    elapsed = (end.tv_sec - start->tv_sec) * 1000.0;
    elapsed += (end.tv_nsec - start->tv_nsec) / 1000000.0;
    return elapsed;
}


/* --------------------------------------------------------------------------
   Log the memory footprint
-------------------------------------------------------------------------- */
void npp_log_memory()
{
    int  mem_used;
    char mem_used_kib[64];
    char mem_used_mib[64];
    char mem_used_gib[64];

    mem_used = npp_get_memory();

    npp_lib_fmt_int_generic(mem_used_kib, mem_used);
    npp_lib_fmt_dec_generic(mem_used_mib, (double)mem_used/1024);
    npp_lib_fmt_dec_generic(mem_used_gib, (double)mem_used/1024/1024);

    ALWAYS_LINE;
    ALWAYS("Memory: %s KiB (%s MiB / %s GiB)", mem_used_kib, mem_used_mib, mem_used_gib);
    ALWAYS_LINE;
}


#ifdef __linux__
/* --------------------------------------------------------------------------
   For lib_memory
-------------------------------------------------------------------------- */
static int mem_parse_line(const char* line)
{
    int     ret=0;
    int     i=0;
    char    strret[64];
    const char* p=line;

    while (!isdigit(*p)) ++p;     /* skip non-digits */

    while (isdigit(*p)) strret[i++] = *p++;

    strret[i] = EOS;

#ifdef NPP_DEBUG
    DBG("mem_parse_line: line [%s]", line);
    DBG("mem_parse_line: strret [%s]", strret);
#endif  /* NPP_DEBUG */

    ret = atoi(strret);

    return ret;
}
#endif  /* __linux__ */


/* --------------------------------------------------------------------------
   Return currently used memory (high water mark) by current process in KiB
-------------------------------------------------------------------------- */
int npp_get_memory()
{
    int result=0;

#ifdef __linux__

    char line[128];
    FILE* file = fopen("/proc/self/status", "r");

    if ( !file )
    {
        ERR("fopen(\"/proc/self/status\" failed, errno = %d (%s)", errno, strerror(errno));
        return result;
    }

    while ( fgets(line, 128, file) != NULL )
    {
        if ( strncmp(line, "VmHWM:", 6) == 0 )
        {
            result = mem_parse_line(line);
            break;
        }
    }

    fclose(file);

#else   /* not Linux */

#ifdef _WIN32   /* Windows */

    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    result = pmc.PrivateUsage / 1024;

#else   /* UNIX */

struct rusage usage;

    getrusage(RUSAGE_SELF, &usage);
    result = usage.ru_maxrss;

#endif  /* _WIN32 */

#endif  /* __linux__ */

    return result;
}


/* --------------------------------------------------------------------------
   Filter everything but basic letters and digits off
---------------------------------------------------------------------------*/
#ifdef NPP_CPP_STRINGS
char *npp_filter_strict(const std::string& src_)
{
    const char *src = src_.c_str();
#else
char *npp_filter_strict(const char *src)
{
#endif
static char dst[NPP_LIB_STR_BUF];
    int     i=0, j=0;

    while ( src[i] && j<NPP_LIB_STR_CHECK )
    {
        if ( (src[i] >= 65 && src[i] <= 90)
                || (src[i] >= 97 && src[i] <= 122)
                || isdigit(src[i]) )
            dst[j++] = src[i];
        else if ( src[i] == ' ' || src[i] == '\t' || src[i] == '\n' )
            dst[j++] = '_';

        ++i;
    }

    dst[j] = EOS;

    return dst;
}


/* --------------------------------------------------------------------------
   Add spaces to make string to have len length
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
char *npp_add_spaces(const std::string& src_, int new_len)
{
    const char *src = src_.c_str();
#else
char *npp_add_spaces(const char *src, int new_len)
{
#endif
static char dst[NPP_LIB_STR_BUF];

    int src_len = strlen(src);

    strcpy(dst, src);

    int i;
    for ( i=src_len; i<new_len; ++i )
        dst[i] = ' ';

    dst[i] = EOS;

    return dst;
}


/* --------------------------------------------------------------------------
   Add leading spaces to make string to have len length
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
char *npp_add_lspaces(const std::string& src_, int new_len)
{
    const char *src = src_.c_str();
#else
char *npp_add_lspaces(const char *src, int new_len)
{
#endif
static char dst[NPP_LIB_STR_BUF];

    int src_len = strlen(src);

    int spaces = new_len - src_len;

    if ( spaces < 0 ) spaces = 0;

    int i;
    for ( i=0; i<spaces; ++i )
        dst[i] = ' ';

    strcpy(dst+spaces, src);

    return dst;
}


/* --------------------------------------------------------------------------
   Extract file name from path
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
char *npp_get_fname_from_path(const std::string& path_)
{
    const char *path = path_.c_str();
#else
char *npp_get_fname_from_path(const char *path)
{
#endif
static char fname[256];
    char *p;

    DDBG("path: [%s]", path);

    if ( (p=(char*)strrchr(path, '/')) == NULL
            && (p=(char*)strrchr(path, '\\')) == NULL )   /* no slash */
    {
        COPY(fname, path, 255);
        return fname;
    }

    if ( (unsigned)(p-path) == strlen(path)-1 )        /* slash is the last char */
    {
        fname[0] = EOS;
        return fname;
    }

    ++p;    /* skip slash */

    COPY(fname, p, 255);

    DDBG("fname: [%s]", fname);

    return fname;
}


/* --------------------------------------------------------------------------
   Get the file extension
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
char *npp_get_file_ext(const std::string& fname_)
{
    const char *fname = fname_.c_str();
#else
char *npp_get_file_ext(const char *fname)
{
#endif
static char ext[64];
    char *p;

    DDBG("name: [%s]", fname);

    if ( (p=(char*)strrchr(fname, '.')) == NULL )     /* no dot */
    {
        ext[0] = EOS;
        return ext;
    }

    if ( (unsigned)(p-fname) == strlen(fname)-1 )     /* dot is the last char */
    {
        ext[0] = EOS;
        return ext;
    }

    ++p;    /* skip dot */

    COPY(ext, p, 63);

    DDBG("ext: [%s]", ext);

    return ext;
}


/* --------------------------------------------------------------------------
   Determine resource type by its extension
-------------------------------------------------------------------------- */
char npp_lib_get_res_type(const char *fname)
{
    char *ext=NULL;
    char uext[8]="";

    if ( (ext=(char*)strrchr(fname, '.')) == NULL )     /* no dot */
        return NPP_CONTENT_TYPE_TEXT;

    if ( (unsigned)(ext-fname) == strlen(fname)-1 )     /* dot is the last char */
        return NPP_CONTENT_TYPE_TEXT;

    ++ext;

    if ( strlen(ext) > 4 )                          /* extension too long */
        return NPP_CONTENT_TYPE_TEXT;

    strcpy(uext, npp_upper(ext));

    if ( 0==strcmp(uext, "HTML") || 0==strcmp(uext, "HTM") )
        return NPP_CONTENT_TYPE_HTML;
    else if ( 0==strcmp(uext, "CSS") )
        return NPP_CONTENT_TYPE_CSS;
    else if ( 0==strcmp(uext, "JS") )
        return NPP_CONTENT_TYPE_JS;
    else if ( 0==strcmp(uext, "GIF") )
        return NPP_CONTENT_TYPE_GIF;
    else if ( 0==strcmp(uext, "JPG") || 0==strcmp(uext, "JPEG") )
        return NPP_CONTENT_TYPE_JPG;
    else if ( 0==strcmp(uext, "ICO") )
        return NPP_CONTENT_TYPE_ICO;
    else if ( 0==strcmp(uext, "PNG") )
        return NPP_CONTENT_TYPE_PNG;
    else if ( 0==strcmp(uext, "BMP") )
        return NPP_CONTENT_TYPE_BMP;
    else if ( 0==strcmp(uext, "SVG") )
        return NPP_CONTENT_TYPE_SVG;
    else if ( 0==strcmp(uext, "JSON") )
        return NPP_CONTENT_TYPE_JSON;
    else if ( 0==strcmp(uext, "MD") )
        return NPP_CONTENT_TYPE_MD;
    else if ( 0==strcmp(uext, "PDF") )
        return NPP_CONTENT_TYPE_PDF;
    else if ( 0==strcmp(uext, "XML") )
        return NPP_CONTENT_TYPE_XML;
    else if ( 0==strcmp(uext, "MP3") )
        return NPP_CONTENT_TYPE_AMPEG;
    else if ( 0==strcmp(uext, "EXE") )
        return NPP_CONTENT_TYPE_EXE;
    else if ( 0==strcmp(uext, "ZIP") )
        return NPP_CONTENT_TYPE_ZIP;
    else if ( 0==strcmp(uext, "GZ") )
        return NPP_CONTENT_TYPE_GZIP;

    return NPP_CONTENT_TYPE_TEXT;
}


/* --------------------------------------------------------------------------
   Convert URI (YYYY-MM-DD) date to tm struct
-------------------------------------------------------------------------- */
void date_str2rec(const char *str, date_t *rec)
{
    int     len;
    int     i;
    int     j=0;
    char    part='Y';
    char    strtmp[8];

    len = strlen(str);

    /* empty or invalid date => return today */

    if ( len != 10 || str[4] != '-' || str[7] != '-' )
    {
        DBG("date_str2rec: empty or invalid date in URI, returning today");
        rec->year = G_ptm->tm_year+1900;
        rec->month = G_ptm->tm_mon+1;
        rec->day = G_ptm->tm_mday;
        return;
    }

    for ( i=0; i<len; ++i )
    {
        if ( str[i] != '-' )
        {
//            DBG("str[i] = %c", str[i]);
            strtmp[j++] = str[i];
        }
        else    /* end of part */
        {
            strtmp[j] = EOS;

            if ( part == 'Y' )  /* year */
            {
                rec->year = atoi(strtmp);
                part = 'M';
            }
            else if ( part == 'M' )  /* month */
            {
                rec->month = atoi(strtmp);
                part = 'D';
            }

            j = 0;
        }
    }

    /* day */

    strtmp[j] = EOS;
    rec->day = atoi(strtmp);
}


/* --------------------------------------------------------------------------
   Convert date_t date to YYYY-MM-DD string
-------------------------------------------------------------------------- */
void date_rec2str(char *str, date_t *rec)
{
    sprintf(str, "%d-%02d-%02d", rec->year, rec->month, rec->day);
}


#ifdef _WIN32
/* --------------------------------------------------------------------------
   Is year leap?
-------------------------------------------------------------------------- */
static bool leap(short year)
{
    year += 1900;

    if ( year % 4 == 0 && ((year % 100) != 0 || (year % 400) == 0) )
        return TRUE;

    return FALSE;
}


/* --------------------------------------------------------------------------
   Convert database datetime to epoch time
-------------------------------------------------------------------------- */
static time_t win_timegm(struct tm *t)
{
    time_t epoch=0;

    static int days[2][12]={
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
    };

    int i;

    for ( i=70; i<t->tm_year; ++i )
        epoch += leap(i)?366:365;

    for ( i=0; i<t->tm_mon; ++i )
        epoch += days[leap(t->tm_year)][i];

    epoch += t->tm_mday - 1;
    epoch *= 24;

    epoch += t->tm_hour;
    epoch *= 60;

    epoch += t->tm_min;
    epoch *= 60;

    epoch += t->tm_sec;

    return epoch;
}
#endif  /* _WIN32 */


/* --------------------------------------------------------------------------
   Convert HTTP time to epoch
   Tue, 18 Oct 2016 13:13:03 GMT
   Thu, 24 Nov 2016 21:19:40 GMT
-------------------------------------------------------------------------- */
time_t time_http2epoch(const char *str)
{
    time_t  epoch;
    char    tmp[8];
struct tm   tm;

    DDBG("time_http2epoch in:  [%s]", str);

    if ( strlen(str) != 29 )
        return 0;

    /* day */

    strncpy(tmp, str+5, 2);
    tmp[2] = EOS;
    tm.tm_mday = atoi(tmp);

    /* month */

    strncpy(tmp, str+8, 3);
    tmp[3] = EOS;
    if ( 0==strcmp(tmp, "Feb") )
        tm.tm_mon = 1;
    else if ( 0==strcmp(tmp, "Mar") )
        tm.tm_mon = 2;
    else if ( 0==strcmp(tmp, "Apr") )
        tm.tm_mon = 3;
    else if ( 0==strcmp(tmp, "May") )
        tm.tm_mon = 4;
    else if ( 0==strcmp(tmp, "Jun") )
        tm.tm_mon = 5;
    else if ( 0==strcmp(tmp, "Jul") )
        tm.tm_mon = 6;
    else if ( 0==strcmp(tmp, "Aug") )
        tm.tm_mon = 7;
    else if ( 0==strcmp(tmp, "Sep") )
        tm.tm_mon = 8;
    else if ( 0==strcmp(tmp, "Oct") )
        tm.tm_mon = 9;
    else if ( 0==strcmp(tmp, "Nov") )
        tm.tm_mon = 10;
    else if ( 0==strcmp(tmp, "Dec") )
        tm.tm_mon = 11;
    else    /* January */
        tm.tm_mon = 0;

    /* year */

    strncpy(tmp, str+12, 4);
    tmp[4] = EOS;
    tm.tm_year = atoi(tmp) - 1900;

    /* hour */

    strncpy(tmp, str+17, 2);
    tmp[2] = EOS;
    tm.tm_hour = atoi(tmp);

    /* minute */

    strncpy(tmp, str+20, 2);
    tmp[2] = EOS;
    tm.tm_min = atoi(tmp);

    /* second */

    strncpy(tmp, str+23, 2);
    tmp[2] = EOS;
    tm.tm_sec = atoi(tmp);

//  DBG("%d-%02d-%02d %02d:%02d:%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

#ifdef _WIN32
    epoch = win_timegm(&tm);
#else
    epoch = timegm(&tm);
#endif

#ifdef NPP_DEBUG
    char *temp = time_epoch2http(epoch);
    DBG("time_http2epoch out: [%s]", temp);
#endif  /* NPP_DEBUG */

    return epoch;
}


/* --------------------------------------------------------------------------
   Convert db time (YYYY-MM-DD hh:mm:ss) to epoch
-------------------------------------------------------------------------- */
time_t time_db2epoch(const char *str)
{
    time_t  epoch;
    char    tmp[8];
struct tm   tm={0};

    if ( strlen(str) != 19 )
        return 0;

    /* year */

    strncpy(tmp, str, 4);
    tmp[4] = EOS;
    tm.tm_year = atoi(tmp) - 1900;

    /* month */

    strncpy(tmp, str+5, 2);
    tmp[2] = EOS;
    tm.tm_mon = atoi(tmp) - 1;

    /* day */

    strncpy(tmp, str+8, 2);
    tmp[2] = EOS;
    tm.tm_mday = atoi(tmp);

    /* hour */

    strncpy(tmp, str+11, 2);
    tmp[2] = EOS;
    tm.tm_hour = atoi(tmp);

    /* minute */

    strncpy(tmp, str+14, 2);
    tmp[2] = EOS;
    tm.tm_min = atoi(tmp);

    /* second */

    strncpy(tmp, str+17, 2);
    tmp[2] = EOS;
    tm.tm_sec = atoi(tmp);

#ifdef _WIN32
    epoch = win_timegm(&tm);
#else
    epoch = timegm(&tm);
#endif

    return epoch;
}


/* --------------------------------------------------------------------------
   Convert epoch to HTTP time
-------------------------------------------------------------------------- */
char *time_epoch2http(time_t epoch)
{
static char dst[32];

    G_ptm = gmtime(&epoch);
#ifdef _WIN32   /* Windows */
    strftime(dst, 32, "%a, %d %b %Y %H:%M:%S GMT", G_ptm);
#else
    strftime(dst, 32, "%a, %d %b %Y %T GMT", G_ptm);
#endif  /* _WIN32 */

    G_ptm = gmtime(&G_now);  /* make sure G_ptm is up to date */

    DDBG("time_epoch2http: [%s]", dst);

    return dst;
}


/* --------------------------------------------------------------------------
   Convert epoch to db time (YYYY-MM-DD HH:mm:ss)
-------------------------------------------------------------------------- */
char *time_epoch2db(time_t epoch)
{
static char dst[20];

    G_ptm = gmtime(&epoch);

    strftime(dst, 20, "%Y-%m-%d %H:%M:%S", G_ptm);

    G_ptm = gmtime(&G_now);  /* make sure G_ptm is up to date */

    DDBG("time_epoch2db: [%s]", dst);

    return dst;
}


/* --------------------------------------------------------------------------
   Format decimal amount (generic US format)
---------------------------------------------------------------------------*/
void npp_lib_fmt_dec_generic(char *dest, double in_val)
{
    char    in_val_str[64];
    int     i, j=0;
    bool    minus=FALSE;

    sprintf(in_val_str, "%0.2lf", in_val);

    if ( in_val_str[0] == '-' )   /* change to UTF-8 minus sign */
    {
        strcpy(dest, "− ");
        j = 4;
        minus = TRUE;
    }

    int len = strlen(in_val_str);

    for ( i=(j?1:0); i<len; ++i, ++j )
    {
        if ( ((!minus && i) || (minus && i>1)) && !((len-i)%3) && len-i > 3 && in_val_str[i] != ' ' && in_val_str[i-1] != ' ' && in_val_str[i-1] != '-' )
        {
            dest[j++] = ',';    /* extra character */
        }
        dest[j] = in_val_str[i];
    }

    dest[j] = EOS;
}


/* --------------------------------------------------------------------------
   Format integer amount (generic US format)
---------------------------------------------------------------------------*/
void npp_lib_fmt_int_generic(char *dest, long long in_val)
{
    char    in_val_str[256];
    int     i, j=0;
    bool    minus=FALSE;

    sprintf(in_val_str, "%lld", in_val);

    if ( in_val_str[0] == '-' )   /* change to UTF-8 minus sign */
    {
        strcpy(dest, "− ");
        j = 4;
        minus = TRUE;
    }

    int len = strlen(in_val_str);

    for ( i=(j?1:0); i<len; ++i, ++j )
    {
        if ( ((!minus && i) || (minus && i>1)) && !((len-i)%3) )
            dest[j++] = ',';
        dest[j] = in_val_str[i];
    }

    dest[j] = EOS;
}


/* --------------------------------------------------------------------------
   Convert string replacing first comma to dot
---------------------------------------------------------------------------*/
void npp_lib_normalize_float(char *str)
{
    char *comma = strchr(str, ',');
    if ( comma ) *comma = '.';
}


/* --------------------------------------------------------------------------
   SQL-escape string
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
char *npp_sql_esc(const std::string& str_)
{
    const char *str = str_.c_str();
#else
char *npp_sql_esc(const char *str)
{
#endif
static char dst[MAX_LONG_URI_VAL_LEN+1];

    npp_lib_escape_for_sql(dst, str, MAX_LONG_URI_VAL_LEN);

    return dst;
}


/* --------------------------------------------------------------------------
   HTML-escape string
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
char *npp_html_esc(const std::string& str_)
{
    const char *str = str_.c_str();
#else
char *npp_html_esc(const char *str)
{
#endif
static char dst[MAX_LONG_URI_VAL_LEN+1];

    npp_lib_escape_for_html(dst, str, MAX_LONG_URI_VAL_LEN);

    return dst;
}


/* --------------------------------------------------------------------------
   SQL-escape string respecting destination length (excluding '\0')
-------------------------------------------------------------------------- */
void sanitize_sql_old(char *dest, const char *str, int len)
{
    strncpy(dest, npp_sql_esc(str), len);
    dest[len] = EOS;

    /* cut off orphaned single backslash */

    int i=len-1;
    int bs=0;
    while ( dest[i]=='\\' && i>-1 )
    {
        ++bs;
        i--;
    }

    if ( bs % 2 )   /* odd number of trailing backslashes -- cut one */
        dest[len-1] = EOS;
}


/* --------------------------------------------------------------------------
   SQL-escape string respecting destination length (excluding '\0')
-------------------------------------------------------------------------- */
void npp_lib_escape_for_sql(char *dst, const char *str, int dst_len)
{
    int i=0, j=0;

    while ( str[i] )
    {
        if ( j > dst_len-3 )
            break;
        else if ( str[i] == '\'' )
        {
            dst[j++] = '\\';
            dst[j++] = '\'';
        }
        else if ( str[i] == '"' )
        {
            dst[j++] = '\\';
            dst[j++] = '"';
        }
        else if ( str[i] == '\\' )
        {
            dst[j++] = '\\';
            dst[j++] = '\\';
        }
        else
            dst[j++] = str[i];
        ++i;
    }

    dst[j] = EOS;
}


/* --------------------------------------------------------------------------
   HTML-escape string respecting destination length (excluding '\0')
-------------------------------------------------------------------------- */
void npp_lib_escape_for_html(char *dst, const char *str, int dst_len)
{
    int i=0, j=0;

    while ( str[i] )
    {
        if ( j > dst_len-7 )
            break;
        else if ( str[i] == '\'' )
        {
            dst[j++] = '&';
            dst[j++] = 'a';
            dst[j++] = 'p';
            dst[j++] = 'o';
            dst[j++] = 's';
            dst[j++] = ';';
        }
        else if ( str[i] == '"' )
        {
            dst[j++] = '&';
            dst[j++] = 'q';
            dst[j++] = 'u';
            dst[j++] = 'o';
            dst[j++] = 't';
            dst[j++] = ';';
        }
        else if ( str[i] == '<' )
        {
            dst[j++] = '&';
            dst[j++] = 'l';
            dst[j++] = 't';
            dst[j++] = ';';
        }
        else if ( str[i] == '>' )
        {
            dst[j++] = '&';
            dst[j++] = 'g';
            dst[j++] = 't';
            dst[j++] = ';';
        }
        else if ( str[i] == '&' )
        {
            dst[j++] = '&';
            dst[j++] = 'a';
            dst[j++] = 'm';
            dst[j++] = 'p';
            dst[j++] = ';';
        }
#ifdef HTML_ESCAPE_NEW_LINES
        else if ( str[i] == '\n' )
        {
            dst[j++] = '<';
            dst[j++] = 'b';
            dst[j++] = 'r';
            dst[j++] = '>';
        }
#endif  /* HTML_ESCAPE_NEW_LINES */
        else if ( str[i] != '\r' )
            dst[j++] = str[i];
        ++i;
    }

    dst[j] = EOS;
}


/* --------------------------------------------------------------------------
   ex unsan_noparse
   HTML un-escape string
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
char *npp_html_unesc(const std::string& str_)
{
    const char *str = str_.c_str();
#else
char *npp_html_unesc(const char *str)
{
#endif
static char dst[MAX_LONG_URI_VAL_LEN+1];
    int     i=0, j=0;

    while ( str[i] )
    {
        if ( j > MAX_LONG_URI_VAL_LEN-1 )
            break;
        else if ( i > 4
                    && str[i-5]=='&'
                    && str[i-4]=='a'
                    && str[i-3]=='p'
                    && str[i-2]=='o'
                    && str[i-1]=='s'
                    && str[i]==';' )
        {
            j -= 5;
            dst[j++] = '\'';
        }
        else if ( i > 1
                    && str[i-1]=='\\'
                    && str[i]=='\\' )
        {
            j -= 1;
            dst[j++] = '\\';
        }
        else if ( i > 4
                    && str[i-5]=='&'
                    && str[i-4]=='q'
                    && str[i-3]=='u'
                    && str[i-2]=='o'
                    && str[i-1]=='t'
                    && str[i]==';' )
        {
            j -= 5;
            dst[j++] = '"';
        }
        else if ( i > 2
                    && str[i-3]=='&'
                    && str[i-2]=='l'
                    && str[i-1]=='t'
                    && str[i]==';' )
        {
            j -= 3;
            dst[j++] = '<';
        }
        else if ( i > 2
                    && str[i-3]=='&'
                    && str[i-2]=='g'
                    && str[i-1]=='t'
                    && str[i]==';' )
        {
            j -= 3;
            dst[j++] = '>';
        }
        else if ( i > 3
                    && str[i-4]=='&'
                    && str[i-3]=='a'
                    && str[i-2]=='m'
                    && str[i-1]=='p'
                    && str[i]==';' )
        {
            j -= 4;
            dst[j++] = '&';
        }
        else if ( i > 2
                    && str[i-3]=='<'
                    && str[i-2]=='b'
                    && str[i-1]=='r'
                    && str[i]=='>' )
        {
            j -= 3;
            dst[j++] = '\n';
        }
        else
            dst[j++] = str[i];

        ++i;
    }

    dst[j] = EOS;

    return dst;
}


/* --------------------------------------------------------------------------
   Convert string to uppercase
---------------------------------------------------------------------------*/
#ifdef NPP_CPP_STRINGS
char *npp_upper(const std::string& str_)
{
    const char *str = str_.c_str();
#else
char *npp_upper(const char *str)
{
#endif
static char dst[NPP_LIB_STR_BUF];
    int     i;

    for ( i=0; str[i] && i<NPP_LIB_STR_CHECK; ++i )
    {
        if ( str[i] >= 97 && str[i] <= 122 )
            dst[i] = str[i] - 32;
        else
            dst[i] = str[i];
    }

    dst[i] = EOS;

    return dst;
}


/* --------------------------------------------------------------------------
   Strip trailing spaces from string
-------------------------------------------------------------------------- */
char *stp_right(char *str)
{
    char *p;

    for ( p = str + strlen(str) - 1;
          p >= str && (*p == ' ' || *p == '\t');
          p-- )
          *p = 0;

    return str;
}


/* --------------------------------------------------------------------------
   Return TRUE if digits only
---------------------------------------------------------------------------*/
bool strdigits(const char *src)
{
    int i;

    for ( i=0; src[i]; ++i )
    {
        if ( !isdigit(src[i]) )
            return FALSE;
    }

    return TRUE;
}


/* --------------------------------------------------------------------------
   Sleep for n miliseconds
   n must be less than 1 second (< 1000)!
-------------------------------------------------------------------------- */
void msleep(int msec)
{
    struct timeval tv;

    if ( msec < 1000 )
    {
        tv.tv_sec = 0;
        tv.tv_usec = msec*1000;
    }
    else    /* 1000 ms or more */
    {
        tv.tv_sec = msec/1000;
        tv.tv_usec = (msec-((int)(msec/1000)*1000))*1000;
    }

#ifdef NPP_DEBUG
/*    DBG("-----------------------");
    DBG("msec = %d", msec);
    DBG("tv.tv_sec = %d", tv.tv_sec);
    DBG("tv.tv_usec = %d", tv.tv_usec); */
#endif  /* NPP_DEBUG */

    select(0, NULL, NULL, NULL, &tv);
}


/* --------------------------------------------------------------------------
   Return JSON object elements
-------------------------------------------------------------------------- */
int lib_json_count(JSON *json)
{
    DDBG("lib_json_count");
    return json->cnt;
}


/* --------------------------------------------------------------------------
   Reset JSON object
-------------------------------------------------------------------------- */
void lib_json_reset(JSON *json)
{
    DDBG("lib_json_reset");
    json->cnt = 0;
    json->array = 0;
}


/* --------------------------------------------------------------------------
   Get index from JSON buffer
-------------------------------------------------------------------------- */
static int json_get_i(JSON *json, const char *name)
{
    int i;

    for ( i=0; i<json->cnt; ++i )
        if ( 0==strcmp(json->rec[i].name, name) )
            return i;

    return -1;
}


/* --------------------------------------------------------------------------
   Convert Node++ JSON format to JSON string
   Reentrant version
-------------------------------------------------------------------------- */
static void json_to_string(char *dst, JSON *json, bool array)
{
    char    *p=dst;
    int     i;

    p = stpcpy(p, array?"[":"{");

    for ( i=0; i<json->cnt; ++i )
    {
        /* key */

        if ( !array )
        {
            p = stpcpy(p, "\"");
            p = stpcpy(p, json->rec[i].name);
            p = stpcpy(p, "\":");
        }

        /* value */

        if ( json->rec[i].type == NPP_JSON_STRING )
        {
            p = stpcpy(p, "\"");
            p = stpcpy(p, json->rec[i].value);
            p = stpcpy(p, "\"");
        }
        else if ( json->rec[i].type==NPP_JSON_INTEGER || json->rec[i].type==NPP_JSON_UNSIGNED || json->rec[i].type==NPP_JSON_LONG || json->rec[i].type==NPP_JSON_FLOAT || json->rec[i].type==NPP_JSON_DOUBLE || json->rec[i].type==NPP_JSON_BOOL )
        {
            p = stpcpy(p, json->rec[i].value);
        }
        else if ( json->rec[i].type == NPP_JSON_RECORD )
        {
            intptr_t jp;
            sscanf(json->rec[i].value, "%p", (void**)&jp);
            char tmp[NPP_JSON_BUFSIZE];
            json_to_string(tmp, (JSON*)jp, FALSE);
            p = stpcpy(p, tmp);
        }
        else if ( json->rec[i].type == NPP_JSON_ARRAY )
        {
            intptr_t jp;
            sscanf(json->rec[i].value, "%p", (void**)&jp);
            char tmp[NPP_JSON_BUFSIZE];
            json_to_string(tmp, (JSON*)jp, TRUE);
            p = stpcpy(p, tmp);
        }

        if ( i < json->cnt-1 )
            p = stpcpy(p, ",");
    }

    p = stpcpy(p, array?"]":"}");

    *p = EOS;
}


/* --------------------------------------------------------------------------
   Add indent
-------------------------------------------------------------------------- */
static char *json_indent(int level)
{
static char dst[NPP_LIB_STR_BUF];
    int     i;

    dst[0] = EOS;

    for ( i=0; i<level; ++i )
        strcat(dst, NPP_JSON_PRETTY_INDENT);

    return dst;
}


/* --------------------------------------------------------------------------
   Convert Node++ JSON format to JSON string
   Reentrant version
-------------------------------------------------------------------------- */
static void json_to_string_pretty(char *dst, JSON *json, bool array, int level)
{
    char    *p=dst;
    int     i;

    p = stpcpy(p, array?"[\n":"{\n");

    for ( i=0; i<json->cnt; ++i )
    {
        p = stpcpy(p, json_indent(level));

        /* key */

        if ( !array )
        {
            p = stpcpy(p, "\"");
            p = stpcpy(p, json->rec[i].name);
            p = stpcpy(p, "\": ");
        }

        /* value */

        if ( json->rec[i].type == NPP_JSON_STRING )
        {
            p = stpcpy(p, "\"");
            p = stpcpy(p, json->rec[i].value);
            p = stpcpy(p, "\"");
        }
        else if ( json->rec[i].type==NPP_JSON_INTEGER || json->rec[i].type==NPP_JSON_UNSIGNED || json->rec[i].type==NPP_JSON_LONG || json->rec[i].type==NPP_JSON_FLOAT || json->rec[i].type==NPP_JSON_DOUBLE || json->rec[i].type==NPP_JSON_BOOL )
        {
            p = stpcpy(p, json->rec[i].value);
        }
        else if ( json->rec[i].type == NPP_JSON_RECORD )
        {
            if ( !array )
            {
                p = stpcpy(p, "\n");
                p = stpcpy(p, json_indent(level));
            }
            intptr_t jp;
            sscanf(json->rec[i].value, "%p", (void**)&jp);
            char tmp[NPP_JSON_BUFSIZE];
            json_to_string_pretty(tmp, (JSON*)jp, FALSE, level+1);
            p = stpcpy(p, tmp);
        }
        else if ( json->rec[i].type == NPP_JSON_ARRAY )
        {
            if ( !array )
            {
                p = stpcpy(p, "\n");
                p = stpcpy(p, json_indent(level));
            }
            intptr_t jp;
            sscanf(json->rec[i].value, "%p", (void**)&jp);
            char tmp[NPP_JSON_BUFSIZE];
            json_to_string_pretty(tmp, (JSON*)jp, TRUE, level+1);
            p = stpcpy(p, tmp);
        }

        if ( i < json->cnt-1 )
            p = stpcpy(p, ",");

        p = stpcpy(p, "\n");
    }

    p = stpcpy(p, json_indent(level-1));
    p = stpcpy(p, array?"]":"}");

    *p = EOS;
}


/* --------------------------------------------------------------------------
   Convert Node++ JSON format to JSON string
-------------------------------------------------------------------------- */
char *lib_json_to_string(JSON *json)
{
static char dst[NPP_JSON_BUFSIZE];

    json_to_string(dst, json, FALSE);

    return dst;
}


/* --------------------------------------------------------------------------
   Convert Node++ JSON format to JSON string
-------------------------------------------------------------------------- */
char *lib_json_to_string_pretty(JSON *json)
{
static char dst[NPP_JSON_BUFSIZE];

    json_to_string_pretty(dst, json, FALSE, 1);

    return dst;
}


/* --------------------------------------------------------------------------
   Find matching closing bracket in JSON string
-------------------------------------------------------------------------- */
static char *get_json_closing_bracket(const char *src)
{
    int     i=1, subs=0;
    bool    in_quotes=0, escape=0;

#ifdef NPP_DEBUG
    int len = strlen(src);
    DBG("get_json_closing_bracket: len = %d", len);
//    npp_log_long(src, len, "get_json_closing_bracket");
#endif  /* NPP_DEBUG */

    while ( src[i] )
    {
        if ( escape )
        {
            escape = 0;
        }
        else if ( src[i]=='\\' && in_quotes )
        {
            escape = 1;
        }
        else if ( src[i]=='"' )
        {
            if ( in_quotes )
                in_quotes = 0;
            else
                in_quotes = 1;
        }
        else if ( src[i]=='{' && !in_quotes )
        {
            ++subs;
        }
        else if ( src[i]=='}' && !in_quotes )
        {
            if ( subs<1 )
                return (char*)(src+i);
            else
                subs--;
        }

        ++i;
    }

    DDBG("get_json_closing_bracket: not found");

    return NULL;
}


/* --------------------------------------------------------------------------
   Find matching closing square bracket in JSON string
-------------------------------------------------------------------------- */
static char *get_json_closing_square_bracket(const char *src)
{
    int     i=1, subs=0;
    bool    in_quotes=0, escape=0;

#ifdef NPP_DEBUG
    int len = strlen(src);
    DBG("get_json_closing_square_bracket: len = %d", len);
//    npp_log_long(src, len, "get_json_closing_square_bracket");
#endif  /* NPP_DEBUG */

    while ( src[i] )
    {
//        DDBG("%c", src[i]);

        if ( escape )
        {
            escape = 0;
        }
        else if ( src[i]=='\\' && in_quotes )
        {
            escape = 1;
        }
        else if ( src[i]=='"' )
        {
//            DDBG("in_quotes = %d", in_quotes);

            if ( in_quotes )
                in_quotes = 0;
            else
                in_quotes = 1;

//            DDBG("in_quotes switched to %d", in_quotes);
        }
        else if ( src[i]=='[' && !in_quotes )
        {
            ++subs;
//            DDBG("subs+1 = %d", subs);
        }
        else if ( src[i]==']' && !in_quotes )
        {
            if ( subs<1 )
                return (char*)(src+i);
            else
                subs--;

//            DDBG("subs-1 = %d", subs);
        }

        ++i;
    }

    DDBG("get_json_closing_square_bracket: not found");

    return NULL;
}


/* --------------------------------------------------------------------------
   Convert JSON string to Node++ JSON format
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
bool lib_json_from_string(JSON *json, const std::string& src_, size_t len, unsigned level)
{
    const char *src = src_.c_str();
#else
bool lib_json_from_string(JSON *json, const char *src, size_t len, unsigned level)
{
#endif
    unsigned i=0, j=0;
    char    key[NPP_JSON_KEY_LEN+1];
    char    value[NPP_JSON_STR_LEN+1];
    int     index;
    char    now_key=0, now_value=0, inside_array=0, type=NPP_JSON_STRING;
    float   flo_value;
    double  dbl_value;

static JSON json_pool[NPP_JSON_POOL_SIZE*NPP_JSON_MAX_LEVELS];
static int  json_pool_cnt[NPP_JSON_MAX_LEVELS]={0};

    if ( len == 0 ) len = strlen(src);

    if ( level == 0 )
    {
        lib_json_reset(json);

        while ( i<len && src[i] != '{' ) ++i;   /* skip junk if there's any */

        if ( src[i] != '{' )    /* no opening bracket */
        {
            WAR("JSON syntax error -- no opening curly bracket");
            return FALSE;
        }

        ++i;    /* skip '{' */
    }
    else if ( src[i]=='{' )     /* record */
    {
        ++i;    /* skip '{' */
    }
    else if ( src[i]=='[' )     /* array */
    {
        inside_array = 1;
        ++i;    /* skip '[' */
        index = -1;
    }

#ifdef NPP_DEBUG
static char tmp[NPP_JSON_BUFSIZE];
    strncpy(tmp, src+i, len-i);
    tmp[len-i] = EOS;
    char debug[64];
    sprintf(debug, "lib_json_from_string level %u", level);
    npp_log_long(tmp, len, debug);
    if ( inside_array ) DBG("inside_array");
#endif  /* NPP_DEBUG */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"

    for ( i; i<len; ++i )
    {

#pragma GCC diagnostic pop

        if ( !now_key && !now_value )
        {
            while ( i<len && (src[i]==' ' || src[i]=='\t' || src[i]=='\r' || src[i]=='\n') ) ++i;

            if ( !inside_array && src[i]=='"' )  /* start of key */
            {
                now_key = 1;
                j = 0;
                ++i;    /* skip '"' */
            }
        }

        if ( (now_key && src[i]=='"') || (inside_array && !now_value && (index==-1 || src[i]==',')) )      /* end of key */
        {
#ifdef NPP_DEBUG
            if ( now_key && src[i]=='"' )
                DBG("second if because of now_key");
            else
                DBG("second if because of inside_array");
#endif  /* NPP_DEBUG */
            if ( inside_array )
            {
                if ( src[i]==',' ) ++i;    /* skip ',' */

                ++index;

                DDBG("inside_array, starting new value, index = %d", index);
            }
            else
            {
                key[j] = EOS;

                DDBG("key [%s]", key);

                now_key = 0;

                ++i;    /* skip '"' */

                while ( i<len && src[i]!=':' ) ++i;

                if ( src[i] != ':' )
                {
                    WAR("JSON syntax error -- no colon after name");
                    return FALSE;
                }

                ++i;    /* skip ':' */
            }

            while ( i<len && (src[i]==' ' || src[i]=='\t' || src[i]=='\r' || src[i]=='\n') ) ++i;

            if ( i==len )
            {
                WAR("JSON syntax error -- expected value");
                return FALSE;
            }

            /* value starts here --------------------------------------------------- */

            if ( src[i]=='"' )    /* NPP_JSON_STRING */
            {
                DDBG("NPP_JSON_STRING");

                type = NPP_JSON_STRING;

                now_value = 1;
                j = 0;
            }
            else if ( src[i]=='{' )     /* NPP_JSON_RECORD */
            {
                DDBG("NPP_JSON_RECORD");

                type = NPP_JSON_RECORD;

                if ( level < NPP_JSON_MAX_LEVELS-1 )
                {
                    if ( json_pool_cnt[level] >= NPP_JSON_POOL_SIZE ) json_pool_cnt[level] = 0;   /* overwrite previous ones */

                    int pool_idx = NPP_JSON_POOL_SIZE*level + json_pool_cnt[level];
                    lib_json_reset(&json_pool[pool_idx]);
                    /* save the pointer first as a parent record */
                    if ( inside_array )
                        lib_json_add_record(json, NULL, &json_pool[pool_idx], FALSE, index);
                    else
                        lib_json_add_record(json, key, &json_pool[pool_idx], FALSE, 0);
                    /* fill in the destination (children) */
                    char *closing;
                    if ( (closing=get_json_closing_bracket(src+i)) )
                    {
//                        DBG("closing [%s], len=%d", closing, closing-(src+i));
                        if ( !lib_json_from_string(&json_pool[pool_idx], src+i, closing-(src+i)+1, level+1) )
                            return FALSE;
                        ++json_pool_cnt[level];
                        i += closing-(src+i);
//                        DBG("after closing record bracket [%s]", src+i);
                    }
                    else    /* syntax error */
                    {
                        WAR("No closing bracket in JSON record");
                        return FALSE;
                    }
                }
            }
            else if ( src[i]=='[' )     /* NPP_JSON_ARRAY */
            {
                DDBG("NPP_JSON_ARRAY");

                type = NPP_JSON_ARRAY;

                if ( level < NPP_JSON_MAX_LEVELS-1 )
                {
                    if ( json_pool_cnt[level] >= NPP_JSON_POOL_SIZE ) json_pool_cnt[level] = 0;   /* overwrite previous ones */

                    int pool_idx = NPP_JSON_POOL_SIZE*level + json_pool_cnt[level];
                    lib_json_reset(&json_pool[pool_idx]);
                    /* save the pointer first as a parent record */
                    if ( inside_array )
                        lib_json_add_record(json, NULL, &json_pool[pool_idx], TRUE, index);
                    else
                        lib_json_add_record(json, key, &json_pool[pool_idx], TRUE, 0);
                    /* fill in the destination (children) */
                    char *closing;
                    if ( (closing=get_json_closing_square_bracket(src+i)) )
                    {
//                        DBG("closing [%s], len=%d", closing, closing-(src+i));
                        if ( !lib_json_from_string(&json_pool[pool_idx], src+i, closing-(src+i)+1, level+1) )
                            return FALSE;
                        ++json_pool_cnt[level];
                        i += closing-(src+i);
//                        DBG("after closing array bracket [%s]", src+i);
                    }
                    else    /* syntax error */
                    {
                        WAR("No closing square bracket in JSON array");
                        return FALSE;
                    }
                }
            }
            else    /* number */
            {
                DDBG("NPP_JSON_INTEGER || NPP_JSON_UNSIGNED || NPP_JSON_LONG || NPP_JSON_FLOAT || NPP_JSON_DOUBLE || NPP_JSON_BOOL");

                type = NPP_JSON_INTEGER;    /* we're not sure yet but need to mark it's definitely not STRING */

                i--;

                now_value = 1;
                j = 0;
            }
        }
        else if ( now_value
                    && ((type==NPP_JSON_STRING && src[i]=='"' && src[i-1]!='\\')
                            || (type!=NPP_JSON_STRING && (src[i]==',' || src[i]=='}' || src[i]==']' || src[i]=='\r' || src[i]=='\n'))) )     /* end of value */
        {
            value[j] = EOS;

            DDBG("value [%s]", value);

            if ( type==NPP_JSON_STRING ) ++i;   /* skip closing '"' */

            /* src[i] should now be at ',' */

            if ( inside_array )
            {
                if ( type==NPP_JSON_STRING )
                    lib_json_add_str(json, NULL, value, index);
                else if ( value[0]=='t' )
                    lib_json_add_bool(json, NULL, 1, index);
                else if ( value[0]=='f' )
                    lib_json_add_bool(json, NULL, 0, index);
                else if ( strchr(value, '.') )
                {
                    if ( strlen(value) <= NPP_JSON_MAX_FLOAT_LEN )
                    {
                        sscanf(value, "%f", &flo_value);
                        lib_json_add_float(json, NULL, flo_value, index);
                    }
                    else    /* double */
                    {
                        sscanf(value, "%lf", &dbl_value);
                        lib_json_add_double(json, NULL, dbl_value, index);
                    }
                }
#ifdef _WIN32   /* sizeof(long) == sizeof(int) */
                else if ( value[0] == '-' || strtoul(value, NULL, 10) < INT_MAX )
                {
                    int num_val;
                    sscanf(value, "%d", &num_val);
                    lib_json_add_int(json, NULL, num_val, index);
                }
                else    /* unsigned */
                {
                    unsigned num_val;
                    sscanf(value, "%u", &num_val);
                    lib_json_add_uint(json, NULL, num_val, index);
                }
#else   /* Linux */
                else    /* long */
                {
                    long num_val;
                    sscanf(value, "%ld", &num_val);
                    lib_json_add_long(json, NULL, num_val, index);
                }
#endif  /* _WIN32 */
            }
            else    /* not an array */
            {
                if ( type==NPP_JSON_STRING )
                    lib_json_add_str(json, key, value, -1);
                else if ( value[0]=='t' )
                    lib_json_add_bool(json, key, 1, -1);
                else if ( value[0]=='f' )
                    lib_json_add_bool(json, key, 0, -1);
                else if ( strchr(value, '.') )
                {
                    if ( strlen(value) <= NPP_JSON_MAX_FLOAT_LEN )
                    {
                        sscanf(value, "%f", &flo_value);
                        lib_json_add_float(json, key, flo_value, -1);
                    }
                    else    /* double */
                    {
                        sscanf(value, "%lf", &dbl_value);
                        lib_json_add_double(json, key, dbl_value, -1);
                    }
                }
#ifdef _WIN32   /* sizeof(long) == sizeof(int) */
                else if ( value[0] == '-' || strtoul(value, NULL, 10) < INT_MAX )
                {
                    int num_val;
                    sscanf(value, "%d", &num_val);
                    lib_json_add_int(json, key, num_val, -1);
                }
                else    /* unsigned */
                {
                    unsigned num_val;
                    sscanf(value, "%u", &num_val);
                    lib_json_add_uint(json, key, num_val, -1);
                }
#else   /* Linux */
                else    /* long */
                {
                    long num_val;
                    sscanf(value, "%ld", &num_val);
                    lib_json_add_long(json, key, num_val, -1);
                }
#endif  /* _WIN32 */
            }

            now_value = 0;

            if ( src[i]==',' ) i--;     /* we need it to detect the next array element */
        }
        else if ( now_key )
        {
            if ( j < NPP_JSON_KEY_LEN )
                key[j++] = src[i];
        }
        else if ( now_value )
        {
            if ( j < NPP_JSON_STR_LEN )
                value[j++] = src[i];
        }
    }

    return TRUE;
}


/* --------------------------------------------------------------------------
   Log JSON buffer
-------------------------------------------------------------------------- */
void lib_json_log_dbg(JSON *json, const char *name)
{
    int     i;
    char    type[64];

    DBG_LINE;

    if ( name )
        DBG("%s:", name);
    else
        DBG("JSON record:");

    for ( i=0; i<json->cnt; ++i )
    {
        if ( json->rec[i].type == NPP_JSON_STRING )
            strcpy(type, "NPP_JSON_STRING");
        else if ( json->rec[i].type == NPP_JSON_INTEGER )
            strcpy(type, "NPP_JSON_INTEGER");
        else if ( json->rec[i].type == NPP_JSON_UNSIGNED )
            strcpy(type, "NPP_JSON_UNSIGNED");
        else if ( json->rec[i].type == NPP_JSON_LONG )
            strcpy(type, "NPP_JSON_LONG");
        else if ( json->rec[i].type == NPP_JSON_FLOAT )
            strcpy(type, "NPP_JSON_FLOAT");
        else if ( json->rec[i].type == NPP_JSON_DOUBLE )
            strcpy(type, "NPP_JSON_DOUBLE");
        else if ( json->rec[i].type == NPP_JSON_BOOL )
            strcpy(type, "NPP_JSON_BOOL");
        else if ( json->rec[i].type == NPP_JSON_RECORD )
            strcpy(type, "NPP_JSON_RECORD");
        else if ( json->rec[i].type == NPP_JSON_ARRAY )
            strcpy(type, "NPP_JSON_ARRAY");
        else
        {
            sprintf(type, "Unknown type! (%d)", json->rec[i].type);
            break;
        }

        DBG("%d %s [%s] %s", i, json->array?"":json->rec[i].name, json->rec[i].value, type);
    }

    DBG_LINE;
}


/* --------------------------------------------------------------------------
   Log JSON buffer
-------------------------------------------------------------------------- */
void lib_json_log_inf(JSON *json, const char *name)
{
    int     i;
    char    type[64];

    INF_LINE;

    if ( name )
        INF("%s:", name);
    else
        INF("JSON record:");

    for ( i=0; i<json->cnt; ++i )
    {
        if ( json->rec[i].type == NPP_JSON_STRING )
            strcpy(type, "NPP_JSON_STRING");
        else if ( json->rec[i].type == NPP_JSON_INTEGER )
            strcpy(type, "NPP_JSON_INTEGER");
        else if ( json->rec[i].type == NPP_JSON_UNSIGNED )
            strcpy(type, "NPP_JSON_UNSIGNED");
        else if ( json->rec[i].type == NPP_JSON_LONG )
            strcpy(type, "NPP_JSON_LONG");
        else if ( json->rec[i].type == NPP_JSON_FLOAT )
            strcpy(type, "NPP_JSON_FLOAT");
        else if ( json->rec[i].type == NPP_JSON_DOUBLE )
            strcpy(type, "NPP_JSON_DOUBLE");
        else if ( json->rec[i].type == NPP_JSON_BOOL )
            strcpy(type, "NPP_JSON_BOOL");
        else if ( json->rec[i].type == NPP_JSON_RECORD )
            strcpy(type, "NPP_JSON_RECORD");
        else if ( json->rec[i].type == NPP_JSON_ARRAY )
            strcpy(type, "NPP_JSON_ARRAY");
        else
        {
            sprintf(type, "Unknown type! (%d)", json->rec[i].type);
            break;
        }

        INF("%d %s [%s] %s", i, json->array?"":json->rec[i].name, json->rec[i].value, type);
    }

    INF_LINE;
}


/* --------------------------------------------------------------------------
   Add/set value to a JSON buffer
-------------------------------------------------------------------------- */
static int json_add_elem(JSON *json, const char *name, int i)
{
    if ( name )
    {
        i = json_get_i(json, name);

        if ( i==-1 )    /* not present -- append new */
        {
            if ( json->cnt >= NPP_JSON_MAX_ELEMS ) return -1;
            i = json->cnt++;
            COPY(json->rec[i].name, name, NPP_JSON_KEY_LEN);
            json->array = FALSE;
        }
    }
    else    /* array */
    {
        if ( i >= NPP_JSON_MAX_ELEMS-1 ) return -1;
        json->array = TRUE;
        if ( json->cnt < i+1 ) json->cnt = i + 1;
    }

    return i;
}


/* --------------------------------------------------------------------------
   Add/set value to a JSON buffer
-------------------------------------------------------------------------- */
bool lib_json_add_str(JSON *json, const char *name, const char *value, int i)
{
    if ( (i=json_add_elem(json, name, i)) == -1 )
        return FALSE;

    COPY(json->rec[i].value, value, NPP_JSON_STR_LEN);
    json->rec[i].type = NPP_JSON_STRING;

    return TRUE;
}


/* --------------------------------------------------------------------------
   Add/set value to a JSON buffer
-------------------------------------------------------------------------- */
bool lib_json_add_int(JSON *json, const char *name, int value, int i)
{
    if ( (i=json_add_elem(json, name, i)) == -1 )
        return FALSE;

    sprintf(json->rec[i].value, "%d", value);
    json->rec[i].type = NPP_JSON_INTEGER;

    return TRUE;
}


/* --------------------------------------------------------------------------
   Add/set value to a JSON buffer
-------------------------------------------------------------------------- */
bool lib_json_add_uint(JSON *json, const char *name, unsigned value, int i)
{
    if ( (i=json_add_elem(json, name, i)) == -1 )
        return FALSE;

    sprintf(json->rec[i].value, "%u", value);
    json->rec[i].type = NPP_JSON_UNSIGNED;

    return TRUE;
}


/* --------------------------------------------------------------------------
   Add/set value to a JSON buffer
-------------------------------------------------------------------------- */
bool lib_json_add_long(JSON *json, const char *name, long value, int i)
{
    if ( (i=json_add_elem(json, name, i)) == -1 )
        return FALSE;

    sprintf(json->rec[i].value, "%ld", value);
    json->rec[i].type = NPP_JSON_LONG;

    return TRUE;
}


/* --------------------------------------------------------------------------
   Add/set value to a JSON buffer
-------------------------------------------------------------------------- */
bool lib_json_add_float(JSON *json, const char *name, float value, int i)
{
    if ( (i=json_add_elem(json, name, i)) == -1 )
        return FALSE;

    sprintf(json->rec[i].value, "%f", value);
    json->rec[i].type = NPP_JSON_FLOAT;

    return TRUE;
}


/* --------------------------------------------------------------------------
   Add/set value to a JSON buffer
-------------------------------------------------------------------------- */
bool lib_json_add_double(JSON *json, const char *name, double value, int i)
{
    if ( (i=json_add_elem(json, name, i)) == -1 )
        return FALSE;

    sprintf(json->rec[i].value, "%lf", value);
    json->rec[i].type = NPP_JSON_DOUBLE;

    return TRUE;
}


/* --------------------------------------------------------------------------
   Add/set value to a JSON buffer
-------------------------------------------------------------------------- */
bool lib_json_add_bool(JSON *json, const char *name, bool value, int i)
{
    if ( (i=json_add_elem(json, name, i)) == -1 )
        return FALSE;

    if ( value )
        strcpy(json->rec[i].value, "true");
    else
        strcpy(json->rec[i].value, "false");

    json->rec[i].type = NPP_JSON_BOOL;

    return TRUE;
}


/* --------------------------------------------------------------------------
   Insert or update value (address) in JSON buffer
-------------------------------------------------------------------------- */
bool lib_json_add_record(JSON *json, const char *name, JSON *json_sub, bool is_array, int i)
{
    DDBG("lib_json_add_record (%s)", is_array?"ARRAY":"RECORD");

    if ( name )   /* named record */
    {
        DDBG("named record [%s]", name);

        i = json_get_i(json, name);

        if ( i==-1 )    /* not present -- append new */
        {
            if ( json->cnt >= NPP_JSON_MAX_ELEMS ) return FALSE;
            i = json->cnt;
            ++json->cnt;
            COPY(json->rec[i].name, name, NPP_JSON_KEY_LEN);
            json->array = FALSE;
        }
    }
    else    /* array element */
    {
        DDBG("array element %d", i);

        if ( i >= NPP_JSON_MAX_ELEMS-1 ) return FALSE;
        json->array = TRUE;
        if ( json->cnt < i+1 ) json->cnt = i + 1;
    }

    /* store sub-record address as a text in value */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"

    sprintf(json->rec[i].value, "%p", json_sub);

    json->rec[i].type = is_array?NPP_JSON_ARRAY:NPP_JSON_RECORD;

#pragma GCC diagnostic pop

    return TRUE;
}


/* --------------------------------------------------------------------------
   Check value presence in JSON buffer
-------------------------------------------------------------------------- */
bool lib_json_present(JSON *json, const char *name)
{
    int i;

    for ( i=0; i<json->cnt; ++i )
    {
        if ( 0==strcmp(json->rec[i].name, name) )
            return TRUE;
    }

    return FALSE;
}


/* --------------------------------------------------------------------------
   Get value from JSON buffer
-------------------------------------------------------------------------- */
char *lib_json_get_str(JSON *json, const char *name, int i)
{
static char dst[NPP_JSON_STR_LEN+1];

    if ( !name )    /* array elem */
    {
        if ( i >= json->cnt )
        {
            WAR("lib_json_get_str index (%d) out of bound (max = %d)", i, json->cnt-1);
            dst[0] = EOS;
            return dst;
        }

        if ( json->rec[i].type==NPP_JSON_STRING || json->rec[i].type==NPP_JSON_INTEGER || json->rec[i].type==NPP_JSON_UNSIGNED || json->rec[i].type==NPP_JSON_FLOAT || json->rec[i].type==NPP_JSON_DOUBLE || json->rec[i].type==NPP_JSON_BOOL )
        {
            strcpy(dst, json->rec[i].value);
            return dst;
        }
        else    /* types don't match */
        {
            dst[0] = EOS;
            return dst;   /* types don't match or couldn't convert */
        }
    }

    for ( i=0; i<json->cnt; ++i )
    {
        if ( 0==strcmp(json->rec[i].name, name) )
        {
            if ( json->rec[i].type==NPP_JSON_STRING || json->rec[i].type==NPP_JSON_INTEGER || json->rec[i].type==NPP_JSON_UNSIGNED || json->rec[i].type==NPP_JSON_FLOAT || json->rec[i].type==NPP_JSON_DOUBLE || json->rec[i].type==NPP_JSON_BOOL )
            {
                strcpy(dst, json->rec[i].value);
                return dst;
            }

            dst[0] = EOS;
            return dst;   /* types don't match or couldn't convert */
        }
    }

    dst[0] = EOS;
    return dst;   /* no such field */
}


/* --------------------------------------------------------------------------
   Get value from JSON buffer
-------------------------------------------------------------------------- */
int lib_json_get_int(JSON *json, const char *name, int i)
{
    int value;

    if ( !name )    /* array elem */
    {
        if ( i >= json->cnt )
        {
            WAR("lib_json_get_int index (%d) out of bound (max = %d)", i, json->cnt-1);
            return 0;
        }

        sscanf(json->rec[i].value, "%d", &value);
        return value;
    }

    for ( i=0; i<json->cnt; ++i )
    {
        if ( 0==strcmp(json->rec[i].name, name) )
        {
            sscanf(json->rec[i].value, "%d", &value);
            return value;
        }
    }

    return 0;   /* no such field */
}


/* --------------------------------------------------------------------------
   Get value from JSON buffer
-------------------------------------------------------------------------- */
unsigned lib_json_get_uint(JSON *json, const char *name, int i)
{
    unsigned value;

    if ( !name )    /* array elem */
    {
        if ( i >= json->cnt )
        {
            WAR("lib_json_get_uint index (%d) out of bound (max = %d)", i, json->cnt-1);
            return 0;
        }

        sscanf(json->rec[i].value, "%u", &value);
        return value;
    }

    for ( i=0; i<json->cnt; ++i )
    {
        if ( 0==strcmp(json->rec[i].name, name) )
        {
            sscanf(json->rec[i].value, "%u", &value);
            return value;
        }
    }

    return 0;   /* no such field */
}


/* --------------------------------------------------------------------------
   Get value from JSON buffer
-------------------------------------------------------------------------- */
long lib_json_get_long(JSON *json, const char *name, int i)
{
    long value;

    if ( !name )    /* array elem */
    {
        if ( i >= json->cnt )
        {
            WAR("lib_json_get_long index (%d) out of bound (max = %d)", i, json->cnt-1);
            return 0;
        }

        sscanf(json->rec[i].value, "%ld", &value);
        return value;
    }

    for ( i=0; i<json->cnt; ++i )
    {
        if ( 0==strcmp(json->rec[i].name, name) )
        {
            sscanf(json->rec[i].value, "%ld", &value);
            return value;
        }
    }

    return 0;   /* no such field */
}


/* --------------------------------------------------------------------------
   Get value from JSON buffer
-------------------------------------------------------------------------- */
float lib_json_get_float(JSON *json, const char *name, int i)
{
    float value;

    if ( !name )    /* array elem */
    {
        if ( i >= json->cnt )
        {
            WAR("lib_json_get_float index (%d) out of bound (max = %d)", i, json->cnt-1);
            return 0;
        }

        sscanf(json->rec[i].value, "%f", &value);
        return value;
    }

    for ( i=0; i<json->cnt; ++i )
    {
        if ( 0==strcmp(json->rec[i].name, name) )
        {
            sscanf(json->rec[i].value, "%f", &value);
            return value;
        }
    }

    return 0;   /* no such field */
}


/* --------------------------------------------------------------------------
   Get value from JSON buffer
-------------------------------------------------------------------------- */
double lib_json_get_double(JSON *json, const char *name, int i)
{
    double value;

    if ( !name )    /* array elem */
    {
        if ( i >= json->cnt )
        {
            WAR("lib_json_get_double index (%d) out of bound (max = %d)", i, json->cnt-1);
            return 0;
        }

        sscanf(json->rec[i].value, "%lf", &value);
        return value;
    }

    for ( i=0; i<json->cnt; ++i )
    {
        if ( 0==strcmp(json->rec[i].name, name) )
        {
            sscanf(json->rec[i].value, "%lf", &value);
            return value;
        }
    }

    return 0;   /* no such field */
}


/* --------------------------------------------------------------------------
   Get value from JSON buffer
-------------------------------------------------------------------------- */
bool lib_json_get_bool(JSON *json, const char *name, int i)
{
    if ( !name )    /* array elem */
    {
        if ( i >= json->cnt )
        {
            WAR("lib_json_get_bool index (%d) out of bound (max = %d)", i, json->cnt-1);
            return FALSE;
        }

        if ( NPP_IS_THIS_TRUE(json->rec[i].value[0]) )
            return TRUE;
        else
            return FALSE;
    }

    for ( i=0; i<json->cnt; ++i )
    {
        if ( 0==strcmp(json->rec[i].name, name) )
        {
            if ( NPP_IS_THIS_TRUE(json->rec[i].value[0]) )
                return TRUE;
            else
                return FALSE;
        }
    }

    return FALSE;   /* no such field */
}


/* --------------------------------------------------------------------------
   Get (copy) value from JSON buffer
   How to change it to returning pointer without confusing beginners?
   It would be better performing without copying all the fields
-------------------------------------------------------------------------- */
bool lib_json_get_record(JSON *json, const char *name, JSON *json_sub, int i)
{
    DBG("lib_json_get_record by %s", name?"name":"index");

    if ( !name )    /* array elem */
    {
        if ( i >= json->cnt )
        {
            WAR("lib_json_get_record index (%d) out of bound (max = %d)", i, json->cnt-1);
            return FALSE;
        }

        DDBG("index = %d", i);

        if ( json->rec[i].type == NPP_JSON_RECORD || json->rec[i].type == NPP_JSON_ARRAY )
        {
            intptr_t jp;
            sscanf(json->rec[i].value, "%p", (void**)&jp);
            memcpy(json_sub, (JSON*)jp, sizeof(JSON));
            return TRUE;
        }
        else
        {
            return FALSE;   /* types don't match or couldn't convert */
        }
    }

    DDBG("name [%s]", name);

    for ( i=0; i<json->cnt; ++i )
    {
        if ( 0==strcmp(json->rec[i].name, name) )
        {
//            DBG("lib_json_get_record, found [%s]", name);
            if ( json->rec[i].type == NPP_JSON_RECORD || json->rec[i].type == NPP_JSON_ARRAY )
            {
                intptr_t jp;
                sscanf(json->rec[i].value, "%p", (void**)&jp);
                memcpy(json_sub, (JSON*)jp, sizeof(JSON));
                return TRUE;
            }

//            DBG("lib_json_get_record, types of [%s] don't match", name);
            return FALSE;   /* types don't match or couldn't convert */
        }
    }

//    DBG("lib_json_get_record, [%s] not found", name);
    return FALSE;   /* no such field */
}


/* --------------------------------------------------------------------------
   Check system's endianness
-------------------------------------------------------------------------- */
static bool endianness32()
{
    union {
        intptr_t p;
        char c[4];
    } test;

    memset(&test, 0, sizeof(test));

    test.p = 1;

    if ( test.c[3] && !test.c[2] && !test.c[1] && !test.c[0] )
    {
        INF("32-bit Big Endian");
        return ENDIANNESS_BIG;
    }

    INF("32-bit Little Endian");

    return ENDIANNESS_LITTLE;
}


/* --------------------------------------------------------------------------
   Check system's endianness
-------------------------------------------------------------------------- */
static bool endianness64()
{
    union {
        intptr_t p;
        char c[8];
    } test;

    memset(&test, 0, sizeof(test));

    test.p = 1;

    if ( test.c[7] && !test.c[3] && !test.c[2] && !test.c[1] && !test.c[0] )
    {
        INF("64-bit Big Endian");
        return ENDIANNESS_BIG;
    }

    INF("64-bit Little Endian");

    return ENDIANNESS_LITTLE;
}


/* --------------------------------------------------------------------------
   Check system's endianness
-------------------------------------------------------------------------- */
void npp_get_byteorder()
{
    if ( sizeof(intptr_t) == 4 )
        G_endianness = endianness32();
    else if ( sizeof(intptr_t) == 8 )
        G_endianness = endianness64();
}


/* --------------------------------------------------------------------------
   Send an email
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
bool npp_email(const std::string& to_, const std::string& subject_, const std::string& message_)
{
    const char *to = to_.c_str();
    const char *subject = subject_.c_str();
    const char *message = message_.c_str();
#else
bool npp_email(const char *to, const char *subject, const char *message)
{
#endif
    DBG("Sending email to [%s], subject [%s]", to, subject);

#ifndef _WIN32
    char    sender[512];
    char    command[1024];

//#ifndef NPP_SVC   /* web server mode */

//    sprintf(sender, "%s <noreply@%s>", G_connections[ci].app_name, G_connections[ci].host);

    /* happens when using non-standard port */

//    char    *colon;
//    if ( G_test && (colon=strchr(sender, ':')) )
//    {
//        *colon = '>';
//        *(++colon) = EOS;
//        DBG("sender truncated to [%s]", sender);
//    }
//#else
    sprintf(sender, "%s <%s@%s>", NPP_APP_NAME, NPP_EMAIL_FROM_USER, NPP_APP_DOMAIN);
//#endif  /* NPP_SVC */

    sprintf(command, "/usr/lib/sendmail -t -f \"%s\"", sender);

    FILE *mailpipe = popen(command, "w");

    if ( mailpipe == NULL )
    {
        ERR("Failed to invoke sendmail");
        return FALSE;
    }
    else
    {
        fprintf(mailpipe, "From: %s\n", sender);
        fprintf(mailpipe, "To: %s\n", to);
        fprintf(mailpipe, "Subject: %s\n", subject);
        fprintf(mailpipe, "Content-Type: text/plain; charset=\"utf-8\"\n\n");
        fwrite(message, strlen(message), 1, mailpipe);
        fwrite("\n.\n", 3, 1, mailpipe);
        pclose(mailpipe);
    }

    return TRUE;

#else   /* Windows */

    WAR("There's no email service for Windows");
    return TRUE;

#endif  /* _WIN32 */
}


/* --------------------------------------------------------------------------
   Send an email with attachement
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
bool npp_email_attach(const std::string& to_, const std::string& subject_, const std::string& message_, const std::string& att_name_, const unsigned char *att_data, int att_data_len)
{
    const char *to = to_.c_str();
    const char *subject = subject_.c_str();
    const char *message = message_.c_str();
    const char *att_name = att_name_.c_str();
#else
bool npp_email_attach(const char *to, const char *subject, const char *message, const char *att_name, const unsigned char *att_data, int att_data_len)
{
#endif
    DBG("Sending email to [%s], subject [%s], with attachement [%s]", to, subject, att_name);

#define NPP_BOUNDARY "nppbndGq7ehJxtz"

#ifndef _WIN32
    char    sender[512];
    char    command[1024];

    sprintf(sender, "%s <%s@%s>", NPP_APP_NAME, NPP_EMAIL_FROM_USER, NPP_APP_DOMAIN);

    sprintf(command, "/usr/lib/sendmail -t -f \"%s\"", sender);

    FILE *mailpipe = popen(command, "w");

    if ( mailpipe == NULL )
    {
        ERR("Failed to invoke sendmail");
        return FALSE;
    }
    else
    {
        fprintf(mailpipe, "From: %s\n", sender);
        fprintf(mailpipe, "To: %s\n", to);
        fprintf(mailpipe, "Subject: %s\n", subject);
        fprintf(mailpipe, "Content-Type: multipart/mixed; boundary=%s\n", NPP_BOUNDARY);
        fprintf(mailpipe, "\n");

        /* message */

        fprintf(mailpipe, "--%s\n", NPP_BOUNDARY);

        fprintf(mailpipe, "Content-Type: text/plain; charset=\"utf-8\"\n");
//        fprintf(mailpipe, "Content-Transfer-Encoding: quoted-printable\n");
        fprintf(mailpipe, "Content-Disposition: inline\n");
        fprintf(mailpipe, "\n");


/*        char *qpm;
        int qpm_len = strlen(message) * 4;

        if ( !(qpm=(char*)malloc(qpm_len)) )
        {
            ERR("Couldn't allocate %d bytes for qpm", qpm_len);
            return FALSE;
        }

        qp(qpm, message);

        DBG("qpm [%s]", qpm); */

        fwrite(message, strlen(message), 1, mailpipe);
//        fwrite(qpm, 1, strlen(qpm), mailpipe);
//        free(qpm);
        fprintf(mailpipe, "\n\n");


        /* attachement */

        fprintf(mailpipe, "--%s\n", NPP_BOUNDARY);

        fprintf(mailpipe, "Content-Type: application\n");
        fprintf(mailpipe, "Content-Transfer-Encoding: base64\n");
        fprintf(mailpipe, "Content-Disposition: attachment; filename=\"%s\"\n", att_name);
        fprintf(mailpipe, "\n");

        char *b64data;
        int b64data_len = ((4 * att_data_len / 3) + 3) & ~3;

        DBG("Predicted b64data_len = %d", b64data_len);

        if ( !(b64data=(char*)malloc(b64data_len+16)) )   /* just in case, to be verified */
        {
            ERR("Couldn't allocate %d bytes for b64data", b64data_len+16);
            return FALSE;
        }

        npp_b64_encode(b64data, att_data, att_data_len);
        b64data_len = strlen(b64data);

        DBG("     Real b64data_len = %d", b64data_len);

        fwrite(b64data, b64data_len, 1, mailpipe);

        free(b64data);

        /* finish */

        fprintf(mailpipe, "\n\n--%s--\n", NPP_BOUNDARY);

        pclose(mailpipe);
    }

    return TRUE;

#else   /* Windows */

    WAR("There's no email service for Windows");
    return TRUE;

#endif  /* _WIN32 */
}


/* --------------------------------------------------------------------------
   First pass -- only remove comments
-------------------------------------------------------------------------- */
static void minify_1(char *dest, const char *src, int len)
{
    int  i;
    int  j=0;
    bool opensq=FALSE;       /* single quote */
    bool opendq=FALSE;       /* double quote */
    int  backslashes=0;
    bool openco=FALSE;       /* comment */
    bool opensc=FALSE;       /* star comment */

    for ( i=0; i<len; ++i )
    {
        /* odd number of backslashes invalidates the quote */

        if ( !openco && !opensc && !opensq && src[i]=='"' && backslashes%2==0 )
        {
            if ( !opendq )
                opendq = TRUE;
            else
                opendq = FALSE;
        }
        else if ( !openco && !opensc && !opendq && src[i]=='\'' && backslashes%2==0 )
        {
            if ( !opensq )
                opensq = TRUE;
            else
                opensq = FALSE;
        }
        else if ( !opensq && !opendq && !openco && !opensc && src[i]=='/' && src[i+1] == '/' )
        {
            openco = TRUE;
        }
        else if ( !opensq && !opendq && !openco && !opensc && src[i]=='/' && src[i+1] == '*' )
        {
            opensc = TRUE;
        }
        else if ( openco && src[i]=='\n' )
        {
            openco = FALSE;
        }
        else if ( opensc && src[i]=='*' && src[i+1]=='/' )
        {
            opensc = FALSE;
            i += 2;
        }

        if ( !openco && !opensc )       /* unless it's a comment ... */
        {
            dest[j++] = src[i];

            if ( src[i]=='\\' )
                ++backslashes;
            else
                backslashes = 0;
        }
    }

    dest[j] = EOS;
}


/* --------------------------------------------------------------------------
   Normalize whitespaces & new line characters
-------------------------------------------------------------------------- */
static void minify_2(char *dest, const char *src)
{
    bool first=TRUE;
    bool opensq=FALSE;       /* single quote */
    bool opendq=FALSE;       /* double quote */
    int  backslashes=0;
    char prev_written=' ';

    while ( *src==' ' || *src=='\t' || *src=='\r' || *src=='\n' ) ++src;   /* skip leading whitespaces */

    while ( *src )
    {
        /* odd number of backslashes invalidates the quote */

        if ( !opensq && *src=='"' && backslashes%2==0 )
        {
            if ( !opendq )
                opendq = TRUE;
            else
                opendq = FALSE;
        }
        else if ( !opendq && *src=='\'' && backslashes%2==0 )
        {
            if ( !opensq )
                opensq = TRUE;
            else
                opensq = FALSE;
        }

        if ( *src != '\r' )
        {
            if ( *src==' ' || *src=='\n' || *src=='\t' )
            {
                if ( prev_written != ' ' )
                    *dest++ = ' ';
            }
            else
                *dest++ = *src;

            if ( !first )
                prev_written = *(dest-1);

            first = FALSE;
        }

        if ( *src=='\\' )
            ++backslashes;
        else
            backslashes = 0;

        ++src;
    }

    *dest = EOS;
}


/* --------------------------------------------------------------------------
   Remove excess whitespaces
   Return new length
-------------------------------------------------------------------------- */
static int minify_3(char *dest, const char *src)
{
    int  len;
    int  i;
    int  j=0;
    bool opensq=FALSE;       /* single quote */
    bool opendq=FALSE;       /* double quote */
    int  backslashes=0;

    len = strlen(src);

    for ( i=0; i<len; ++i )
    {
        /* odd number of backslashes invalidates the quote */

        if ( !opensq && src[i]=='"' && backslashes%2==0 )
        {
            if ( !opendq )
                opendq = TRUE;
            else
                opendq = FALSE;
        }
        else if ( !opendq && src[i]=='\'' && backslashes%2==0 )
        {
            if ( !opensq )
                opensq = TRUE;
            else
                opensq = FALSE;
        }

        if ( !opensq && !opendq && src[i]==' ' )
        {
            if ( src[i-1] != ':' && src[i-1] != ';'
                    && src[i-1] != '{' && src[i-1] != '}'
                    && src[i-1] != '=' && src[i-1] != '+' && src[i-1] != ','
                    && src[i-1] != '(' && src[i-1] != ')'
                    && src[i+1] != ':' && src[i+1] != ';'
                    && src[i+1] != '{' && src[i+1] != '}'
                    && src[i+1] != '=' && src[i+1] != '+' && src[i+1] != ','
                    && (src[i+1]!='(' || (src[i+1]=='(' && src[i-1]=='d')) && src[i+1] != ')' )
                dest[j++] = ' ';
        }
        else
            dest[j++] = src[i];

        if ( src[i]=='\\' )
            ++backslashes;
        else
            backslashes = 0;
    }

    dest[j] = EOS;

    return j;
}


/* --------------------------------------------------------------------------
   Minify CSS/JS
   return new length
-------------------------------------------------------------------------- */
int npp_minify(char *dest, const char *src)
{
    char *temp1;

    int len = strlen(src);

    if ( !(temp1=(char*)malloc(len+1)) )
    {
        ERR("Couldn't allocate %d bytes for npp_minify", len);
        return 0;
    }

    minify_1(temp1, src, len);          /* remove comments */

#ifdef NPP_DEBUG
//    npp_log_long(temp1, strlen(temp1), "After minify_1");
#endif

    /* ------------------------------------------------- */

    char *temp2;

    len = strlen(temp1);

    if ( !(temp2=(char*)malloc(len+1)) )
    {
        ERR("Couldn't allocate %d bytes for npp_minify", len);
        free(temp1);
        return 0;
    }

    minify_2(temp2, temp1);             /* normalize whitespaces & new line characters */

    free(temp1);

#ifdef NPP_DEBUG
//    npp_log_long(temp2, strlen(temp2), "After minify_2");
#endif

    /* ------------------------------------------------- */

    int ret = minify_3(dest, temp2);    /* remove excess whitespaces */

    free(temp2);

#ifdef NPP_DEBUG
    npp_log_long(dest, ret, "After minify_3");
#endif

    return ret;
}


/* --------------------------------------------------------------------------
   Increment date by 'days' days. Return day of week as well.
   Format: YYYY-MM-DD
-------------------------------------------------------------------------- */
void date_inc(char *str, int days, int *dow)
{
    char    full[32];
    time_t  told, tnew;

    sprintf(full, "%s 00:00:00", str);

    told = time_db2epoch(full);

    tnew = told + 3600*24*days;

    G_ptm = gmtime(&tnew);
    sprintf(str, "%d-%02d-%02d", G_ptm->tm_year+1900, G_ptm->tm_mon+1, G_ptm->tm_mday);
    *dow = G_ptm->tm_wday;

    G_ptm = gmtime(&G_now);   /* set it back */

}


/* --------------------------------------------------------------------------
   Compare dates
   Format: YYYY-MM-DD
-------------------------------------------------------------------------- */
int date_cmp(const char *str1, const char *str2)
{
    char    full[32];
    time_t  t1, t2;

    sprintf(full, "%s 00:00:00", str1);
    t1 = time_db2epoch(full);

    sprintf(full, "%s 00:00:00", str2);
    t2 = time_db2epoch(full);

    return t1 - t2;
}


/* --------------------------------------------------------------------------
   Compare datetime
   Format: YYYY-MM-DD hh:mm:ss
-------------------------------------------------------------------------- */
int datetime_cmp(const char *str1, const char *str2)
{
    time_t  t1, t2;

    t1 = time_db2epoch(str1);
    t2 = time_db2epoch(str2);

    return t1 - t2;
}


/* --------------------------------------------------------------------------
   Read the config file
-------------------------------------------------------------------------- */
bool npp_read_conf(const char *file)
{
    FILE *h_file=NULL;

    /* open the conf file */

#ifdef _WIN32   /* Windows */
    if ( NULL == (h_file=fopen(file, "rb")) )
#else
    if ( NULL == (h_file=fopen(file, "r")) )
#endif
    {
//        printf("Error opening %s, using defaults.\n", file);
        return FALSE;
    }

    /* read content into M_conf for npp_read_param */

    fseek(h_file, 0, SEEK_END);     /* determine the file size */
    unsigned size = ftell(h_file);
    rewind(h_file);

    if ( (M_conf=(char*)malloc(size+1)) == NULL )
    {
        printf("ERROR: Couldn't get %u bytes for M_conf\n", size+1);
        fclose(h_file);
        return FALSE;
    }

    if ( fread(M_conf, size, 1, h_file) != 1 )
    {
        printf("ERROR: Couldn't read from %s\n", file);
        fclose(h_file);
        return FALSE;
    }

    *(M_conf+size) = EOS;

    fclose(h_file);

    return TRUE;
}


/* --------------------------------------------------------------------------
   Get param from config file
---------------------------------------------------------------------------*/
bool npp_read_param_str(const char *param, char *dest)
{
    char *p;
    int  plen = strlen(param);

    DDBG("npp_read_param_str [%s]", param);

    if ( !M_conf )
    {
//        ERR("No config file or not read yet");
        return FALSE;
    }

    if ( (p=strstr(M_conf, param)) == NULL )
    {
//        if ( dest ) dest[0] = EOS;
        return FALSE;
    }

    /* string present but is it label or value? */

    bool found=FALSE;

    while ( p )    /* param may be commented out but present later */
    {
        if ( p > M_conf && *(p-1) != '\n' )  /* commented out or within quotes -- try the next occurence */
        {
            DDBG("param commented out or within quotes, continue search...");
            p = strstr(++p, param);
        }
        else if ( *(p+plen) != '=' && *(p+plen) != ' ' && *(p+plen) != '\t' )
        {
            DDBG("param does not end with '=', space or tab, continue search...");
            p = strstr(++p, param);
        }
        else
        {
            found = TRUE;
            break;
        }
    }

    if ( !found )
        return FALSE;

    /* param present ----------------------------------- */

    if ( !dest ) return TRUE;   /* it's only a presence check */


    /* copy value to dest ------------------------------ */

    p += plen;

    while ( *p=='=' || *p==' ' || *p=='\t' )
        ++p;

    int i=0;

    while ( *p != '\r' && *p != '\n' && *p != '#' && *p != EOS )
        dest[i++] = *p++;

    dest[i] = EOS;

    if ( strstr(npp_upper(param), "PASSWORD") || strstr(npp_upper(param), "PASSWD") )
        DBG("%s [<...>]", param);
    else
        DBG("%s [%s]", param, dest);

    return TRUE;
}


/* --------------------------------------------------------------------------
   Get integer param from config file
---------------------------------------------------------------------------*/
bool npp_read_param_int(const char *param, int *dest)
{
    char tmp[256];

    if ( npp_read_param_str(param, tmp) )
    {
        if ( dest )
            sscanf(tmp, "%d", dest);

        return TRUE;
    }

    return FALSE;
}


/* --------------------------------------------------------------------------
   Read & parse conf file and set global parameters
-------------------------------------------------------------------------- */
void npp_lib_read_conf(bool first)
{
#ifdef NPP_CLIENT
    DBG("Reading configuration...");
#else
    INF("Reading configuration...");
#endif

    bool conf_read=FALSE;

    /* -------------------------------------------------- */
    /* set defaults */

    if ( first )
    {
        G_test = 0;
        G_logLevel = LOG_INF;
        G_logToStdout = 0;
        G_logCombined = 0;

#ifdef NPP_WATCHER
        G_httpPort = 80;
#endif

#ifdef NPP_APP
        G_httpPort = 80;
        G_httpsPort = 443;
        G_cipherList[0] = EOS;
        G_certFile[0] = EOS;
        G_certChainFile[0] = EOS;
        G_keyFile[0] = EOS;
#endif

        G_dbHost[0] = EOS;
        G_dbPort = 0;
        G_dbName[0] = EOS;
        G_dbUser[0] = EOS;
        G_dbPassword[0] = EOS;

#ifdef NPP_USERS
        G_usersRequireActivation = 0;
#endif

#ifdef NPP_APP
        G_IPBlackList[0] = EOS;
        G_IPWhiteList[0] = EOS;
#endif

#ifdef NPP_ASYNC
        G_ASYNCId = -1;
        G_ASYNCSvcProcesses = 0;
        G_ASYNCDefTimeout = NPP_ASYNC_DEF_TIMEOUT;
#endif

        G_callHTTPTimeout = CALL_HTTP_DEFAULT_TIMEOUT;
    }

    /* -------------------------------------------------- */
    /* get the conf file path & name */

    if ( G_appdir[0] )
    {
        char conf_path[1024];

        sprintf(conf_path, "%s/bin/npp.conf", G_appdir);

        DBG("Trying read %s...", conf_path);

        conf_read = npp_read_conf(conf_path);
    }

    if ( !conf_read )   /* no NPP_DIR or no npp.conf in bin -- try current dir */
    {
#ifndef NPP_WATCHER
        WAR("Couldn't read $NPP_DIR/bin/npp.conf -- trying current directory...");
#endif
        conf_read = npp_read_conf("npp.conf");
    }

    if ( conf_read )
    {
        /* test */

        npp_read_param_int("test", &G_test);

        /* logLevel */

#ifndef NPP_WATCHER   /* npp_watcher has its own log settings */
#ifndef NPP_UPDATE
        npp_read_param_int("logLevel", &G_logLevel);
#endif
#endif

        /* logToStdout */

        if ( first )
        {
            npp_read_param_int("logToStdout", &G_logToStdout);
        }
        else    /* npp_reload_conf */
        {
            int tmp_logToStdout=G_logToStdout;

            npp_read_param_int("logToStdout", &tmp_logToStdout);

            if ( tmp_logToStdout != G_logToStdout )
            {
                G_logToStdout = tmp_logToStdout;

                if ( G_logToStdout )    /* switch to stdout */
                {
                    ALWAYS("Switching log to stdout");
                    npp_lib_log_switch_to_stdout();
                }
                else    /* switch to file */
                {
                    ALWAYS("Switching log to a file");
                    npp_lib_log_switch_to_file();
                }
            }
        }

        /* -------------------------------------------------- */
        /* logCombined */

        npp_read_param_int("logCombined", &G_logCombined);

        /* -------------------------------------------------- */
        /* ports */

#ifdef NPP_WATCHER
        npp_read_param_int("httpPort", &G_httpPort);
#endif

#ifdef NPP_APP

        if ( first )
        {
            npp_read_param_int("httpPort", &G_httpPort);
            npp_read_param_int("httpsPort", &G_httpsPort);
        }
        else    /* can't change it online */
        {
            int tmp_httpPort=G_httpPort;
            int tmp_httpsPort=G_httpsPort;

            npp_read_param_int("httpPort", &tmp_httpPort);
            npp_read_param_int("httpsPort", &tmp_httpsPort);

            if ( tmp_httpPort != G_httpPort
                    || tmp_httpsPort != G_httpsPort )
            {
                WAR("Changing listening ports requires server restart");
            }
        }

        /* -------------------------------------------------- */
        /* SSL */

#ifdef NPP_HTTPS
        if ( first )
        {
            npp_read_param_str("cipherList", G_cipherList);
            npp_read_param_str("certFile", G_certFile);
            npp_read_param_str("certChainFile", G_certChainFile);
            npp_read_param_str("keyFile", G_keyFile);
        }
        else    /* npp_reload_conf */
        {
            char tmp_cipherList[NPP_CIPHER_LIST_LEN+1]="";
            char tmp_certFile[256]="";
            char tmp_certChainFile[256]="";
            char tmp_keyFile[256]="";

            npp_read_param_str("cipherList", tmp_cipherList);
            npp_read_param_str("certFile", tmp_certFile);
            npp_read_param_str("certChainFile", tmp_certChainFile);
            npp_read_param_str("keyFile", tmp_keyFile);

            if ( strcmp(tmp_cipherList, G_cipherList) != 0
                    || strcmp(tmp_certFile, G_certFile) != 0
                    || strcmp(tmp_certChainFile, G_certChainFile) != 0
                    || strcmp(tmp_keyFile, G_keyFile) != 0 )
            {
                strcpy(G_cipherList, tmp_cipherList);
                strcpy(G_certFile, tmp_certFile);
                strcpy(G_certChainFile, tmp_certChainFile);
                strcpy(G_keyFile, tmp_keyFile);

                SSL_CTX_free(M_ssl_ctx);
                EVP_cleanup();

                npp_eng_init_ssl();
            }
        }
#endif  /* NPP_HTTPS */

#endif  /* NPP_APP */

        /* -------------------------------------------------- */
        /* database */

        if ( first )
        {
            npp_read_param_str("dbHost", G_dbHost);
            npp_read_param_int("dbPort", &G_dbPort);
            npp_read_param_str("dbName", G_dbName);
            npp_read_param_str("dbUser", G_dbUser);
            npp_read_param_str("dbPassword", G_dbPassword);
        }
        else    /* reconnect MySQL if changed */
        {
            char tmp_dbHost[128]="";
            int  tmp_dbPort=0;
            char tmp_dbName[128]="";
            char tmp_dbUser[128]="";
            char tmp_dbPassword[128]="";

            npp_read_param_str("dbHost", tmp_dbHost);
            npp_read_param_int("dbPort", &tmp_dbPort);
            npp_read_param_str("dbName", tmp_dbName);
            npp_read_param_str("dbUser", tmp_dbUser);
            npp_read_param_str("dbPassword", tmp_dbPassword);

            if ( strcmp(tmp_dbHost, G_dbHost) != 0
                    || tmp_dbPort != G_dbPort
                    || strcmp(tmp_dbName, G_dbName) != 0
                    || strcmp(tmp_dbUser, G_dbUser) != 0
                    || strcmp(tmp_dbPassword, G_dbPassword) != 0 )
            {
                strcpy(G_dbHost, tmp_dbHost);
                G_dbPort = tmp_dbPort;
                strcpy(G_dbName, tmp_dbName);
                strcpy(G_dbUser, tmp_dbUser);
                strcpy(G_dbPassword, tmp_dbPassword);
#ifdef NPP_MYSQL
                npp_close_db();

                if ( !npp_open_db() )
                    WAR("Couldn't connect to the database");
#endif  /* NPP_MYSQL */
            }
        }

        /* -------------------------------------------------- */
        /* usersRequireActivation */

#ifdef NPP_USERS
        npp_read_param_int("usersRequireActivation", &G_usersRequireActivation);
#endif

        /* -------------------------------------------------- */
        /* IPBlackList */

#ifdef NPP_APP

        if ( first )
        {
            npp_read_param_str("IPBlackList", G_IPBlackList);
        }
        else    /* npp_reload_conf */
        {
            char tmp_IPBlackList[256]="";

            npp_read_param_str("IPBlackList", tmp_IPBlackList);

            if ( strcmp(tmp_IPBlackList, G_IPBlackList) != 0 )
            {
                strcpy(G_IPBlackList, tmp_IPBlackList);
                npp_eng_read_blocked_ips();
            }
        }

        /* -------------------------------------------------- */
        /* IPWhiteList */

        if ( first )
        {
            npp_read_param_str("IPWhiteList", G_IPWhiteList);
        }
        else    /* npp_reload_conf */
        {
            char tmp_IPWhiteList[256]="";

            npp_read_param_str("IPWhiteList", tmp_IPWhiteList);

            if ( strcmp(tmp_IPWhiteList, G_IPWhiteList) != 0 )
            {
                strcpy(G_IPWhiteList, tmp_IPWhiteList);
                npp_eng_read_allowed_ips();
            }
        }

#endif  /* NPP_APP */

        /* -------------------------------------------------- */
        /* ASYNC */

#ifdef NPP_ASYNC
        if ( first )
        {
            npp_read_param_int("ASYNCId", &G_ASYNCId);
            npp_read_param_int("ASYNCSvcProcesses", &G_ASYNCSvcProcesses);
        }
        else    /* can't change it online */
        {
            int tmp_ASYNCId=G_ASYNCId;
            int tmp_ASYNCSvcProcesses=G_ASYNCSvcProcesses;

            npp_read_param_int("ASYNCId", &tmp_ASYNCId);
            npp_read_param_int("ASYNCSvcProcesses", &tmp_ASYNCSvcProcesses);

            if ( tmp_ASYNCId != G_ASYNCId || tmp_ASYNCSvcProcesses != G_ASYNCSvcProcesses )
            {
                WAR("Changing ASYNCId or ASYNCSvcProcesses requires server restart");
            }
        }

        npp_read_param_int("ASYNCDefTimeout", &G_ASYNCDefTimeout);
#endif  /* NPP_ASYNC */

        /* -------------------------------------------------- */
        /* CALL_HTTP */

        npp_read_param_int("callHTTPTimeout", &G_callHTTPTimeout);
    }
    else
    {
#ifndef NPP_WATCHER
        WAR("Couldn't read npp.conf%s", first?" -- using defaults":"");
#endif
    }

#ifndef NPP_WATCHER
#ifdef NPP_DEBUG
    if ( G_logLevel < 4 )
    {
        G_logLevel = 4;   /* debug */
        DBG("logLevel changed to 4 because of NPP_DEBUG");
    }
#endif  /* NPP_DEBUG */
#endif  /* NPP_WATCHER */
}


/* --------------------------------------------------------------------------
   Create a pid file
-------------------------------------------------------------------------- */
char *npp_lib_create_pid_file(const char *name)
{
static char pidfilename[512];
    FILE    *fpid=NULL;
    char    command[1024];

    if ( G_pid == 0 )
        G_pid = getpid();

    if ( G_appdir[0] )
#ifdef _WIN32   /* Windows */
        sprintf(pidfilename, "%s\\bin\\%s.pid", G_appdir, name);
#else
        sprintf(pidfilename, "%s/bin/%s.pid", G_appdir, name);
#endif
    else    /* empty NPP_DIR */
        sprintf(pidfilename, "%s.pid", name);

    /* check if the pid file already exists */

    if ( access(pidfilename, F_OK) != -1 )
    {
        WAR("PID file already exists");
        INF("Killing the old process...");
#ifdef _WIN32   /* Windows */
        /* open the pid file and read process id */
        if ( NULL == (fpid=fopen(pidfilename, "rb")) )
        {
            ERR("Couldn't open pid file for reading");
            return NULL;
        }
        fseek(fpid, 0, SEEK_END);     /* determine the file size */
        int fsize = ftell(fpid);
        if ( fsize < 1 || fsize > 60 )
        {
            fclose(fpid);
            ERR("Something's wrong with the pid file size (%d bytes)", fsize);
            return NULL;
        }
        rewind(fpid);
        char oldpid[64];

        if ( fread(oldpid, fsize, 1, fpid) != 1 )
        {
            ERR("Couldn't read from the old pid file");
            fclose(fpid);
            return NULL;
        }

        fclose(fpid);
        oldpid[fsize] = EOS;
        DBG("oldpid [%s]", oldpid);

        msleep(100);

        sprintf(command, "taskkill /pid %s", oldpid);
#else
        sprintf(command, "kill `cat %s`", pidfilename);
#endif  /* _WIN32 */

        INF("Removing pid file...");
#ifdef _WIN32   /* Windows */
        sprintf(command, "del %s", pidfilename);
#else
        sprintf(command, "rm %s", pidfilename);
#endif
        if ( system(command) != EXIT_SUCCESS )
            WAR("Couldn't execute %s", command);

        msleep(100);
    }

    /* create a pid file */

#ifdef _WIN32   /* Windows */
    if ( NULL == (fpid=fopen(pidfilename, "wb")) )
#else
    if ( NULL == (fpid=fopen(pidfilename, "w")) )
#endif
    {
        INF("Tried to create [%s]", pidfilename);
        ERR("Failed to create pid file, errno = %d (%s)", errno, strerror(errno));
        return NULL;
    }

    /* write pid to pid file */

    if ( fprintf(fpid, "%d", G_pid) < 1 )
    {
        ERR("Couldn't write to pid file, errno = %d (%s)", errno, strerror(errno));
        fclose(fpid);
        return NULL;
    }

    fclose(fpid);

    return pidfilename;
}


/* --------------------------------------------------------------------------
   Attach to shared memory segment
-------------------------------------------------------------------------- */
char *npp_lib_shm_create(unsigned bytes, int index)
{
    char *shm_segptr=NULL;

    if ( index >= NPP_MAX_SHM_SEGMENTS )
    {
        ERR("Too many SHM segments, NPP_MAX_SHM_SEGMENTS=%d", NPP_MAX_SHM_SEGMENTS);
        return NULL;
    }

#ifndef _WIN32

    /* Create unique key via call to ftok() */

    key_t key;
    if ( (key=ftok(G_appdir, '0'+(char)index)) == -1 )
    {
        ERR("ftok, errno = %d (%s)", errno, strerror(errno));
        return NULL;
    }

    /* Open the shared memory segment - create if necessary */

    if ( (M_shmid[index]=shmget(key, bytes, IPC_CREAT|IPC_EXCL|0600)) == -1 )
    {
        INF("Shared memory segment exists - opening as client");

        if ( (M_shmid[index]=shmget(key, bytes, 0)) == -1 )
        {
            ERR("shmget, errno = %d (%s)", errno, strerror(errno));
            return NULL;
        }
    }
    else
    {
        INF("Creating new shared memory segment");
    }

    /* Attach (map) the shared memory segment into the current process */

    if ( (shm_segptr=(char*)shmat(M_shmid[index], 0, 0)) == (char*)-1 )
    {
        ERR("shmat, errno = %d (%s)", errno, strerror(errno));
        return NULL;
    }

#endif  /* _WIN32 */

    return shm_segptr;
}


/* --------------------------------------------------------------------------
   Delete shared memory segment
-------------------------------------------------------------------------- */
void npp_lib_shm_delete(int index)
{
#ifndef _WIN32
    if ( M_shmid[index] )
    {
        shmctl(M_shmid[index], IPC_RMID, 0);
        M_shmid[index] = 0;
        INF("Shared memory segment (index=%d) marked for deletion", index);
    }
#endif  /* _WIN32 */
}


/* --------------------------------------------------------------------------
   Start a log
-------------------------------------------------------------------------- */
bool npp_log_start(const char *prefix, bool test, bool switching)
{
    char    fprefix[64]="";     /* formatted prefix */
    char    fname[512];         /* file name */
    char    ffname[1024];       /* full file name */

    if ( G_logLevel < 1 ) return TRUE;  /* no log */

    if ( G_logToStdout < 1 )   /* log to a file */
    {
        if ( G_log_fd != NULL && G_log_fd != stdout ) return TRUE;  /* already started */

        if ( prefix && prefix[0] )
            snprintf(fprefix, 62, "%s_", prefix);

        sprintf(fname, "%s%d%02d%02d_%02d%02d", fprefix, G_ptm->tm_year+1900, G_ptm->tm_mon+1, G_ptm->tm_mday, G_ptm->tm_hour, G_ptm->tm_min);

        if ( test )
            sprintf(ffname, "%s_t.log", fname);
        else
            sprintf(ffname, "%s.log", fname);

        /* first try in NPP_DIR --------------------------------------------- */

        if ( G_appdir[0] )
        {
            char fffname[2048];     /* full file name with path */

#ifdef _WIN32
            sprintf(fffname, "%s\\logs\\%s", G_appdir, ffname);
#else
            sprintf(fffname, "%s/logs/%s", G_appdir, ffname);
#endif
            if ( NULL == (G_log_fd=fopen(fffname, "a")) )
            {
                if ( NULL == (G_log_fd=fopen(ffname, "a")) )  /* try current dir */
                {
                    printf("ERROR: Couldn't open log file.\n");
                    return FALSE;
                }
            }
        }
        else    /* no NPP_DIR -- try current dir */
        {
            if ( NULL == (G_log_fd=fopen(ffname, "a")) )
            {
                printf("ERROR: Couldn't open log file.\n");
                return FALSE;
            }
        }
    }

    if ( !switching )
    {
        fprintf(G_log_fd, LOG_LINE_LONG_N);
        ALWAYS(" %s  Starting %s's log. Node++ version: %s", DT_NOW_GMT, NPP_APP_NAME, NPP_VERSION);
        fprintf(G_log_fd, LOG_LINE_LONG_NN);
    }

    return TRUE;
}


/* --------------------------------------------------------------------------
   Write to log
-------------------------------------------------------------------------- */
#ifndef NPP_CPP_STRINGS
void npp_log_write(char level, const char *message, ...)
{
    if ( level > G_logLevel ) return;

    if ( LOG_ERR == level )
        fprintf(G_log_fd, "ERROR: ");
    else if ( LOG_WAR == level )
        fprintf(G_log_fd, "WARNING: ");

    /* compile message with arguments into buffer */

    char buffer[NPP_MAX_LOG_STR_LEN+1+64];

    va_list args;
    va_start(args, message);
    vsprintf(buffer, message, args);
    va_end(args);

    /* write to the log file */

    fprintf(G_log_fd, "%s\n", buffer);

#ifdef NPP_DEBUG
    fflush(G_log_fd);
#else
    if ( G_logLevel >= LOG_DBG || level == LOG_ERR ) fflush(G_log_fd);
#endif
}


/* --------------------------------------------------------------------------
   Write to log with date/time
-------------------------------------------------------------------------- */
void npp_log_write_time(char level, const char *message, ...)
{
    if ( level > G_logLevel ) return;

    /* output timestamp */

    fprintf(G_log_fd, "[%s] ", G_dt_string_gmt+11);

    if ( LOG_ERR == level )
        fprintf(G_log_fd, "ERROR: ");
    else if ( LOG_WAR == level )
        fprintf(G_log_fd, "WARNING: ");

    /* compile message with arguments into buffer */

    char buffer[NPP_MAX_LOG_STR_LEN+1+64];

    va_list args;
    va_start(args, message);
    vsprintf(buffer, message, args);
    va_end(args);

    /* write to the log file */

    fprintf(G_log_fd, "%s\n", buffer);

#ifdef NPP_DEBUG
    fflush(G_log_fd);
#else
    if ( G_logLevel >= LOG_DBG || level == LOG_ERR ) fflush(G_log_fd);
#endif
}
#endif  /* NPP_CPP_STRINGS */


/* --------------------------------------------------------------------------
   Write looong message to a log or --
   its first (NPP_MAX_LOG_STR_LEN-50) part if it's longer
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
void npp_log_long(const std::string& message_, size_t len, const std::string& desc_)
{
    const char *message = message_.c_str();
    const char *desc = desc_.c_str();
#else
void npp_log_long(const char *message, size_t len, const char *desc)
{
#endif
    if ( G_logLevel < LOG_DBG ) return;

    char buffer[NPP_MAX_LOG_STR_LEN+1];

    if ( len < NPP_MAX_LOG_STR_LEN-50 )
    {
        COPY(buffer, message, len);
    }
    else    /* need to truncate */
    {

#if __GNUC__ > 7
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif

        strncpy(buffer, message, NPP_MAX_LOG_STR_LEN-50);
        strcpy(buffer+NPP_MAX_LOG_STR_LEN-50, " (...)");

#if __GNUC__ > 7
#pragma GCC diagnostic pop
#endif

    }

    DBG("%s:\n\n[%s]\n", desc, buffer);
}


/* --------------------------------------------------------------------------
   Flush log
-------------------------------------------------------------------------- */
void npp_log_flush()
{
    if ( G_log_fd != NULL )
        fflush(G_log_fd);
}


/* --------------------------------------------------------------------------
   Switch log
-------------------------------------------------------------------------- */
void npp_lib_log_switch_to_stdout()
{
    if ( G_log_fd != NULL && G_log_fd != stdout )
    {
        fclose(G_log_fd);
        G_log_fd = stdout;
    }
}


/* --------------------------------------------------------------------------
   Switch log
-------------------------------------------------------------------------- */
void npp_lib_log_switch_to_file()
{
    npp_log_start("", G_test, TRUE);
}


/* --------------------------------------------------------------------------
   Close log
-------------------------------------------------------------------------- */
void npp_log_finish()
{
    if ( G_logLevel > 0 )
        INF_T("Closing log");

    npp_lib_log_switch_to_stdout();
}


/* --------------------------------------------------------------------------
   Convert string
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
char *npp_convert(const std::string& src_, const std::string& cp_from_, const std::string& cp_to_)
{
    const char *src = src_.c_str();
    const char *cp_from = cp_from_.c_str();
    const char *cp_to = cp_to_.c_str();
#else
char *npp_convert(const char *src, const char *cp_from, const char *cp_to)
{
#endif
static char dst[NPP_LIB_STR_BUF];

    char *out_buf = dst;

#ifdef NPP_ICONV

    iconv_t cd = iconv_open(cp_to, cp_from);

    if ( cd == (iconv_t)-1 )
    {
        strcpy(dst, "iconv_open failed");
        return dst;
    }

    const char *in_buf = src;
    size_t in_left = strlen(src);

    size_t out_left = NPP_LIB_STR_BUF-1;

    do
    {
        if ( iconv(cd, (char**)&in_buf, &in_left, &out_buf, &out_left) == (size_t)-1 )
        {
            strcpy(dst, "iconv failed");
            return dst;
        }
    } while (in_left > 0 && out_left > 0);

    iconv_close(cd);

#endif  /* NPP_ICONV */

    *out_buf = EOS;

    return dst;
}


/* --------------------------------------------------------------------------
   Seed rand()
-------------------------------------------------------------------------- */
static void seed_rand()
{
#define NPP_SEEDS_MEM 256  /* remaining 8 bits & last seeds to remember */
static char first=1;
/* make sure at least the last NPP_SEEDS_MEM seeds are unique */
static unsigned int seeds[NPP_SEEDS_MEM];

    DBG("seed_rand");

    /* generate possibly random, or at least based on some non-deterministic factors, 32-bit integer */

    int time_remainder = (int)G_now % 63 + 1;     /* 6 bits */
    int mem_remainder = npp_get_memory() % 63 + 1;    /* 6 bits */
    int pid_remainder;       /* 6 bits */
    int yesterday_rem;    /* 6 bits */

    if ( first )    /* initial seed */
    {
        pid_remainder = G_pid % 63 + 1;
#ifdef NPP_CLIENT
        yesterday_rem = 30;
#else
        yesterday_rem = G_cnts_yesterday.req % 63 + 1;
#endif
    }
    else    /* subsequent calls */
    {
        pid_remainder = rand() % 63 + 1;
        yesterday_rem = rand() % 63 + 1;
    }

static int seeded=0;    /* 8 bits */

    unsigned int seed;
static unsigned int prev_seed=0;

    while ( 1 )
    {
        if ( seeded >= NPP_SEEDS_MEM )
            seeded = 1;
        else
            ++seeded;

        seed = pid_remainder * time_remainder * mem_remainder * yesterday_rem * seeded;

        /* check uniqueness in the history */

        char found=0;
        int i = 0;
        while ( i < NPP_SEEDS_MEM )
        {
            if ( seeds[i++] == seed )
            {
                found = 1;
                break;
            }
        }

        if ( found )    /* same seed again */
        {
            WAR("seed %u repeated; seeded = %d, i = %d", seed, seeded, i);
        }
        else   /* seed not found ==> OK */
        {
            /* new seed needs to be at least 10000 apart from the previous one */

            if ( !first && abs((long long)(seed-prev_seed)) < 10000 )
            {
                WAR("seed %u too close to the previous one (%u); seeded = %d", seed, prev_seed, seeded);
            }
            else    /* everything OK */
            {
                seeds[seeded-1] = seed;
                break;
            }
        }

        /* stir it up to avoid endless loop */

        pid_remainder = rand() % 63 + 1;
        time_remainder = rand() % 63 + 1;
    }

#ifdef NPP_DEBUG
    char f[256];
    npp_lib_fmt_int_generic(f, seed);
    DBG("seed = %s", f);
    DBG("");
#endif  /* NPP_DEBUG */

    prev_seed = seed;

    first = 0;

    srand(seed);
}


/* --------------------------------------------------------------------------
   Initialize M_random_numbers array
-------------------------------------------------------------------------- */
void npp_lib_init_random_numbers()
{
    int i;

#ifdef NPP_DEBUG
    struct timespec start;
#ifdef _WIN32
    clock_gettime_win(&start);
#else
    clock_gettime(MONOTONIC_CLOCK_NAME, &start);
#endif
#endif  /* NPP_DEBUG */

    DBG("npp_lib_init_random_numbers");

    seed_rand();

#ifdef __linux__
    /* On Linux we have access to a hardware-influenced RNG */

    DBG("Trying /dev/urandom...");

    int urandom_fd = open("/dev/urandom", O_RDONLY);

    if ( urandom_fd )
    {
        if ( read(urandom_fd, &M_random_numbers, NPP_RANDOM_NUMBERS) < NPP_RANDOM_NUMBERS )
        {
            WAR("Couldn't read from /dev/urandom");
            close(urandom_fd);
            return;
        }

        close(urandom_fd);

        INF("M_random_numbers obtained from /dev/urandom");
    }
    else
    {
        WAR("Couldn't open /dev/urandom");

        /* fallback to old plain rand(), seeded the best we could */

        for ( i=0; i<NPP_RANDOM_NUMBERS; ++i )
            M_random_numbers[i] = rand() % 256;

        INF("M_random_numbers obtained from rand()");
    }

#else   /* Windows */

    for ( i=0; i<NPP_RANDOM_NUMBERS; ++i )
        M_random_numbers[i] = rand() % 256;

    INF("M_random_numbers obtained from rand()");

#endif

    INF("");

    M_random_initialized = 1;

#ifdef NPP_DEBUG

    DBG("--------------------------------------------------------------------------------------------------------------------------------");
    DBG("M_random_numbers distribution visualization");
    DBG("The square below should be filled fairly randomly and uniformly.");
    DBG("If it's not, or you can see any regular patterns across the square, your system may be broken or too old to be deemed secure.");
    DBG("--------------------------------------------------------------------------------------------------------------------------------");

    /* One square takes two columns, so we can have between 0 and 4 dots per square */

#define SQUARE_ROWS             64
#define SQUARE_COLS             SQUARE_ROWS*2
#define SQUARE_IS_EMPTY(x, y)   (dots[y][x*2]==' ' && dots[y][x*2+1]==' ')
#define SQUARE_HAS_ONE(x, y)    (dots[y][x*2]==' ' && dots[y][x*2+1]=='.')
#define SQUARE_HAS_TWO(x, y)    (dots[y][x*2]=='.' && dots[y][x*2+1]=='.')
#define SQUARE_HAS_THREE(x, y)  (dots[y][x*2]=='.' && dots[y][x*2+1]==':')
#define SQUARE_HAS_FOUR(x, y)   (dots[y][x*2]==':' && dots[y][x*2+1]==':')

#if __GNUC__ < 6
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
#endif

    char dots[SQUARE_ROWS][SQUARE_COLS+1]={0};

#if __GNUC__ < 6
#pragma GCC diagnostic pop
#endif

    int j;

    for ( i=0; i<SQUARE_ROWS; ++i )
        for ( j=0; j<SQUARE_COLS; ++j )
            dots[i][j] = ' ';

    /* we only have SQUARE_ROWS^2 squares with 5 possible values in each of them */
    /* let only once in divider in */

    int divider = NPP_RANDOM_NUMBERS / (SQUARE_ROWS*SQUARE_ROWS) + 1;

    for ( i=0; i<NPP_RANDOM_NUMBERS-1; i+=2 )
    {
        if ( i % divider ) continue;

        int x = M_random_numbers[i] % SQUARE_ROWS;
        int y = M_random_numbers[i+1] % SQUARE_ROWS;

        if ( SQUARE_IS_EMPTY(x, y) )    /* make it one */
            dots[y][x*2+1] = '.';
        else if ( SQUARE_HAS_ONE(x, y) )    /* make it two */
            dots[y][x*2] = '.';
        else if ( SQUARE_HAS_TWO(x, y) )    /* make it three */
            dots[y][x*2+1] = ':';
        else if ( SQUARE_HAS_THREE(x, y) )  /* make it four */
            dots[y][x*2] = ':';
    }

    for ( i=0; i<SQUARE_ROWS; ++i )
        DBG(dots[i]);

    DBG("--------------------------------------------------------------------------------------------------------------------------------");
    DBG("");
    DBG("npp_lib_init_random_numbers took %.3lf ms", npp_elapsed(&start));
    DBG("");

#endif  /* NPP_DEBUG */
}


/* --------------------------------------------------------------------------
   Return a random 8-bit number from M_random_numbers
-------------------------------------------------------------------------- */
static unsigned char get_random_number()
{
    static int i=0;

    if ( M_random_initialized )
    {
        if ( i >= NPP_RANDOM_NUMBERS )  /* refill the pool with fresh numbers */
        {
            npp_lib_init_random_numbers();
            i = 0;
        }
        return M_random_numbers[i++];
    }
    else
    {
        WAR("Using get_random_number() before M_random_initialized");
        return rand() % 256;
    }
}


/* --------------------------------------------------------------------------
   Generate random string
   Generates FIPS-compliant random sequences (tested with Burp)
-------------------------------------------------------------------------- */
void npp_random(char *dest, size_t len)
{
const char *chars="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static unsigned since_seed=0;
    unsigned i;

#ifdef NPP_DEBUG
    struct timespec start;
#ifdef _WIN32
    clock_gettime_win(&start);
#else
    clock_gettime(MONOTONIC_CLOCK_NAME, &start);
#endif
#endif  /* NPP_DEBUG */

#ifdef NPP_CLIENT
    if ( since_seed > (unsigned)(G_pid % 246 + 10) )   /* seed every now and then */
#else
    if ( since_seed > (G_cnts_today.req % 246 + 10) )   /* seed every now and then */
#endif
//    if ( 1 )  /* test */
    {
        seed_rand();
        since_seed = 0;
    }
    else
    {
        ++since_seed;

        DDBG("since_seed = %u", since_seed);
    }

#ifdef NPP_DEBUG
    DBG_LINE;
#endif

    int r;

    for ( i=0; i<len; ++i )
    {
        /* source random numbers from two different sets: 'normal' and 'lucky' */

        if ( get_random_number() % 3 == 0 )
        {
            DDBG("i=%d lucky", i);
            r = get_random_number();
            while ( r > 247 ) r = get_random_number();   /* avoid modulo bias -- 62*4 - 1 */
        }
        else
        {
            DDBG("i=%d normal", i);
            r = rand() % 256;
            while ( r > 247 ) r = rand() % 256;
        }

        dest[i] = chars[r % 62];
    }

    dest[i] = EOS;

#ifdef NPP_DEBUG
    DBG_LINE;
    DBG("npp_random took %.3lf ms", npp_elapsed(&start));
#endif
}




/* ================================================================================================ */
/* Base64                                                                                           */
/* ================================================================================================ */
/*
    Based on base64.c - by Joe DF (joedf@ahkscript.org)
    Released under the MIT License
    I hope it's slightly faster
*/
/* --------------------------------------------------------------------------
   Base64 encode
-------------------------------------------------------------------------- */
int npp_b64_encode(char *dst, const unsigned char *src, size_t len)
{
static const char b64set[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned int i, j=0, k=0, block[3];

    for ( i=0; i<len; ++i )
    {
        block[j++] = *(src+i);

        if ( j==3 )
        {
            dst[k] = b64set[(block[0]&255)>>2];
            dst[k+1] = b64set[((block[0]&0x03)<<4)+((block[1]&0xF0)>>4)];
            dst[k+2] = b64set[((block[1]&0x0F)<<2)+((block[2]&0xC0)>>6)];
            dst[k+3] = b64set[block[2]&0x3F];
            j = 0;
            k += 4;
        }
    }

    if ( j )    /* trailing needed */
    {
        if ( j==1 )
            block[1] = 0;

        dst[k+0] = b64set[(block[0]&255)>>2];
        dst[k+1] = b64set[((block[0]&0x03)<<4)+((block[1]&0xF0)>>4)];

        if ( j==2 )
            dst[k+2] = b64set[((block[1]&0x0F)<<2)];
        else
            dst[k+2] = '=';

        dst[k+3] = '=';
        k += 4;
    }

    dst[k] = EOS;

    return k;
}


/* --------------------------------------------------------------------------
   Base64 decode
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
int npp_b64_decode(unsigned char *dst, const std::string& src_)
{
    const char *src = src_.c_str();
#else
int npp_b64_decode(unsigned char *dst, const char *src)
{
#endif
    unsigned int i=0, k=0, block[4];
    const char *p=src;

    while ( *p )
    {
        if (*p==43) block[i] = 62;
        else if (*p==47) block[i] = 63;
        else if (*p==61) block[i] = 64;
        else if ((*p>47) && (*p<58)) block[i] = *p + 4;
        else if ((*p>64) && (*p<91)) block[i] = *p - 65;
        else if ((*p>96) && (*p<123)) block[i] = (*p-97) + 26;
        else block[i] = 0;

        ++i;

        if ( i==4 )
        {
            dst[k+0] = ((block[0]&255)<<2)+((block[1]&0x30)>>4);

            if ( block[2] != 64 )
            {
                dst[k+1] = ((block[1]&0x0F)<<4)+((block[2]&0x3C)>>2);

                if ( block[3] != 64 )
                {
                    dst[k+2] = ((block[2]&0x03)<<6)+(block[3]);
                    k += 3;
                }
                else
                {
                    k += 2;
                }
            }
            else
            {
                ++k;
            }

            i = 0;
        }
    }

    return k;
}



#ifdef _WIN32   /* Windows */
/* --------------------------------------------------------------------------
   Windows port of getpid
-------------------------------------------------------------------------- */
int getpid()
{
    return GetCurrentProcessId();
}


/* --------------------------------------------------------------------------
   Windows port of clock_gettime
   https://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows
-------------------------------------------------------------------------- */
#define exp7           10000000     // 1E+7
#define exp9         1000000000     // 1E+9
#define w2ux 116444736000000000     // 1 Jan 1601 to 1 Jan 1970

static void unix_time(struct timespec *spec)
{
    __int64 wintime;
    GetSystemTimeAsFileTime((FILETIME*)&wintime);
    wintime -= w2ux;
    spec->tv_sec = wintime / exp7;
    spec->tv_nsec = wintime % exp7 * 100;
}

int clock_gettime_win(struct timespec *spec)
{
static struct timespec startspec;
static double ticks2nano;
static __int64 startticks, tps=0;
   __int64 tmp, curticks;

   QueryPerformanceFrequency((LARGE_INTEGER*)&tmp); // some strange system can possibly change freq?

   if ( tps != tmp )
   {
       tps = tmp; // init ONCE
       QueryPerformanceCounter((LARGE_INTEGER*)&startticks);
       unix_time(&startspec);
       ticks2nano = (double)exp9 / tps;
   }

   QueryPerformanceCounter((LARGE_INTEGER*)&curticks);
   curticks -= startticks;
   spec->tv_sec = startspec.tv_sec + (curticks / tps);
   spec->tv_nsec = startspec.tv_nsec + (double)(curticks % tps) * ticks2nano;
   if ( !(spec->tv_nsec < exp9) )
   {
       ++spec->tv_sec;
       spec->tv_nsec -= exp9;
   }

   return 0;
}


/* --------------------------------------------------------------------------
   Windows port of stpcpy
-------------------------------------------------------------------------- */
char *stpcpy(char *dest, const char *src)
{
#ifdef __cplusplus
    char *d=dest;
    const char *s=src;
#else
    register char *d=dest;
    register const char *s=src;
#endif

    do
        *d++ = *s;
    while (*s++ != '\0');

    return d-1;
}


/* --------------------------------------------------------------------------
   Windows port of stpncpy
-------------------------------------------------------------------------- */
char *stpncpy(char *dest, const char *src, size_t len)
{
#ifdef __cplusplus
    char *d=dest;
    const char *s=src;
#else
    register char *d=dest;
    register const char *s=src;
#endif
    size_t count=0;

    do
        *d++ = *s;
    while (*s++ != '\0' && ++count<len);

    if ( *(s-1) == EOS )
        return d-1;

    return d;
}


/* --------------------------------------------------------------------------
   Windows port of strnstr
-------------------------------------------------------------------------- */
char *strnstr(const char *haystack, const char *needle, size_t len)
{
    int i;
    size_t needle_len;

    if ( 0 == (needle_len = strnlen(needle, len)) )
        return (char*)haystack;

    for ( i=0; i<=(int)(len-needle_len); ++i )
    {
        if ( (haystack[0] == needle[0]) && (0 == strncmp(haystack, needle, needle_len)) )
            return (char*)haystack;

        ++haystack;
    }

    return NULL;
}
#endif  /* _WIN32 */




/* ================================================================================================ */
/* Main process (npp_app) only                                                                      */
/* ================================================================================================ */

#ifndef NPP_CLIENT
/* --------------------------------------------------------------------------
   Set date format, decimal & thousand separator
   for current connection and session
---------------------------------------------------------------------------*/
void npp_lib_set_formats(int ci, const char *lang)
{
    DBG("npp_lib_set_formats, lang [%s]", lang);

    /* date format */

    if ( 0==strcmp(lang, "EN-US") )
        G_connections[ci].formats |= NPP_DATE_US;
    else if ( 0==strcmp(lang, "EN-GB") || 0==strcmp(lang, "EN-AU") || 0==strcmp(lang, "FR-FR") || 0==strcmp(lang, "EN-IE") || 0==strcmp(lang, "ES-ES") || 0==strcmp(lang, "IT-IT") || 0==strcmp(lang, "PT-PT") || 0==strcmp(lang, "PT-BR") || 0==strcmp(lang, "ES-AR") )
        G_connections[ci].formats |= NPP_DATE_GB;
    else if ( 0==strcmp(lang, "PL-PL") || 0==strcmp(lang, "RU-RU") || 0==strcmp(lang, "DE-CH") || 0==strcmp(lang, "FR-CH") )
        G_connections[ci].formats |= NPP_DATE_PL;

    /* amount format */

    if ( 0==strcmp(lang, "EN-US") || 0==strcmp(lang, "EN-GB") || 0==strcmp(lang, "EN-AU") || 0==strcmp(lang, "TH-TH") )
    {
//        M_dsep = '.';
//        M_tsep = ',';
        G_connections[ci].formats |= NPP_NUMBER_DS_DOT;
        G_connections[ci].formats |= NPP_NUMBER_TS_COMMA;
    }
    else if ( 0==strcmp(lang, "PL-PL") || 0==strcmp(lang, "IT-IT") || 0==strcmp(lang, "NB-NO") || 0==strcmp(lang, "ES-ES") )
    {
//        M_dsep = ',';
//        M_tsep = '.';
        G_connections[ci].formats |= NPP_NUMBER_TS_DOT;
    }
    else
    {
//        M_dsep = ',';
//        M_tsep = ' ';
    }

    G_connections[ci].formats |= NPP_FORMATS_SET;

    SESSION.formats = G_connections[ci].formats;
}


/* --------------------------------------------------------------------------
   Format date
---------------------------------------------------------------------------*/
static bool lib_is_date_US(int ci)
{
    if ( IS_SESSION )
        return ((SESSION.formats & NPP_DATE_US) == NPP_DATE_US);
    else
        return ((G_connections[ci].formats & NPP_DATE_US) == NPP_DATE_US);
}


/* --------------------------------------------------------------------------
   Format date
---------------------------------------------------------------------------*/
static bool lib_is_date_GB(int ci)
{
    if ( IS_SESSION )
        return ((SESSION.formats & NPP_DATE_GB) == NPP_DATE_GB);
    else
        return ((G_connections[ci].formats & NPP_DATE_GB) == NPP_DATE_GB);
}


/* --------------------------------------------------------------------------
   Format date
---------------------------------------------------------------------------*/
static bool lib_is_date_PL(int ci)
{
    if ( IS_SESSION )
        return ((SESSION.formats & NPP_DATE_PL) == NPP_DATE_PL);
    else
        return ((G_connections[ci].formats & NPP_DATE_PL) == NPP_DATE_PL);
}


/* --------------------------------------------------------------------------
   Format date
---------------------------------------------------------------------------*/
char *npp_lib_fmt_date(int ci, short year, short month, short day)
{
static char dest[32];

    if ( lib_is_date_US(ci) )
        sprintf(dest, "%02hd/%02hd/%hd", month, day, year);
    else if ( lib_is_date_GB(ci) )
        sprintf(dest, "%02hd/%02hd/%hd", day, month, year);
    else if ( lib_is_date_PL(ci) )
        sprintf(dest, "%02hd.%02hd.%hd", day, month, year);
    else    /* NPP_DATE_DEFAULT */
        sprintf(dest, "%hd-%02hd-%02hd", year, month, day);

    return dest;
}


/* --------------------------------------------------------------------------
   Format amount
---------------------------------------------------------------------------*/
static char get_dsep(int ci)
{
    if ( IS_SESSION )
    {
        if ( (SESSION.formats & NPP_NUMBER_DS_COMMA) == NPP_NUMBER_DS_COMMA )
            return ',';
        else
            return '.';
    }
    else    /* no session -- get from current request */
    {
        if ( (G_connections[ci].formats & NPP_NUMBER_DS_COMMA) == NPP_NUMBER_DS_COMMA )
            return ',';
        else
            return '.';
    }
}


/* --------------------------------------------------------------------------
   Format amount
---------------------------------------------------------------------------*/
static char get_tsep(int ci)
{
    if ( IS_SESSION )
    {
        if ( (SESSION.formats & NPP_NUMBER_TS_SPACE) == NPP_NUMBER_TS_SPACE )
            return ' ';
        else if ( (SESSION.formats & NPP_NUMBER_TS_COMMA) == NPP_NUMBER_TS_COMMA )
            return ',';
        else
            return '.';
    }
    else    /* no session -- get from current request */
    {
        if ( (G_connections[ci].formats & NPP_NUMBER_TS_SPACE) == NPP_NUMBER_TS_SPACE )
            return ' ';
        else if ( (G_connections[ci].formats & NPP_NUMBER_TS_COMMA) == NPP_NUMBER_TS_COMMA )
            return ',';
        else
            return '.';
    }
}


/* --------------------------------------------------------------------------
   Format decimal amount
---------------------------------------------------------------------------*/
char *npp_lib_fmt_dec(int ci, double in_val)
{
static char dest[64];

    char    in_val_str[64];
    int     i, j=0;
    bool    minus=FALSE;

    sprintf(in_val_str, "%0.2lf", in_val);

    if ( in_val_str[0] == '-' )   /* change to UTF-8 minus sign */
    {
        strcpy(dest, "− ");
        j = 4;
        minus = TRUE;
    }

    int len = strlen(in_val_str);

    char dsep = get_dsep(ci);   /* decimal separator */
    char tsep = get_tsep(ci);   /* thousand separator */

    for ( i=(j?1:0); i<len; ++i, ++j )
    {
        if ( in_val_str[i]=='.' && dsep!='.' )   /* replace decimal separator */
        {
            dest[j] = dsep;
            continue;
        }
        else if ( ((!minus && i) || (minus && i>1)) && !((len-i)%3) && len-i > 3 && in_val_str[i] != ' ' && in_val_str[i-1] != ' ' && in_val_str[i-1] != '-' )
        {
            dest[j++] = tsep;    /* extra character */
        }
        dest[j] = in_val_str[i];
    }

    dest[j] = EOS;

    return dest;
}


/* --------------------------------------------------------------------------
   Format integer amount
---------------------------------------------------------------------------*/
char *npp_lib_fmt_int(int ci, long long in_val)
{
static char dest[256];

    char    in_val_str[256];
    int     i, j=0;
    bool    minus=FALSE;

    sprintf(in_val_str, "%lld", in_val);

//    DDBG("in_val_str [%s]", in_val_str);

    if ( in_val_str[0] == '-' )   /* change to UTF-8 minus sign */
    {
        strcpy(dest, "− ");
        j = 4;
        minus = TRUE;
    }

    int len = strlen(in_val_str);

    char tsep = get_tsep(ci);    /* thousand separator */

    for ( i=(j?1:0); i<len; ++i, ++j )
    {
        if ( ((!minus && i) || (minus && i>1)) && !((len-i)%3) )
            dest[j++] = tsep;
        dest[j] = in_val_str[i];
    }

    dest[j] = EOS;

    return dest;
}


/* --------------------------------------------------------------------------
   Notify admin via email
-------------------------------------------------------------------------- */
#ifdef NPP_CPP_STRINGS
void npp_notify_admin(const std::string& msg_)
{
    const char *msg = msg_.c_str();
#else
void npp_notify_admin(const char *msg)
{
#endif
#ifdef NPP_ADMIN_EMAIL

    char tag[16];

    npp_random(tag, 15);

    char message[NPP_MAX_LOG_STR_LEN+1];

    sprintf(message, "%s %s", msg, tag);

    ALWAYS_T(message);

    npp_email(NPP_ADMIN_EMAIL, "Admin Notification", message);

#endif  /* NPP_ADMIN_EMAIL */
}

#endif  /* NPP_CLIENT */


/* ================================================================================================ */
/* End of main process (npp_app) only part                                                          */
/* ================================================================================================ */
