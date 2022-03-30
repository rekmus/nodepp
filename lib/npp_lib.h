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

#ifndef NPP_LIB_H
#define NPP_LIB_H


/* --------------------------------------------------------------------------
   macros
-------------------------------------------------------------------------- */

#define ENDIANNESS_LITTLE               (char)0
#define ENDIANNESS_BIG                  (char)1

#define bswap32(x)                      ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))


#define NPP_LIB_STR_BUF                 4096
#define NPP_LIB_STR_CHECK               4092


#if __cplusplus >= 201703L  /* C++17 and above */
//    #define NPP_CPP_STRINGS
#endif  /* C++17 or above only */


/* logs */

#define LOG_ALWAYS                      (char)0         /* print always */
#define LOG_ERR                         (char)1         /* print errors only */
#define LOG_WAR                         (char)2         /* print errors and warnings */
#define LOG_INF                         (char)3         /* print errors and warnings and info */
#define LOG_DBG                         (char)4         /* for debug mode -- most detailed */

#define ALWAYS(str, ...)                npp_log_write(LOG_ALWAYS, str, ##__VA_ARGS__)
#define ERR(str, ...)                   npp_log_write(LOG_ERR, str, ##__VA_ARGS__)
#define WAR(str, ...)                   npp_log_write(LOG_WAR, str, ##__VA_ARGS__)
#define INF(str, ...)                   npp_log_write(LOG_INF, str, ##__VA_ARGS__)
#define DBG(str, ...)                   npp_log_write(LOG_DBG, str, ##__VA_ARGS__)

#ifdef NPP_DEBUG
#define DDBG                            DBG
#else
#define DDBG(str, ...)
#endif

#define ALWAYS_T(str, ...)              npp_log_write_time(LOG_ALWAYS, str, ##__VA_ARGS__)
#define ERR_T(str, ...)                 npp_log_write_time(LOG_ERR, str, ##__VA_ARGS__)
#define WAR_T(str, ...)                 npp_log_write_time(LOG_WAR, str, ##__VA_ARGS__)
#define INF_T(str, ...)                 npp_log_write_time(LOG_INF, str, ##__VA_ARGS__)
#define DBG_T(str, ...)                 npp_log_write_time(LOG_DBG, str, ##__VA_ARGS__)

#define LOG_LINE                        "--------------------------------------------------"
#define LOG_LINE_N                      "--------------------------------------------------\n"
#define LOG_LINE_NN                     "--------------------------------------------------\n\n"
#define ALWAYS_LINE                     ALWAYS(LOG_LINE)
#define INF_LINE                        INF(LOG_LINE)
#define DBG_LINE                        DBG(LOG_LINE)

#define LOG_LINE_LONG                   "--------------------------------------------------------------------------------------------------"
#define LOG_LINE_LONG_N                 "--------------------------------------------------------------------------------------------------\n"
#define LOG_LINE_LONG_NN                "--------------------------------------------------------------------------------------------------\n\n"
#define ALWAYS_LINE_LONG                ALWAYS(LOG_LINE_LONG)
#define INF_LINE_LONG                   INF(LOG_LINE_LONG)
#define DBG_LINE_LONG                   DBG(LOG_LINE_LONG)

#define LOREM_IPSUM                     "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."

#define PARAM(param)                    (0==strcmp(label, param))

#define UTF8_ANY(c)                     (((unsigned char)c & 0x80) == 0x80)
#define UTF8_START(c)                   (UTF8_ANY(c) && ((unsigned char)c & 0x40) == 0x40)

#define COPY(dst, src, dst_len)         npp_safe_copy(dst, src, dst_len)


/* query string values' retrieval */

#define NPP_ESC_NONE                    '0'
#define NPP_ESC_SQL                     '1'
#define NPP_ESC_HTML                    '2'

#define QS_DONT_ESCAPE(param, val)      npp_lib_get_qs_param(ci, param, val, MAX_URI_VAL_LEN, NPP_ESC_NONE)
#define QS_SQL_ESCAPE(param, val)       npp_lib_get_qs_param(ci, param, val, MAX_URI_VAL_LEN, NPP_ESC_SQL)
#define QS_HTML_ESCAPE(param, val)      npp_lib_get_qs_param(ci, param, val, MAX_URI_VAL_LEN, NPP_ESC_HTML)

#define QS_TEXT_DONT_ESCAPE(param, val) npp_lib_get_qs_param(ci, param, val, 65535, NPP_ESC_NONE)
#define QS_TEXT_SQL_ESCAPE(param, val)  npp_lib_get_qs_param(ci, param, val, 65535, NPP_ESC_SQL)
#define QS_TEXT_HTML_ESCAPE(param, val) npp_lib_get_qs_param(ci, param, val, 65535, NPP_ESC_HTML)

#define QS_FILE(param, retlen, retfname) npp_lib_get_qs_param_multipart(ci, param, retlen, retfname)

