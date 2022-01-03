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
    Silgy compatibility macros
    nodepp.org

-------------------------------------------------------------------------- */

#ifndef NPP_SILGY_H
#define NPP_SILGY_H

/*
#ifdef OLD_MACRO_NAME
#define NEW_MACRO_NAME                  OLD_MACRO_NAME
#else   OLD_MACRO_NAME might have been used in app code, so:
#define OLD_MACRO_NAME                  default value
#endif
*/

#ifdef APP_WEBSITE
#define NPP_APP_NAME                    APP_WEBSITE
#else
#define APP_WEBSITE                     "Node++ Web Application"
#endif

#ifdef APP_DOMAIN
#define NPP_APP_DOMAIN                  APP_DOMAIN
#else
#define APP_DOMAIN                      ""
#endif

#ifdef APP_DESCRIPTION
#define NPP_APP_DESCRIPTION             APP_DESCRIPTION
#endif

#ifdef APP_KEYWORDS
#define NPP_APP_KEYWORDS                APP_KEYWORDS
#endif

#ifdef DEF_RES_AUTH_LEVEL
#define NPP_REQUIRED_AUTH_LEVEL         DEF_RES_AUTH_LEVEL
#else
#define DEF_RES_AUTH_LEVEL              NPP_AUTH_NONE
#endif

#ifdef DEF_USER_AUTH_LEVEL
#define NPP_DEFAULT_USER_AUTH_LEVEL     DEF_USER_AUTH_LEVEL
#else
#define DEF_USER_AUTH_LEVEL             AUTH_LEVEL_USER
#endif

#ifdef USES_TIMEOUT
#define NPP_SESSION_TIMEOUT             USES_TIMEOUT
#else
#define USES_TIMEOUT                    600
#endif

#ifdef APP_ADMIN_EMAIL
#define NPP_ADMIN_EMAIL                 APP_ADMIN_EMAIL
#endif

#ifdef APP_CONTACT_EMAIL
#define NPP_CONTACT_EMAIL               APP_CONTACT_EMAIL
#endif

#ifdef APP_EMAIL_FROM_USER
#define NPP_EMAIL_FROM_USER             APP_EMAIL_FROM_USER
#else
#define EMAIL_FROM_USER                 "noreply"
#endif

#ifdef ALLOW_BEARER_AUTH
#define NPP_ALLOW_BEARER_AUTH
#endif

#ifdef ASYNC_MQ_MAXMSG
#define NPP_ASYNC_MQ_MAXMSG             ASYNC_MQ_MAXMSG
#else
#define ASYNC_MQ_MAXMSG                 10
#endif

#ifdef ASYNC_MAX_REQUESTS
#define NPP_ASYNC_MAX_REQUESTS          ASYNC_MAX_REQUESTS
#else
#define ASYNC_MAX_REQUESTS              10
#endif

#ifdef ASYNC_REQ_MSG_SIZE
#define NPP_ASYNC_REQ_MSG_SIZE          ASYNC_REQ_MSG_SIZE
#else
#define ASYNC_REQ_MSG_SIZE              8192
#endif

#ifdef ASYNC_RES_MSG_SIZE
#define NPP_ASYNC_RES_MSG_SIZE          ASYNC_RES_MSG_SIZE
#else
#define ASYNC_RES_MSG_SIZE              8192
#endif

#ifdef APP_ASYNC_ID
#define NPP_ASYNC_ID                    APP_ASYNC_ID
#endif

#ifdef APP_ERROR_PAGE
#define NPP_APP_CUSTOM_MESSAGE_PAGE
#endif

#ifdef APP_LOGIN_URI
#define NPP_LOGIN_URI                   APP_LOGIN_URI
#else
#define APP_LOGIN_URI                   "/login"
#endif

#ifdef APP_MAX_STATICS
#define NPP_MAX_STATICS                 APP_MAX_STATICS
#else
#define MAX_STATICS                     1000
#endif

#ifdef APP_SESID_LEN
#define NPP_SESSID_LEN                  APP_SESID_LEN
#else
#define SESID_LEN                       15
#endif