#ifdef QS_DEF_HTML_ESCAPE
#define QS(param, val)                  npp_lib_get_qs_param(ci, param, val, MAX_URI_VAL_LEN, NPP_ESC_HTML)
#define QS1K(param, val)                npp_lib_get_qs_param(ci, param, val, 1023, NPP_ESC_HTML)
#define QS2K(param, val)                npp_lib_get_qs_param(ci, param, val, 2047, NPP_ESC_HTML)
#define QS4K(param, val)                npp_lib_get_qs_param(ci, param, val, 4095, NPP_ESC_HTML)
#define QS8K(param, val)                npp_lib_get_qs_param(ci, param, val, 8191, NPP_ESC_HTML)
#define QS16K(param, val)               npp_lib_get_qs_param(ci, param, val, 16383, NPP_ESC_HTML)
#define QS32K(param, val)               npp_lib_get_qs_param(ci, param, val, 32767, NPP_ESC_HTML)
#define QS64K(param, val)               npp_lib_get_qs_param(ci, param, val, 65535, NPP_ESC_HTML)
#define QS_TEXT(param, val)             npp_lib_get_qs_param(ci, param, val, 65535, NPP_ESC_HTML)
#endif
#ifdef QS_DEF_SQL_ESCAPE
#define QS(param, val)                  npp_lib_get_qs_param(ci, param, val, MAX_URI_VAL_LEN, NPP_ESC_SQL)
#define QS1K(param, val)                npp_lib_get_qs_param(ci, param, val, 1023, NPP_ESC_SQL)
#define QS2K(param, val)                npp_lib_get_qs_param(ci, param, val, 2047, NPP_ESC_SQL)
#define QS4K(param, val)                npp_lib_get_qs_param(ci, param, val, 4095, NPP_ESC_SQL)
#define QS8K(param, val)                npp_lib_get_qs_param(ci, param, val, 8191, NPP_ESC_SQL)
#define QS16K(param, val)               npp_lib_get_qs_param(ci, param, val, 16383, NPP_ESC_SQL)
#define QS32K(param, val)               npp_lib_get_qs_param(ci, param, val, 32767, NPP_ESC_SQL)
#define QS64K(param, val)               npp_lib_get_qs_param(ci, param, val, 65535, NPP_ESC_SQL)
#define QS_TEXT(param, val)             npp_lib_get_qs_param(ci, param, val, 65535, NPP_ESC_SQL)
#endif
#ifdef QS_DEF_DONT_ESCAPE
#define QS(param, val)                  npp_lib_get_qs_param(ci, param, val, MAX_URI_VAL_LEN, NPP_ESC_NONE)
#define QS1K(param, val)                npp_lib_get_qs_param(ci, param, val, 1023, NPP_ESC_NONE)
#define QS2K(param, val)                npp_lib_get_qs_param(ci, param, val, 2047, NPP_ESC_NONE)
#define QS4K(param, val)                npp_lib_get_qs_param(ci, param, val, 4095, NPP_ESC_NONE)
#define QS8K(param, val)                npp_lib_get_qs_param(ci, param, val, 8191, NPP_ESC_NONE)
#define QS16K(param, val)               npp_lib_get_qs_param(ci, param, val, 16383, NPP_ESC_NONE)
#define QS32K(param, val)               npp_lib_get_qs_param(ci, param, val, 32767, NPP_ESC_NONE)
#define QS64K(param, val)               npp_lib_get_qs_param(ci, param, val, 65535, NPP_ESC_NONE)
#define QS_TEXT(param, val)             npp_lib_get_qs_param(ci, param, val, 65535, NPP_ESC_NONE)
#endif

#define QSI(param, val)                 npp_lib_qsi(ci, param, val)
#define QSU(param, val)                 npp_lib_qsu(ci, param, val)
#define QSL(param, val)                 npp_lib_qsl(ci, param, val)
#define QSF(param, val)                 npp_lib_qsf(ci, param, val)
#define QSD(param, val)                 npp_lib_qsd(ci, param, val)
#define QSB(param, val)                 npp_lib_qsb(ci, param, val)


#define RES_HEADER(key, val)            npp_lib_res_header(ci, key, val)

#define RES_CONTENT_TYPE_FROM_FILE_EXTENSION(fname) (G_connections[ci].out_ctype = npp_lib_get_res_type(fname))


#define NPP_OPER_CONNECT                '0'
#define NPP_OPER_READ                   '1'
#define NPP_OPER_WRITE                  '2'
#define NPP_OPER_SHUTDOWN               '3'


#define NPP_RANDOM_NUMBERS              1024*64


#define NPP_MAX_SHM_SEGMENTS            100


#define NPP_IS_THIS_TRUE(c)             (c=='t' || c=='T' || c=='1')


#define npp_message(code)               npp_get_message(ci, code)
#define MSG(code)                       npp_get_message(ci, code)
#define MSG_CAT_GREEN(code)             npp_is_msg_main_cat(code, MSG_CAT_MESSAGE)
#define MSG_CAT_ORANGE(code)            npp_is_msg_main_cat(code, MSG_CAT_WARNING)
#define MSG_CAT_RED(code)               npp_is_msg_main_cat(code, MSG_CAT_ERROR)

#define OUT_MSG_DESCRIPTION(code)       npp_lib_send_msg_description(ci, code)

#define OUT_HTML_HEADER                 npp_out_html_header(ci)
#define OUT_HTML_FOOTER                 npp_out_html_footer(ci)
#define OUT_SNIPPET(name)               npp_out_snippet(ci, name)
#define OUT_SNIPPET_MD(name)            npp_out_snippet_md(ci, name)

#define REQ_COOKIE(key, val)            npp_lib_get_cookie(ci, key, val)
#define RES_COOKIE(key, val, days)      npp_lib_set_cookie(ci, key, val, days)

#define STR(str)                        npp_lib_get_string(ci, str)


/* convenient & fast string building */

#define STRM_BEGIN(buf)                 G_strm=buf

#define STRMS(str)                      (G_strm = stpcpy(G_strm, str))

#ifdef NPP_CPP_STRINGS
    #define STRM(str, ...)                   NPP_CPP_STRINGS_STRM(str, ##__VA_ARGS__)
#else
#ifdef _MSC_VER /* Microsoft compiler */
    #define STRM(...)                        (sprintf(G_tmp, EXPAND_VA(__VA_ARGS__)), STRMS(G_tmp))
#else   /* GCC */
    #define STRMM(str, ...)                  (sprintf(G_tmp, str, __VA_ARGS__), STRMS(G_tmp))   /* STRM with multiple args */
    #define CHOOSE_STRM(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, NAME, ...) NAME          /* single or multiple? */
    #define STRM(...)                        CHOOSE_STRM(__VA_ARGS__, STRMM, STRMM, STRMM, STRMM, STRMM, STRMM, STRMM, STRMM, STRMM, STRMM, STRMM, STRMM, STRMM, STRMS)(__VA_ARGS__)
#endif  /* _MSC_VER */
#endif  /* NPP_CPP_STRINGS */

#define STRM_END                        *G_strm = EOS


/* date & number format bits */

/* date (000000XX) */

#define NPP_DATE_DEFAULT                0x00    /* year first, dash separated */
#define NPP_DATE_US                     0x01    /* month first, slash separated */
#define NPP_DATE_GB                     0x02    /* day first, slash separated */
#define NPP_DATE_PL                     0x03    /* day first, dot separated */

/* number */

#define NPP_NUMBER_DS_COMMA             0x00    /* decimal separator (00000X00) */
#define NPP_NUMBER_DS_DOT               0x04

#define NPP_NUMBER_TS_SPACE             0x00    /* thousand separator (000XX000) */
#define NPP_NUMBER_TS_COMMA             0x10
#define NPP_NUMBER_TS_DOT               0x20

/* set (X0000000) */

#define NPP_FORMATS_SET                 0x80

#define NPP_IS_FORMATS_SET              ((G_connections[ci].formats & NPP_FORMATS_SET) == NPP_FORMATS_SET)



/* format date */

#define DATE(year, month, day)          npp_lib_fmt_date(ci, year, month, day)

/* format amount */

#define AMT(val)                        npp_lib_fmt_dec(ci, val)

/* format integer */

#define INT(val)                        npp_lib_fmt_int(ci, val)



/* socket errors */

#ifdef _WIN32
#define NPP_SOCKET_GET_ERROR            WSAGetLastError()
#define NPP_SOCKET_WOULD_BLOCK(e)       (e==WSAEWOULDBLOCK)
#define NPP_SOCKET_LOG_ERROR(e)         lib_log_win_socket_error(e)
#else
#define NPP_SOCKET_GET_ERROR            errno
#define NPP_SOCKET_WOULD_BLOCK(e)       (e==EWOULDBLOCK || e==EINPROGRESS)
#define NPP_SOCKET_LOG_ERROR(e)         DBG("errno = %d (%s)", e, strerror(e))
#endif



/* HTTP & RESTful calls */

#define CALL_HTTP_HEADER_KEY_LEN                    255
#define CALL_HTTP_HEADER_VAL_LEN                    1023

#define CALL_HTTP_MAX_HEADERS                       100
#define CALL_HTTP_HEADERS_RESET                     npp_call_http_headers_reset()
#define CALL_HTTP_HEADER_SET(key, val)              npp_call_http_header_set(key, val)
#define CALL_HTTP_HEADER_UNSET(key)                 npp_call_http_header_unset(key)

#define CALL_HTTP_RES_HEADER_LEN                    4095
#define CALL_HTTP_ADDRESSES_CACHE_SIZE              100

#define CALL_HTTP_DEFAULT_TIMEOUT                   10000     /* in ms -- to avoid blocking forever */

#define CALL_HTTP(req, res, method, url, keep)      npp_call_http(req, res, method, url, FALSE, keep)
#define CALL_REST(req, res, method, url, keep)      npp_call_http(req, res, method, url, TRUE, keep)

#ifndef NPP_SILGY_COMPATIBILITY
#ifndef JSON_NO_AUTO_AMPERSANDS
#define CALL_REST_HTTP(req, res, method, url, keep) npp_call_http(req, res, method, url, FALSE, keep)
#define CALL_REST_JSON(req, res, method, url, keep) npp_call_http(req, res, method, url, TRUE, keep)
#endif  /* JSON_NO_AUTO_AMPERSANDS */
#endif  /* NPP_SILGY_COMPATIBILITY */

#define CALL_HTTP_DISCONNECT                        npp_call_http_disconnect()

#define CALL_HTTP_STATUS                            G_call_http_status
#define CALL_REST_STATUS                            CALL_HTTP_STATUS
#define CALL_HTTP_CONTENT_TYPE                      G_call_http_content_type
#define CALL_REST_CONTENT_TYPE                      CALL_HTTP_CONTENT_TYPE
#define CALL_HTTP_STATUS_OK                         (G_call_http_status>=200 && G_call_http_status<=204)
#define CALL_REST_STATUS_OK                         CALL_HTTP_STATUS_OK
#define CALL_HTTP_RESPONSE_LEN                      G_call_http_res_len


/* JSON */

#define NPP_JSON_STRING                     's'
#define NPP_JSON_INTEGER                    'i'
#define NPP_JSON_UNSIGNED                   'u'
#define NPP_JSON_LONG                       'l'
#define NPP_JSON_FLOAT                      'f'
#define NPP_JSON_DOUBLE                     'd'
#define NPP_JSON_BOOL                       'b'
#define NPP_JSON_RECORD                     'r'
#define NPP_JSON_ARRAY                      'a'

#ifdef NPP_MEM_TINY
#define NPP_JSON_POOL_SIZE                  100     /* for storing sub-JSONs */
#else
#define NPP_JSON_POOL_SIZE                  1000    /* for storing sub-JSONs */
#endif

#define NPP_JSON_MAX_FLOAT_LEN              8
#define NPP_JSON_PRETTY_INDENT              "    "


#ifndef NPP_SILGY_COMPATIBILITY
#ifndef JSON_NO_AUTO_AMPERSANDS

#define JSON_TO_STRING(json)                lib_json_to_string(json)
#define JSON_TO_STRING_PRETTY(json)         lib_json_to_string_pretty(json)
#define JSON_FROM_STRING(json, str)         lib_json_from_string(json, str, 0, 0)

#define JSON_ADD_STR(json, name, value)     lib_json_add_str(json, name, -1, value)
#define JSON_ADD_STR_A(json, i, value)      lib_json_add_str(json, "", i, value)
#define JSON_ADD_INT(json, name, value)     lib_json_add_int(json, name, -1, value)
#define JSON_ADD_INT_A(json, i, value)      lib_json_add_int(json, "", i, value)
#define JSON_ADD_UINT(json, name, value)    lib_json_add_uint(json, name, -1, value)
#define JSON_ADD_UINT_A(json, i, value)     lib_json_add_uint(json, "", i, value)
#define JSON_ADD_LONG(json, name, value)    lib_json_add_long(json, name, -1, value)
#define JSON_ADD_LONG_A(json, i, value)     lib_json_add_long(json, "", i, value)
#define JSON_ADD_FLOAT(json, name, value)   lib_json_add_float(json, name, -1, value)
#define JSON_ADD_FLOAT_A(json, i, value)    lib_json_add_float(json, "", i, value)
#define JSON_ADD_DOUBLE(json, name, value)  lib_json_add_double(json, name, -1, value)
#define JSON_ADD_DOUBLE_A(json, i, value)   lib_json_add_double(json, "", i, value)
#define JSON_ADD_BOOL(json, name, value)    lib_json_add_bool(json, name, -1, value)
#define JSON_ADD_BOOL_A(json, i, value)     lib_json_add_bool(json, "", i, value)

#define JSON_ADD_RECORD(json, name, value)  lib_json_add_record(json, name, 0, value, FALSE)
#define JSON_ADD_RECORD_A(json, i, value)   lib_json_add_record(json, "", i, value, FALSE)

#define JSON_ADD_ARRAY(json, name, value)   lib_json_add_record(json, name, 0, value, TRUE)
#define JSON_ADD_ARRAY_A(json, i, value)    lib_json_add_record(json, "", i, value, TRUE)

#define JSON_PRESENT(json, name)            lib_json_present(json, name)

#ifdef NPP_JSON_V1  /* old version */