#ifdef COMPRESS_TRESHOLD
#define NPP_COMPRESS_TRESHOLD           COMPRESS_TRESHOLD
#else
#define COMPRESS_TRESHOLD               500
#endif

#ifdef COMPRESS_LEVEL
#define NPP_COMPRESS_LEVEL              COMPRESS_LEVEL
#else
#define COMPRESS_LEVEL                  Z_BEST_SPEED
#endif

#ifdef CONN_TIMEOUT
#define NPP_CONNECTION_TIMEOUT          CONN_TIMEOUT
#else
#define CONN_TIMEOUT                    180
#endif

#ifdef LUSES_TIMEOUT
#define NPP_AUTH_SESSION_TIMEOUT        LUSES_TIMEOUT
#else
#define LUSES_TIMEOUT                   1800
#endif

#ifdef MEM_TINY
#define NPP_MEM_TINY
#endif

#ifdef MEM_SMALL
#define NPP_MEM_SMALL
#endif

#ifdef MEM_MEDIUM
#define NPP_MEM_MEDIUM
#endif

#ifdef MEM_LARGE
#define NPP_MEM_LARGE
#endif

#ifdef MEM_XLARGE
#define NPP_MEM_XLARGE
#endif

#ifdef MEM_XXLARGE
#define NPP_MEM_XXLARGE
#endif

#ifdef MEM_XXXLARGE
#define NPP_MEM_XXXLARGE
#endif

#ifdef MEM_XXXXLARGE
#define NPP_MEM_XXXXLARGE
#endif

#ifdef HTTPS
#define NPP_HTTPS
#endif

#ifdef DBMYSQL
#define NPP_MYSQL
#endif

#ifdef USERS
#define NPP_USERS
#endif

#ifdef ASYNC
#define NPP_ASYNC
#endif

#ifdef ICONV
#define NPP_ICONV
#endif

#ifdef DUMP
#define NPP_DEBUG
#endif

#ifdef JSON_KEY_LEN
#define NPP_JSON_KEY_LEN                31
#endif

#ifdef JSON_VAL_LEN
#define NPP_JSON_STR_LEN                255
#endif

#ifdef JSON_MAX_ELEMS
#define NPP_JSON_MAX_ELEMS              15
#endif

#ifdef JSON_MAX_LEVELS
#define NPP_JSON_MAX_LEVELS             4
#endif

#ifdef JSON_BUFSIZE
#define NPP_JSON_BUFSIZE                65568
#endif

#ifdef BLACKLISTAUTOUPDATE
#define NPP_BLACKLIST_AUTO_UPDATE
#endif

#ifdef DOMAINONLY
#define NPP_DOMAIN_ONLY
#endif

#ifdef DONT_LOOK_FOR_INDEX
#define NPP_DONT_LOOK_FOR_INDEX
#endif

#ifdef DONT_NOTIFY_NEW_USER
#define NPP_DONT_NOTIFY_NEW_USER
#endif

#ifdef DONT_PASS_QS_ON_LOGIN_REDIRECTION
#define NPP_DONT_PASS_QS_ON_LOGIN_REDIRECTION
#endif

#ifdef DONT_RESCAN_RES
#define NPP_DONT_RESCAN_RES
#endif

#ifdef EXPIRES_STATICS
#define NPP_EXPIRES_STATICS             90
#endif

#ifdef EXPIRES_GENERATED
#define NPP_EXPIRES_GENERATED           30
#endif

#ifdef FD_MON_SELECT
#define NPP_FD_MON_SELECT
#endif

#ifdef FD_MON_POLL
#define NPP_FD_MON_POLL
#endif

#ifdef MAX_BLACKLIST
#define NPP_MAX_BLACKLIST               MAX_BLACKLIST
#else
#define MAX_BLACKLIST                   10000
#endif

#ifdef MAX_MESSAGES
#define NPP_MAX_MESSAGES                MAX_MESSAGES
#else
#define MAX_MESSAGES                    1000
#endif

#ifdef MAX_STRINGS
#define NPP_MAX_STRINGS                 MAX_STRINGS
#else
#define MAX_STRINGS                     1000
#endif