#define JSON_GET_STR(json, name)            lib_json_get_str(json, name, -1)
#define JSON_GET_STR_A(json, i)             lib_json_get_str(json, NULL, i)
#define JSON_GET_INT(json, name)            lib_json_get_int(json, name, -1)
#define JSON_GET_INT_A(json, i)             lib_json_get_int(json, NULL, i)
#define JSON_GET_UINT(json, name)           lib_json_get_uint(json, name, -1)
#define JSON_GET_UINT_A(json, i)            lib_json_get_uint(json, NULL, i)
#define JSON_GET_LONG(json, name)           lib_json_get_long(json, name, -1)
#define JSON_GET_LONG_A(json, i)            lib_json_get_long(json, NULL, i)
#define JSON_GET_FLOAT(json, name)          lib_json_get_float(json, name, -1)
#define JSON_GET_FLOAT_A(json, i)           lib_json_get_float(json, NULL, i)
#define JSON_GET_DOUBLE(json, name)         lib_json_get_double(json, name, -1)
#define JSON_GET_DOUBLE_A(json, i)          lib_json_get_double(json, NULL, i)
#define JSON_GET_BOOL(json, name)           lib_json_get_bool(json, name, -1)
#define JSON_GET_BOOL_A(json, i)            lib_json_get_bool(json, NULL, i)

#else   /* NOT NPP_JSON_V1 = new version */

#define JSON_GET_STR(json, name, value, maxlen) lib_json_get_str(json, name, -1, value, maxlen)
#define JSON_GET_STR_A(json, i, value, maxlen)  lib_json_get_str(json, "", i, value, maxlen)
#define JSON_GET_INT(json, name, value)     lib_json_get_int(json, name, -1, value)
#define JSON_GET_INT_A(json, i, value)      lib_json_get_int(json, "", i, value)
#define JSON_GET_UINT(json, name, value)    lib_json_get_uint(json, name, -1, value)
#define JSON_GET_UINT_A(json, i, value)     lib_json_get_uint(json, "", i, value)
#define JSON_GET_LONG(json, name, value)    lib_json_get_long(json, name, -1, value)
#define JSON_GET_LONG_A(json, i, value)     lib_json_get_long(json, "", i, value)
#define JSON_GET_FLOAT(json, name, value)   lib_json_get_float(json, name, -1, value)
#define JSON_GET_FLOAT_A(json, i, value)    lib_json_get_float(json, "", i, value)
#define JSON_GET_DOUBLE(json, name, value)  lib_json_get_double(json, name, -1, value)
#define JSON_GET_DOUBLE_A(json, i, value)   lib_json_get_double(json, "", i, value)
#define JSON_GET_BOOL(json, name, value)    lib_json_get_bool(json, name, -1, value)
#define JSON_GET_BOOL_A(json, i, value)     lib_json_get_bool(json, "", i, value)

#endif  /* NPP_JSON_V1 */

#define JSON_GET_RECORD(json, name, value)  lib_json_get_record(json, name, -1, value)
#define JSON_GET_RECORD_A(json, i, value)   lib_json_get_record(json, "", i, value)

#define JSON_GET_ARRAY(json, name, value)   lib_json_get_record(json, name, -1, value)
#define JSON_GET_ARRAY_A(json, i, value)    lib_json_get_record(json, "", i, value)


#define JSON_RESET(json)                    lib_json_reset(json)
#define JSON_COUNT(json)                    lib_json_count(json)

#define JSON_LOG_DBG(json, name)            lib_json_log_dbg(json, name)
#define JSON_LOG_INF(json, name)            lib_json_log_inf(json, name)

#endif  /* JSON_NO_AUTO_AMPERSANDS */
#endif  /* NPP_SILGY_COMPATIBILITY */


/* Admin Info */

#define AI_USERS_ALL                        'a'     /* all users */
#define AI_USERS_YAU                        'y'     /* yearly active */
#define AI_USERS_MAU                        'm'     /* monthly active */
#define AI_USERS_DAU                        'd'     /* daily active */


/* convenience */

#define urlencode                           npp_url_encode



/* APP-configurable */

#ifndef CALL_HTTP_MAX_RESPONSE_LEN
#define CALL_HTTP_MAX_RESPONSE_LEN          1048576 /* 1 MiB */
#endif

/* JSON */

#ifndef NPP_JSON_KEY_LEN
#define NPP_JSON_KEY_LEN                    31
#endif

#if NPP_JSON_KEY_LEN < 1
#undef NPP_JSON_KEY_LEN
#define NPP_JSON_KEY_LEN                    1       /* it can't be 0 */
#endif

#ifndef NPP_JSON_STR_LEN
#define NPP_JSON_STR_LEN                    255
#endif

#if NPP_JSON_STR_LEN < 31
#undef NPP_JSON_STR_LEN
#define NPP_JSON_STR_LEN                    31      /* the memory address as hex must fit in */
#endif

#ifndef NPP_JSON_MAX_ELEMS
#define NPP_JSON_MAX_ELEMS                  30      /* in one JSON struct */
#endif

#ifndef NPP_JSON_MAX_LEVELS
#ifdef NPP_MEM_TINY
#define NPP_JSON_MAX_LEVELS                 2
#else
#define NPP_JSON_MAX_LEVELS                 4
#endif
#endif  /* NPP_JSON_MAX_LEVELS */

#ifndef NPP_JSON_BUFSIZE
#define NPP_JSON_BUFSIZE                    65568
#endif

#if NPP_JSON_BUFSIZE < 256
#undef NPP_JSON_BUFSIZE
#define NPP_JSON_BUFSIZE                    256
#endif

/* npp_email */

#ifndef NPP_EMAIL_FROM_NAME
#define NPP_EMAIL_FROM_NAME                 NPP_APP_NAME
#endif

#ifndef NPP_EMAIL_FROM_USER
#define NPP_EMAIL_FROM_USER                 "noreply"
#endif

/* languages */

#ifndef NPP_DEFAULT_LANG
#define NPP_DEFAULT_LANG                    "EN-US"
#endif

/* messages */

#ifndef NPP_MAX_MESSAGE_LEN
#define NPP_MAX_MESSAGE_LEN                 255
#endif

#ifndef NPP_MAX_MESSAGES
#define NPP_MAX_MESSAGES                    1000
#endif

/* strings */

#ifndef NPP_STRINGS_SEP
#define NPP_STRINGS_SEP                     '|'
#endif

#ifndef NPP_MAX_STRING_LEN
#define NPP_MAX_STRING_LEN                  255
#endif

#ifndef NPP_MAX_STRINGS
#define NPP_MAX_STRINGS                     1000
#endif


/* overwrite for NPP_UPDATE */

#ifdef NPP_UPDATE

#undef CALL_HTTP_MAX_RESPONSE_LEN
#define CALL_HTTP_MAX_RESPONSE_LEN          2097152

#undef NPP_JSON_KEY_LEN
#define NPP_JSON_KEY_LEN                    31

#undef NPP_JSON_STR_LEN
#define NPP_JSON_STR_LEN                    255

#undef NPP_JSON_MAX_ELEMS
#define NPP_JSON_MAX_ELEMS                  50

#undef NPP_JSON_MAX_LEVELS
#define NPP_JSON_MAX_LEVELS                 4

#undef NPP_JSON_BUFSIZE
#define NPP_JSON_BUFSIZE                    65568

#endif  /* NPP_UPDATE */



/* --------------------------------------------------------------------------
   structures
-------------------------------------------------------------------------- */

/* languages */

typedef struct {
    char lang[NPP_LANG_LEN+1];
    int  first_index;
    int  next_lang_index;
} npp_lang_t;


/* messages */

typedef struct {
    int  code;
    char lang[NPP_LANG_LEN+1];
    char message[NPP_MAX_MESSAGE_LEN+1];
} npp_message_t;


/* strings */

typedef struct {
    char lang[NPP_LANG_LEN+1];
    char string_upper[NPP_MAX_STRING_LEN+1];
    char string_in_lang[NPP_MAX_STRING_LEN+1];
} npp_string_t;


/* single JSON element */

typedef struct {
    char    name[NPP_JSON_KEY_LEN+1];
    char    value[NPP_JSON_STR_LEN+1];
    char    type;
} json_elem_t;

/* JSON object */

typedef struct {
    int         cnt;
    char        array;
    json_elem_t rec[NPP_JSON_MAX_ELEMS];
} json_t;

typedef json_t JSON;


/* HTTP calls */

typedef struct {
    char    key[CALL_HTTP_HEADER_KEY_LEN+1];
    char    value[CALL_HTTP_HEADER_VAL_LEN+1];
} call_http_header_t;



/* --------------------------------------------------------------------------
   prototypes
-------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

    /* public */

    bool npp_lib_init(bool start_log, const char *log_prefix);
    void npp_lib_done(void);

#ifndef NPP_CPP_STRINGS
    void npp_add_message(int code, const char *lang, const char *message, ...);
#endif

    bool npp_open_db(void);
    void npp_close_db(void);

#ifdef NPP_CPP_STRINGS
    bool npp_file_exists(const std::string& fname);
#else
    bool npp_file_exists(const char *fname);
#endif

#ifdef NPP_CPP_STRINGS
    char *npp_get_exec_name(const std::string& path);
#else
    char *npp_get_exec_name(const char *path);
#endif

    void npp_update_time_globals(void);

#ifdef NPP_CPP_STRINGS
    char *npp_url_encode(const std::string& src);
#else
    char *npp_url_encode(const char *src);
#endif

#ifdef NPP_CPP_STRINGS
    char *npp_filter_strict(const std::string& src);
#else
    char *npp_filter_strict(const char *src);
#endif

#ifdef NPP_CPP_STRINGS
    char *npp_add_spaces(const std::string& src, int new_len);
#else
    char *npp_add_spaces(const char *src, int new_len);
#endif

#ifdef NPP_CPP_STRINGS
    char *npp_add_lspaces(const std::string& src, int new_len);
#else
    char *npp_add_lspaces(const char *src, int new_len);
#endif

#ifdef NPP_CPP_STRINGS
    char *npp_get_fname_from_path(const std::string& path);
#else
    char *npp_get_fname_from_path(const char *path);
#endif

#ifdef NPP_CPP_STRINGS
    char *npp_get_file_ext(const std::string& fname);
#else
    char *npp_get_file_ext(const char *fname);
#endif

    void date_str2rec(const char *str, date_t *rec);
    void date_rec2str(char *str, date_t *rec);

#ifdef NPP_CPP_STRINGS
    time_t time_http2epoch(const std::string& str);
    time_t time_db2epoch(const std::string& str);
#else
    time_t time_http2epoch(const char *str);
    time_t time_db2epoch(const char *str);
#endif

    char *time_epoch2http(time_t epoch);
    char *time_epoch2db(time_t epoch);

#ifdef NPP_CPP_STRINGS
    char *npp_sql_esc(const std::string& str);
#else
    char *npp_sql_esc(const char *str);
#endif

#ifdef NPP_CPP_STRINGS
    char *npp_html_esc(const std::string& str);
#else
    char *npp_html_esc(const char *str);
#endif

#ifdef NPP_CPP_STRINGS
    char *npp_html_unesc(const std::string& str);
#else
    char *npp_html_unesc(const char *str);
#endif

#ifdef NPP_CPP_STRINGS
    char *npp_upper(const std::string& str);
#else
    char *npp_upper(const char *str);
#endif

    double npp_elapsed(struct timespec *start);
    int  npp_get_memory(void);
    void npp_log_memory(void);

#ifndef NPP_CPP_STRINGS
    bool npp_read_param_str(const char *param, char *dest);
#endif

#ifdef NPP_CPP_STRINGS
    bool npp_read_param_int(const std::string& param, int *dest);
#else
    bool npp_read_param_int(const char *param, int *dest);
#endif

#ifdef NPP_CPP_STRINGS
    bool npp_email(const std::string& to, const std::string& subject, const std::string& message);
#else
    bool npp_email(const char *to, const char *subject, const char *message);
#endif

#ifdef NPP_CPP_STRINGS
//    bool npp_email_telnet(const std::string& to, const std::string& subject, const std::string& message, char *err_msg);
#else
//    bool npp_email_telnet(const char *to, const char *subject, const char *message, char *err_msg);
#endif

#ifdef NPP_CPP_STRINGS
    bool npp_email_attach(const std::string& to, const std::string& subject, const std::string& message, const std::string& att_name, const unsigned char *att_data, int att_data_len);
#else
    bool npp_email_attach(const char *to, const char *subject, const char *message, const char *att_name, const unsigned char *att_data, int att_data_len);
#endif

#ifdef NPP_CPP_STRINGS
    char *npp_convert(const std::string& src, const std::string& cp_from, const std::string& cp_to);
#else
    char *npp_convert(const char *src, const char *cp_from, const char *cp_to);
#endif

    char *npp_random(size_t len);

#ifndef NPP_CPP_STRINGS
    int  npp_b64_encode(char *dst, const unsigned char* src, size_t len);
#endif

#ifdef NPP_CPP_STRINGS
    int  npp_b64_decode(unsigned char *dst, const std::string& src);
#else
    int  npp_b64_decode(unsigned char *dst, const char *src);
#endif

#ifdef NPP_CPP_STRINGS
    char *npp_json_escape_string(const std::string& src);
#else
    char *npp_json_escape_string(const char *src);
#endif

#ifdef NPP_CPP_STRINGS
    char *npp_expand_env_path(const std::string& src);
#else
    char *npp_expand_env_path(const char *src);
#endif