#ifdef MAX_PAYLOAD_SIZE
#define NPP_MAX_PAYLOAD_SIZE            MAX_PAYLOAD_SIZE
#else
#define MAX_PAYLOAD_SIZE                16777216
#endif

#ifdef MAX_WHITELIST
#define NPP_MAX_WHITELIST               MAX_WHITELIST
#else
#define MAX_WHITELIST                   1000
#endif

#ifdef MAX_HOSTS
#define NPP_MAX_HOSTS                   MAX_HOSTS
#else
#define MAX_HOSTS                       15
#endif

#ifdef HSTS_INCLUDE_SUBDOMAINS
#define NPP_HSTS_INCLUDE_SUBDOMAINS
#endif

#ifdef HSTS_MAX_AGE
#define NPP_HSTS_MAX_AGE                HSTS_MAX_AGE
#else
#define HSTS_MAX_AGE                    31536000
#endif

#ifdef MAX_RESOURCE_LEN
#define NPP_MAX_RESOURCE_LEN            MAX_RESOURCE_LEN
#else
#define MAX_RESOURCE_LEN                127
#endif

#ifdef NO_HSTS
#define NPP_NO_HSTS
#endif

#ifdef NO_IDENTITY
#define NPP_NO_IDENTITY
#endif

#ifdef NO_NOSNIFF
#define NPP_NO_NOSNIFF
#endif

#ifdef NO_SAMEORIGIN
#define NPP_NO_SAMEORIGIN
#endif

#ifdef OUTCHECKREALLOC
#define NPP_OUT_CHECK_REALLOC
#endif

#ifdef OUTCHECK
#define NPP_OUT_CHECK
#endif

#ifdef OUTFAST
#define NPP_OUT_FAST
#endif

#ifdef STR_001
#define NPP_PEPPER_01                   STR_001
#else
#define STR_001                         "abcde"
#endif

#ifdef STR_002
#define NPP_PEPPER_02                   STR_002
#else
#define STR_002                         "fghij"
#endif

#ifdef STR_003
#define NPP_PEPPER_03                   STR_003
#else
#define STR_003                         "klmno"
#endif

#ifdef STR_004
#define NPP_PEPPER_04                   STR_004
#else
#define STR_004                         "pqrst"
#endif

#ifdef STR_005
#define NPP_PEPPER_05                   STR_005
#else
#define STR_005                         "uvwxy"
#endif

#ifdef STRINGS_SEP
#define NPP_STRINGS_SEP                 STRINGS_SEP
#else
#define STRINGS_SEP                     '|'
#endif

#ifdef TMP_BUFSIZE
#define NPP_TMP_BUFSIZE                 TMP_BUFSIZE
#else
#define TMP_BUFSIZE                     131072
#endif

#ifdef DBMYSQLRECONNECT
#define NPP_MYSQL_RECONNECT
#endif

#ifdef APP_MIN_USERNAME_LEN
#define NPP_MIN_USERNAME_LEN            APP_MIN_USERNAME_LEN
#else
#define MIN_USERNAME_LEN                2
#endif

#ifdef APP_MIN_PASSWD_LEN
#define NPP_MIN_PASSWORD_LEN            APP_MIN_PASSWD_LEN
#else
#define MIN_PASSWORD_LEN                5
#endif

#ifdef USER_ACTIVATION_HOURS
#define NPP_USER_ACTIVATION_HOURS       USER_ACTIVATION_HOURS
#else
#define USER_ACTIVATION_HOURS           48
#endif

#ifdef USER_KEEP_LOGGED_DAYS
#define NPP_USER_KEEP_LOGGED_DAYS       USER_KEEP_LOGGED_DAYS
#else
#define USER_KEEP_LOGGED_DAYS           30
#endif

#ifdef USERSBYEMAIL
#define NPP_USERS_BY_EMAIL
#endif

#ifdef USERSBYLOGIN
#define NPP_USERS_BY_LOGIN
#endif

#ifndef MSG_FORMAT_JSON
#define NPP_MSG_DESCRIPTION_PIPES
#endif

#define NPP_MULTI_HOST