#ifndef NPP_CLIENT  /* server processes only */

#ifdef NPP_CPP_STRINGS
    bool npp_add_host(const std::string& host, const std::string& res, const std::string& resmin, const std::string& snippets, char required_auth_level);
#else
    bool npp_add_host(const char *host, const char *res, const char *resmin, const char *snippets, char required_auth_level);
#endif

#ifdef NPP_CPP_STRINGS
    void npp_notify_admin(const std::string& msg);
#else
    void npp_notify_admin(const char *msg);
#endif

    void npp_admin_info(int ci, int users, admin_info_t ai[], int ai_cnt, bool header_n_footer);

#endif  /* NPP_CLIENT */

    /* public internal */

    void msleep(int msec);
    char *stp_right(char *str);
    bool strdigits(const char *src);
    int  npp_compare_strings(const void *a, const void *b);
    bool npp_read_conf(const char *file);

#ifdef NPP_CPP_STRINGS
    void npp_safe_copy(char *dst, const std::string& src, size_t dst_len);
#else
    void npp_safe_copy(char *dst, const char *src, size_t dst_len);
#endif

    char *npp_bin2hex(const unsigned char *src, size_t len);
    char *npp_today_gmt(void);

#ifdef NPP_CPP_STRINGS
    char *npp_render_md(char *dest, const std::string& src, size_t dest_len);
#else
    char *npp_render_md(char *dest, const char *src, size_t dest_len);
#endif

    void npp_sort_messages(void);
    bool npp_is_msg_main_cat(int code, const char *cat);
    void npp_lib_setnonblocking(int sock);
    void npp_call_http_headers_reset(void);

#ifdef NPP_CPP_STRINGS
    void npp_call_http_header_set(const std::string& key, const std::string& value);
#else
    void npp_call_http_header_set(const char *key, const char *value);
#endif

#ifdef NPP_CPP_STRINGS
    void npp_call_http_header_unset(const std::string& key);
#else
    void npp_call_http_header_unset(const char *key);
#endif

    void npp_sockaddr_to_string(struct sockaddr_in6 *in_addr, char *result);

#ifdef NPP_CPP_STRINGS
    bool npp_call_http(const void *req, void *res, const std::string& method, const std::string& url, bool json, bool keep);
#else
    bool npp_call_http(const void *req, void *res, const char *method, const char *url, bool json, bool keep);
#endif

    void npp_call_http_disconnect(void);
#ifdef _WIN32
    void lib_log_win_socket_error(int sockerr);
#endif
    bool npp_lib_check_ssl_error(int ssl_err);
    void npp_lib_get_app_dir(void);
    char npp_lib_get_res_type(const char *fname);
    void npp_lib_fmt_int_generic(char *stramt, long long in_amt);
    void npp_lib_fmt_dec_generic(char *stramt, double in_amt);
    void npp_lib_normalize_float(char *str);
    void npp_lib_escape_for_sql(char *dst, const char *str, int dst_len);
    void npp_lib_escape_for_html(char *dst, const char *str, int dst_len);

    int  lib_json_count(JSON *json);
    void lib_json_reset(JSON *json);
    char *lib_json_to_string(JSON *json);
    char *lib_json_to_string_pretty(JSON *json);

#ifdef NPP_CPP_STRINGS
    bool lib_json_from_string(JSON *json, const std::string& src, size_t len, unsigned level);
#else
    bool lib_json_from_string(JSON *json, const char *src, size_t len, unsigned level);
#endif

#ifdef NPP_CPP_STRINGS
    bool lib_json_add_str(JSON *json, const std::string& name, int i, const std::string& value);
    bool lib_json_add_int(JSON *json, const std::string& name, int i, int value);
    bool lib_json_add_uint(JSON *json, const std::string& name, int i, unsigned value);
    bool lib_json_add_long(JSON *json, const std::string& name, int i, long value);
    bool lib_json_add_float(JSON *json, const std::string& name, int i, float value);
    bool lib_json_add_double(JSON *json, const std::string& name, int i, double value);
    bool lib_json_add_bool(JSON *json, const std::string& name, int i, bool value);
    bool lib_json_add_record(JSON *json, const std::string& name, int i, JSON *json_sub, bool is_array);
    bool lib_json_present(JSON *json, const std::string& name);
    bool lib_json_get_int(JSON *json, const std::string& name, int i, int *retval);
    bool lib_json_get_uint(JSON *json, const std::string& name, int i, unsigned *retval);
    bool lib_json_get_long(JSON *json, const std::string& name, int i, long *retval);
    bool lib_json_get_float(JSON *json, const std::string& name, int i, float *retval);
    bool lib_json_get_double(JSON *json, const std::string& name, int i, double *retval);
    bool lib_json_get_bool(JSON *json, const std::string& name, int i, bool *retval);
    bool lib_json_get_record(JSON *json, const std::string& name, int i, JSON *json_sub);
#else   /* NOT NPP_CPP_STRINGS */
    bool lib_json_add_str(JSON *json, const char *name, int i, const char *value);
    bool lib_json_add_int(JSON *json, const char *name, int i, int value);
    bool lib_json_add_uint(JSON *json, const char *name, int i, unsigned value);
    bool lib_json_add_long(JSON *json, const char *name, int i, long value);
    bool lib_json_add_float(JSON *json, const char *name, int i, float value);
    bool lib_json_add_double(JSON *json, const char *name, int i, double value);
    bool lib_json_add_bool(JSON *json, const char *name, int i, bool value);
    bool lib_json_add_record(JSON *json, const char *name, int i, JSON *json_sub, bool is_array);
    bool lib_json_present(JSON *json, const char *name);
#ifdef NPP_JSON_V1  /* old version */
    char *lib_json_get_str(JSON *json, const char *name, int i);
    int  lib_json_get_int(JSON *json, const char *name, int i);
    unsigned lib_json_get_uint(JSON *json, const char *name, int i);
    long lib_json_get_long(JSON *json, const char *name, int i);
    float lib_json_get_float(JSON *json, const char *name, int i);
    double lib_json_get_double(JSON *json, const char *name, int i);
    bool lib_json_get_bool(JSON *json, const char *name, int i);
#else   /* new version */
    bool lib_json_get_str(JSON *json, const char *name, int i, char *retval, size_t maxlen);
    bool lib_json_get_int(JSON *json, const char *name, int i, int *retval);
    bool lib_json_get_uint(JSON *json, const char *name, int i, unsigned *retval);
    bool lib_json_get_long(JSON *json, const char *name, int i, long *retval);
    bool lib_json_get_float(JSON *json, const char *name, int i, float *retval);
    bool lib_json_get_double(JSON *json, const char *name, int i, double *retval);
    bool lib_json_get_bool(JSON *json, const char *name, int i, bool *retval);
#endif  /* NPP_JSON_V1 */
    bool lib_json_get_record(JSON *json, const char *name, int i, JSON *json_sub);
#endif  /* NPP_CPP_STRINGS */

    void lib_json_log_dbg(JSON *json, const char *name);
    void lib_json_log_inf(JSON *json, const char *name);

    void npp_get_byteorder(void);
    int  npp_minify(char *dest, const char *src);
    void date_inc(char *str, int days, int *dow);
    int  date_cmp(const char *str1, const char *str2);
    int  datetime_cmp(const char *str1, const char *str2);
    void npp_lib_read_conf(bool first);
    char *npp_lib_create_pid_file(const char *name);
    char *npp_lib_shm_create(unsigned bytes, int index);
    void npp_lib_shm_delete(int index);
    bool npp_log_start(const char *prefix, bool test, bool switching);

#ifndef NPP_CPP_STRINGS
    void npp_log_write(char level, const char *message, ...);
    void npp_log_write_time(char level, const char *message, ...);
#endif

#ifdef NPP_CPP_STRINGS
    void npp_log_long(const std::string& message, size_t len, const std::string& desc);
#else
    void npp_log_long(const char *message, size_t len, const char *desc);
#endif

    void npp_log_flush(void);
    void npp_lib_log_switch_to_stdout(void);
    void npp_lib_log_switch_to_file(void);
    void npp_log_finish(void);
    void npp_lib_init_random_numbers(void);

#ifdef _WIN32   /* Windows */
    int getpid(void);
    int clock_gettime_win(struct timespec *spec);
    char *stpcpy(char *dest, const char *src);
    char *stpncpy(char *dest, const char *src, size_t len);
    char *strnstr(const char *haystack, const char *needle, size_t len);
#endif  /* _WIN32 */

#ifndef NPP_CLIENT  /* server processes only */
    void npp_lib_set_formats(int ci, const char *lang);
    const char *npp_get_message(int ci, int code);
    void npp_out_html_header(int ci);
    void npp_out_html_footer(int ci);
    void npp_append_css(int ci, const char *fname, bool first);
    void npp_append_script(int ci, const char *fname, bool first);

    bool npp_lib_get_qs_param(int ci, const char *name, char *retbuf, size_t maxlen, char esc_type);
    unsigned char *npp_lib_get_qs_param_multipart(int ci, const char *name, size_t *retlen, char *retfname);

#ifdef NPP_CPP_STRINGS
    bool npp_lib_qsi(int ci, const std::string& name, int *retbuf);
    bool npp_lib_qsu(int ci, const std::string& name, unsigned *retbuf);
    bool npp_lib_qsl(int ci, const std::string& name, long *retbuf);
    bool npp_lib_qsf(int ci, const std::string& name, float *retbuf);
    bool npp_lib_qsd(int ci, const std::string& name, double *retbuf);
    bool npp_lib_qsb(int ci, const std::string& name, bool *retbuf);
#else
    bool npp_lib_qsi(int ci, const char *name, int *retbuf);
    bool npp_lib_qsu(int ci, const char *name, unsigned *retbuf);
    bool npp_lib_qsl(int ci, const char *name, long *retbuf);
    bool npp_lib_qsf(int ci, const char *name, float *retbuf);
    bool npp_lib_qsd(int ci, const char *name, double *retbuf);
    bool npp_lib_qsb(int ci, const char *name, bool *retbuf);
#endif

#ifdef NPP_CPP_STRINGS
    bool npp_lib_res_header(int ci, const std::string& hdr, const std::string& val);
#else
    bool npp_lib_res_header(int ci, const char *hdr, const char *val);
#endif

#ifndef NPP_CPP_STRINGS
    bool npp_lib_get_cookie(int ci, const char *key, char *value);
#endif

#ifdef NPP_CPP_STRINGS
    bool npp_lib_set_cookie(int ci, const std::string& key, const std::string& value, int days);
#else
    bool npp_lib_set_cookie(int ci, const char *key, const char *value, int days);
#endif

    void npp_lib_set_res_status(int ci, int status);

#ifdef NPP_CPP_STRINGS
    void npp_lib_set_res_content_type(int ci, const std::string& str);
#else
    void npp_lib_set_res_content_type(int ci, const char *str);
#endif

#ifndef NPP_CPP_STRINGS
    void npp_lib_set_res_location(int ci, const char *str, ...);
    void npp_lib_set_res_content_disposition(int ci, const char *str, ...);
#endif

    void npp_lib_send_msg_description(int ci, int code);

    void lib_sort_strings();

#ifdef NPP_CPP_STRINGS
    void npp_add_string(const std::string& lang, const std::string& str, const std::string& str_lang);
#else
    void npp_add_string(const char *lang, const char *str, const char *str_lang);
#endif

#ifdef NPP_CPP_STRINGS
    const char *npp_lib_get_string(int ci, const std::string& str);
#else
    const char *npp_lib_get_string(int ci, const char *str);
#endif

    void npp_set_tz(int ci);
    time_t npp_ua_time(int ci);
    char *npp_today_ua(int ci);
    char *npp_lib_fmt_date(int ci, short year, short month, short day);
    char *npp_lib_fmt_dec(int ci, double in_val);
    char *npp_lib_fmt_int(int ci, long long in_val);
    bool npp_csrft_ok(int ci);
    int  lib_compare_snippets(const void *a, const void *b);
    bool npp_lib_read_snippets(const char *host, int host_id, const char *directory, bool first_scan, const char *path);
    char *npp_get_snippet(int ci, const char *name);
    unsigned npp_get_snippet_len(int ci, const char *name);

#ifdef NPP_CPP_STRINGS
    void npp_out_snippet(int ci, const std::string& name);
#else
    void npp_out_snippet(int ci, const char *name);
#endif

#ifdef NPP_CPP_STRINGS
    void npp_out_snippet_md(int ci, const std::string& name);
#else
    void npp_out_snippet_md(int ci, const char *name);
#endif

    int  npp_lib_compare_sess_idx(const void *a, const void *b);

#ifdef NPP_MULTI_HOST
    int npp_lib_find_sess_idx_idx(int host_id, const char *sessid);
#else
    int npp_lib_find_sess_idx_idx(const char *sessid);
#endif

#endif  /* NPP_CLIENT */

#ifdef __cplusplus
}   // extern "C"
#endif



/* --------------------------------------------------------------------------
   templates
-------------------------------------------------------------------------- */

#ifdef NPP_CPP_STRINGS