#ifndef ASYNC_EXCLUDE_AUS
#define NPP_ASYNC_INCLUDE_SESSION_DATA
#endif

#define PASSWD_RESET_KEY_LEN            NPP_PASSWD_RESET_KEY_LEN

#ifdef USES_SET_TZ
#define NPP_SET_TZ
#endif



/* convenience macros */

#define DT_TODAY                        DT_TODAY_GMT
#define OUTP_BEGIN                      STRM_BEGIN
#define OUTP                            STRM
#define OUTP_END                        STRM_END

#define RES_JSON                        NPP_CONTENT_TYPE_JSON

#define PROTOCOL                        NPP_PROTOCOL

#define ID                              REQ_ID


/* OpenSSL */

#define SHA1_DIGEST_SIZE                20

#define libSHA1                         SHA1
#define Base64encode(src, dst, size)    npp_b64_encode(src, (const unsigned char*)dst, size)

#include <openssl/md5.h>

#define md5(str)                        MD5((const unsigned char*)str, strlen(str), NULL)



#ifndef JSON_NO_AUTO_AMPERSANDS

#define JSON_TO_STRING(json)                lib_json_to_string(&json)
#define JSON_TO_STRING_PRETTY(json)         lib_json_to_string_pretty(&json)
#define JSON_FROM_STRING(json, str)         lib_json_from_string(&json, str, 0, 0)


#define JSON_ADD_STR(json, name, val)       lib_json_add(&json, name, val, 0, 0, 0, 0, JSON_STRING, -1)
#define JSON_ADD_STR_A(json, i, val)        lib_json_add(&json, NULL, val, 0, 0, 0, 0, JSON_STRING, i)
#define JSON_ADD_INT(json, name, val)       lib_json_add(&json, name, NULL, val, 0, 0, 0, JSON_INTEGER, -1)
#define JSON_ADD_INT_A(json, i, val)        lib_json_add(&json, NULL, NULL, val, 0, 0, 0, JSON_INTEGER, i)
#define JSON_ADD_UINT(json, name, val)      lib_json_add(&json, name, NULL, 0, val, 0, 0, JSON_UNSIGNED, -1)
#define JSON_ADD_UINT_A(json, i, val)       lib_json_add(&json, NULL, NULL, 0, val, 0, 0, JSON_UNSIGNED, i)
#define JSON_ADD_FLOAT(json, name, val)     lib_json_add(&json, name, NULL, 0, 0, val, 0, JSON_FLOAT, -1)
#define JSON_ADD_FLOAT_A(json, i, val)      lib_json_add(&json, NULL, NULL, 0, 0, val, 0, JSON_FLOAT, i)
#define JSON_ADD_DOUBLE(json, name, val)    lib_json_add(&json, name, NULL, 0, 0, 0, val, JSON_DOUBLE, -1)
#define JSON_ADD_DOUBLE_A(json, i, val)     lib_json_add(&json, NULL, NULL, 0, 0, 0, val, JSON_DOUBLE, i)
#define JSON_ADD_BOOL(json, name, val)      lib_json_add(&json, name, NULL, val, 0, 0, 0, JSON_BOOL, -1)
#define JSON_ADD_BOOL_A(json, i, val)       lib_json_add(&json, NULL, NULL, val, 0, 0, 0, JSON_BOOL, i)

#define JSON_ADD_RECORD(json, name, val)    lib_json_add_record(&json, name, &val, FALSE, -1)
#define JSON_ADD_RECORD_A(json, i, val)     lib_json_add_record(&json, NULL, &val, FALSE, i)

#define JSON_ADD_ARRAY(json, name, val)     lib_json_add_record(&json, name, &val, TRUE, -1)
#define JSON_ADD_ARRAY_A(json, i, val)      lib_json_add_record(&json, NULL, &val, TRUE, i)

#define JSON_PRESENT(json, name)            lib_json_present(&json, name)