extern "C" {
extern char         G_tmp[NPP_TMP_BUFSIZE];
extern bool         G_initialized;
extern char         *G_strm;
extern npp_message_t G_messages[NPP_MAX_MESSAGES];
extern int          G_messages_cnt;
extern int          G_logLevel;
extern FILE         *G_log_fd;
extern char         G_dt_string_gmt[128];
}   /* extern "C" */


/* --------------------------------------------------------------------------
   Convert std::string to char*
-------------------------------------------------------------------------- */
template<typename T>
auto cnv_variadic_arg(T&& t)
{
    if constexpr(std::is_same<std::remove_cv_t<std::remove_reference_t<T>>, std::string>::value)
        return std::forward<T>(t).c_str();
    else
        return std::forward<T>(t);
}


/* --------------------------------------------------------------------------
   Overloaded version for std::string
-------------------------------------------------------------------------- */
bool npp_lib_get_qs_param(int ci, const std::string& name, std::string& retbuf, size_t maxlen, char esc_type);


int  npp_b64_encode(char *dst, const unsigned char *src, size_t len);
int  npp_b64_encode(std::string& dst, const unsigned char* src, size_t len);

bool npp_lib_get_cookie(int ci, const std::string& key, char *value);
bool npp_lib_get_cookie(int ci, const std::string& key, std::string& value);

bool npp_read_param_str(const std::string& param, char *dest);
bool npp_read_param_str(const std::string& param, std::string& dest);

bool lib_json_get_str(JSON *json, const std::string& name, int i, char *retval, size_t maxlen);
bool lib_json_get_str(JSON *json, const std::string& name, int i, std::string& retval, size_t maxlen);


/* --------------------------------------------------------------------------
   Write to log
-------------------------------------------------------------------------- */
template<typename... Args>
void npp_log_write(char level, const std::string& message, Args&& ... args)
{
    if ( level > G_logLevel ) return;

    if ( LOG_ERR == level )
        fprintf(G_log_fd, "ERROR: ");
    else if ( LOG_WAR == level )
        fprintf(G_log_fd, "WARNING: ");

    /* compile message with arguments into buffer */

    char buffer[NPP_MAX_LOG_STR_LEN+1+64];

    std::snprintf(buffer, NPP_MAX_LOG_STR_LEN+64, message.c_str(), cnv_variadic_arg(std::forward<Args>(args))...);

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
template<typename... Args>
void npp_log_write_time(char level, const std::string& message, Args&& ... args)
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

    std::snprintf(buffer, NPP_MAX_LOG_STR_LEN+64, message.c_str(), cnv_variadic_arg(std::forward<Args>(args))...);

    /* write to the log file */

    fprintf(G_log_fd, "%s\n", buffer);

#ifdef NPP_DEBUG
    fflush(G_log_fd);
#else
    if ( G_logLevel >= LOG_DBG || level == LOG_ERR ) fflush(G_log_fd);
#endif
}


/* --------------------------------------------------------------------------
   Add error message
-------------------------------------------------------------------------- */
template<typename... Args>
void npp_add_message(int code, const std::string& lang, const std::string& message, Args&& ... args)
{
    if ( G_messages_cnt > NPP_MAX_MESSAGES-1 )
    {
        WAR("NPP_MAX_MESSAGES (%d) has been reached", NPP_MAX_MESSAGES);
        return;
    }

    /* compile message with arguments into buffer */

    char buffer[NPP_MAX_MESSAGE_LEN+1];

    std::snprintf(buffer, NPP_MAX_MESSAGE_LEN, message.c_str(), cnv_variadic_arg(std::forward<Args>(args))...);

    G_messages[G_messages_cnt].code = code;

    if ( lang.c_str() && lang.c_str()[0] )
        COPY(G_messages[G_messages_cnt].lang, npp_upper(lang.c_str()), NPP_LANG_LEN);
    else
        COPY(G_messages[G_messages_cnt].lang, npp_upper(NPP_DEFAULT_LANG), NPP_LANG_LEN);

    strcpy(G_messages[G_messages_cnt].message, buffer);

    ++G_messages_cnt;

    /* in case message was added after init */

    if ( G_initialized )
        npp_sort_messages();
}


/* --------------------------------------------------------------------------
   STRM
-------------------------------------------------------------------------- */
template<typename... Args>
void NPP_CPP_STRINGS_STRM(const std::string& str, Args&& ... args)
{
    std::snprintf(G_tmp, NPP_TMP_STR_LEN, str.c_str(), cnv_variadic_arg(std::forward<Args>(args))...);
    G_strm = stpcpy(G_strm, G_tmp);
}



#ifndef NPP_CLIENT  /* web app only */

extern "C" {
extern npp_connection_t G_connections[NPP_MAX_CONNECTIONS+2];

#ifdef NPP_SVC
void npp_svc_out_check_realloc(const char *str);
#else
void npp_eng_out_check_realloc(int ci, const char *str);
#endif
}   /* extern "C" */


/* --------------------------------------------------------------------------
   OUT
-------------------------------------------------------------------------- */
template<typename... Args>
void NPP_CPP_STRINGS_OUT(int ci, const std::string& str, Args&& ... args)
{
#ifdef NPP_SVC
    std::snprintf(G_tmp, NPP_TMP_STR_LEN, str.c_str(), cnv_variadic_arg(std::forward<Args>(args))...);
    npp_svc_out_check_realloc(G_tmp);
#else   /* NPP_APP */
    std::snprintf(G_tmp, NPP_TMP_STR_LEN, str.c_str(), cnv_variadic_arg(std::forward<Args>(args))...);
    npp_eng_out_check_realloc(ci, G_tmp);
#endif  /* NPP_SVC */
}


/* --------------------------------------------------------------------------
   Set location
-------------------------------------------------------------------------- */
template<typename... Args>
void npp_lib_set_res_location(int ci, const std::string& str, Args&& ... args)
{
    std::snprintf(G_connections[ci].location, NPP_MAX_URI_LEN, str.c_str(), cnv_variadic_arg(std::forward<Args>(args))...);
}


/* --------------------------------------------------------------------------
   Set response content disposition
-------------------------------------------------------------------------- */
template<typename... Args>
void npp_lib_set_res_content_disposition(int ci, const std::string& str, Args&& ... args)
{
    std::snprintf(G_connections[ci].cdisp, NPP_CONTENT_DISP_LEN, str.c_str(), cnv_variadic_arg(std::forward<Args>(args))...);
}

#endif  /* NPP_CLIENT */


#endif  /* NPP_CPP_STRINGS */


#endif  /* NPP_LIB_H */