#define JSON_GET_STR(json, name)            lib_json_get_str(&json, name, -1)
#define JSON_GET_STR_A(json, i)             lib_json_get_str(&json, NULL, i)
#define JSON_GET_INT(json, name)            lib_json_get_int(&json, name, -1)
#define JSON_GET_INT_A(json, i)             lib_json_get_int(&json, NULL, i)
#define JSON_GET_UINT(json, name)           lib_json_get_uint(&json, name, -1)
#define JSON_GET_UINT_A(json, i)            lib_json_get_uint(&json, NULL, i)
#define JSON_GET_FLOAT(json, name)          lib_json_get_float(&json, name, -1)
#define JSON_GET_FLOAT_A(json, i)           lib_json_get_float(&json, NULL, i)
#define JSON_GET_DOUBLE(json, name)         lib_json_get_double(&json, name, -1)
#define JSON_GET_DOUBLE_A(json, i)          lib_json_get_double(&json, NULL, i)
#define JSON_GET_BOOL(json, name)           lib_json_get_bool(&json, name, -1)
#define JSON_GET_BOOL_A(json, i)            lib_json_get_bool(&json, NULL, i)

#define JSON_GET_RECORD(json, name, val)    lib_json_get_record(&json, name, &val, -1)
#define JSON_GET_RECORD_A(json, i, val)     lib_json_get_record(&json, NULL, &val, i)

#define JSON_GET_ARRAY(json, name, val)     lib_json_get_record(&json, name, &val, -1)
#define JSON_GET_ARRAY_A(json, i, val)      lib_json_get_record(&json, NULL, &val, i)


#define JSON_RESET(json)                    lib_json_reset(&json)
#define JSON_COUNT(json)                    lib_json_count(&json)

#define JSON_LOG_DBG(json, name)            lib_json_log_dbg(&json, name)
#define JSON_LOG_INF(json, name)            lib_json_log_inf(&json, name)



#define CALL_REST_HTTP(req, res, method, url, keep) lib_rest_req((char*)req, (char*)res, method, url, FALSE, keep)
#define CALL_REST_JSON(req, res, method, url, keep) lib_rest_req(&req, &res, method, url, TRUE, keep)


#endif  /* JSON_NO_AUTO_AMPERSANDS */


/* functions */

#define upper                           npp_upper
#define log_long                        npp_log_long
#define lib_add_spaces                  npp_add_spaces
#define lib_add_lspaces                 npp_add_lspaces
#define npp_set_auth_level              npp_require_auth


/* UTF-8 */

#define CHAR_POUND                      "&#163;"
#define CHAR_COPYRIGHT                  "&#169;"
#define CHAR_N_ACUTE                    "&#324;"
#define CHAR_DOWN_ARROWHEAD1            "&#709;"
#define CHAR_LONG_DASH                  "&#8212;"
#define CHAR_EURO                       "&#8364;"
#define CHAR_UP                         "&#8593;"
#define CHAR_DOWN                       "&#8595;"
#define CHAR_MINUS                      "&#8722;"
#define CHAR_VEL                        "&#8744;"
#define CHAR_VERTICAL_ELLIPSIS          "&#8942;"
#define CHAR_COUNTERSINK                "&#9013;"
#define CHAR_DOUBLE_TRIANGLE_U          "&#9195;"
#define CHAR_DOUBLE_TRIANGLE_D          "&#9196;"
#define CHAR_DOWN_TRIANGLE_B            "&#9660;"
#define CHAR_DOWN_TRIANGLE_W            "&#9661;"
#define CHAR_CLOSE                      "&#10005;"
#define CHAR_HEAVY_PLUS                 "&#10133;"
#define CHAR_HEAVY_MINUS                "&#10134;"
#define CHAR_DOWN_ARROWHEAD2            "&#65088;"
#define CHAR_FULLW_PLUS                 "&#65291;"
#define CHAR_LESS_EQUAL                 "&#8804;"
#define CHAR_GREATER_EQUAL              "&#8805;"


/* globals */

#define conn    G_connections
#define uses    G_sessions
#define auses   G_app_session_data
#define usi     si
#define US      SESSION
#define AUS     SESSION_DATA
#define p4outp  G_strm
#define website app_name
#define ctype   out_ctype


#endif  /* NPP_SILGY_H */
