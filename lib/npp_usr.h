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
    Logged in users' functions
    nodepp.org

-------------------------------------------------------------------------- */

#ifndef NPP_USR_H
#define NPP_USR_H


/* --------------------------------------------------------------------------
   macros
-------------------------------------------------------------------------- */

#define NPP_PASSWD_HASH_BUFLEN          45                      /* SHA256 digest in base64 + EOS */


#define NPP_DB_UAGENT_LEN               250                     /* User-Agent length stored in ulogins table */
#define NPP_PASSWD_RESET_KEY_LEN        20                      /* password reset key length */


/* user status */

#define USER_STATUS_INACTIVE            (char)0
#define USER_STATUS_ACTIVE              (char)1
#define USER_STATUS_LOCKED              (char)2
#define USER_STATUS_PASSWORD_CHANGE     (char)3
#define USER_STATUS_DELETED             (char)9


#define NPP_COMMON_PASSWORDS_FILE       "passwords.txt"



/* APP-configurable */

#ifndef NPP_MIN_USERNAME_LEN
#define NPP_MIN_USERNAME_LEN            2                       /* minimum user name length */
#endif

#ifndef NPP_MIN_PASSWORD_LEN
#define NPP_MIN_PASSWORD_LEN            5                       /* minimum password length */
#endif

#ifndef MAX_ULA_BEFORE_FIRST_SLOW                               /* maximum unsuccessful login attempts before slowing down to 1 per minute */
#define MAX_ULA_BEFORE_FIRST_SLOW       10
#endif

#ifndef MAX_ULA_BEFORE_SECOND_SLOW                              /* maximum unsuccessful login attempts before slowing down to 1 per hour */
#define MAX_ULA_BEFORE_SECOND_SLOW      25
#endif

#ifndef MAX_ULA_BEFORE_THIRD_SLOW                               /* maximum unsuccessful login attempts before slowing down to 1 per day */
#define MAX_ULA_BEFORE_THIRD_SLOW       100
#endif

#ifndef MAX_ULA_BEFORE_LOCK                                     /* maximum unsuccessful login attempts before user lockout */
#define MAX_ULA_BEFORE_LOCK             1000
#endif

#ifndef NPP_DEFAULT_USER_AUTH_LEVEL
#define NPP_DEFAULT_USER_AUTH_LEVEL     AUTH_LEVEL_USER         /* default new user authorization level */
#endif

#ifndef NPP_USER_ACTIVATION_HOURS
#define NPP_USER_ACTIVATION_HOURS       48                      /* activate user account within */
#endif

#ifndef NPP_USER_KEEP_LOGGED_DAYS
#define NPP_USER_KEEP_LOGGED_DAYS       30                      /* ls cookie validity period */
#endif

#ifndef NPP_AUTH_SESSION_TIMEOUT
#define NPP_AUTH_SESSION_TIMEOUT        1800                    /* authenticated session timeout in seconds (120 for tests / 1800 live) */
#endif                                                          /* (it's really how long it stays in cache) */


#ifndef REFUSE_10_COMMON_PASSWORDS
#ifndef REFUSE_100_COMMON_PASSWORDS
#ifndef REFUSE_1000_COMMON_PASSWORDS
#ifndef REFUSE_10000_COMMON_PASSWORDS
#ifndef DONT_REFUSE_COMMON_PASSWORDS
#define DONT_REFUSE_COMMON_PASSWORDS
#endif
#endif
#endif
#endif
#endif

/* passwords' peppers */

#ifndef NPP_PEPPER_01
#define NPP_PEPPER_01                   "abcde"
#endif
#ifndef NPP_PEPPER_02
#define NPP_PEPPER_02                   "fghij"
#endif
#ifndef NPP_PEPPER_03
#define NPP_PEPPER_03                   "klmno"
#endif
#ifndef NPP_PEPPER_04
#define NPP_PEPPER_04                   "pqrst"
#endif
#ifndef NPP_PEPPER_05
#define NPP_PEPPER_05                   "uvwxy"
#endif


/* Node++ engine errors are 0 ... 99 */

/* ------------------------------------- */
/* errors -- red */

/* login */
#define ERR_INVALID_LOGIN               101
#define ERR_USERNAME_TOO_SHORT          102
#define ERR_USERNAME_CHARS              103
#define ERR_USERNAME_TAKEN              104
/* ------------------------------------- */
#define ERR_MAX_USR_LOGIN_ERROR         110
/* ------------------------------------- */
/* email */
#define ERR_EMAIL_EMPTY                 111
#define ERR_EMAIL_FORMAT                112
#define ERR_EMAIL_FORMAT_OR_EMPTY       113
#define ERR_EMAIL_TAKEN                 114
/* ------------------------------------- */
#define ERR_MAX_USR_EMAIL_ERROR         120
/* ------------------------------------- */
/* password */
#define ERR_INVALID_PASSWORD            121
#define ERR_PASSWORD_TOO_SHORT          122
#define ERR_IN_10_COMMON_PASSWORDS      123
#define ERR_IN_100_COMMON_PASSWORDS     124
#define ERR_IN_1000_COMMON_PASSWORDS    125
#define ERR_IN_10000_COMMON_PASSWORDS   126
/* ------------------------------------- */
#define ERR_MAX_USR_PASSWORD_ERROR      130
/* ------------------------------------- */
/* repeat password */
#define ERR_PASSWORD_DIFFERENT          131
/* ------------------------------------- */
#define ERR_MAX_USR_REPEAT_PASSWORD_ERROR 140
/* ------------------------------------- */
/* old password */
#define ERR_OLD_PASSWORD                141
/* ------------------------------------- */
#define ERR_MAX_USR_OLD_PASSWORD_ERROR  150
/* ------------------------------------- */
/* session / link / other */
#define ERR_SESSION_EXPIRED             151
#define ERR_LINK_BROKEN                 152
#define ERR_LINK_MAY_BE_EXPIRED         153
#define ERR_LINK_EXPIRED                154
#define ERR_LINK_TOO_MANY_TRIES         155
#define ERR_ROBOT                       156
#define ERR_WEBSITE_FIRST_LETTER        157
#define ERR_NOT_ACTIVATED               158
/* ------------------------------------- */
#define ERR_MAX_USR_ERROR               199
/* ------------------------------------- */

/* ------------------------------------- */
/* warnings -- yellow */

#define WAR_NO_EMAIL                    201
#define WAR_BEFORE_DELETE               202
#define WAR_ULA_FIRST                   203
#define WAR_ULA_SECOND                  204
#define WAR_ULA_THIRD                   205
#define WAR_PASSWORD_CHANGE             206
/* ------------------------------------- */
#define WAR_MAX_USR_WARNING             299
/* ------------------------------------- */

/* ------------------------------------- */
/* messages -- green */

#define MSG_WELCOME_NO_ACTIVATION       301
#define MSG_WELCOME_NEED_ACTIVATION     302
#define MSG_WELCOME_AFTER_ACTIVATION    303
#define MSG_USER_LOGGED_OUT             304
#define MSG_CHANGES_SAVED               305
#define MSG_REQUEST_SENT                306
#define MSG_PASSWORD_CHANGED            307
#define MSG_MESSAGE_SENT                308
#define MSG_PROVIDE_FEEDBACK            309
#define MSG_FEEDBACK_SENT               310
#define MSG_USER_ALREADY_ACTIVATED      311
#define MSG_ACCOUNT_DELETED             312
/* ------------------------------------- */
#define MSG_MAX_USR_MESSAGE             399
/* ------------------------------------- */


#define MSG_CAT_USR_LOGIN               "msgLogin"
#define MSG_CAT_USR_EMAIL               "msgEmail"
#define MSG_CAT_USR_PASSWORD            "msgPassword"
#define MSG_CAT_USR_REPEAT_PASSWORD     "msgPasswordRepeat"
#define MSG_CAT_USR_OLD_PASSWORD        "msgPasswordOld"


/* user authentication */

#ifndef NPP_USERS_BY_EMAIL
#ifndef NPP_USERS_BY_LOGIN
#define NPP_USERS_BY_LOGIN
#endif
#endif


#define NPP_MAX_AVATAR_SIZE             65536   /* MySQL's BLOB size */


#define SET_USER_STR(key, val)          npp_usr_set_str(ci, key, val)
#define SET_USR_STR(key, val)           npp_usr_set_str(ci, key, val)

#define GET_USER_STR(key, val)          npp_usr_get_str(ci, key, val)
#define GET_USR_STR(key, val)           npp_usr_get_str(ci, key, val)

#define SET_USER_INT(key, val)          npp_usr_set_int(ci, key, val)
#define SET_USR_INT(key, val)           npp_usr_set_int(ci, key, val)

#define GET_USER_INT(key, val)          npp_usr_get_int(ci, key, val)
#define GET_USR_INT(key, val)           npp_usr_get_int(ci, key, val)


/*
   Brute-force ls cookie attack protection.
   It essentially defines how many different IPs can take part in a botnet attack.
*/

#ifdef NPP_MEM_TINY
#define FAILED_LOGIN_CNT_SIZE           100
#elif defined NPP_MEM_MEDIUM
#define FAILED_LOGIN_CNT_SIZE           1000
#elif defined NPP_MEM_LARGE
#define FAILED_LOGIN_CNT_SIZE           10000
#elif defined NPP_MEM_XLARGE
#define FAILED_LOGIN_CNT_SIZE           10000
#elif defined NPP_MEM_XXLARGE
#define FAILED_LOGIN_CNT_SIZE           100000
#elif defined NPP_MEM_XXXLARGE
#define FAILED_LOGIN_CNT_SIZE           100000
#elif defined NPP_MEM_XXXXLARGE
#define FAILED_LOGIN_CNT_SIZE           100000
#else   /* NPP_MEM_SMALL -- default */
#define FAILED_LOGIN_CNT_SIZE           1000
#endif



/* --------------------------------------------------------------------------
   structures
-------------------------------------------------------------------------- */

typedef struct {
    char   ip[INET_ADDRSTRLEN];
    int    cnt;
    time_t when;
} failed_login_cnt_t;



/* --------------------------------------------------------------------------
   prototypes
-------------------------------------------------------------------------- */

#ifdef NPP_CPP_STRINGS
    char *npp_usr_name(const char *login, const char *email, const char *name, int user_id);
    char *npp_usr_name(const std::string& login, const std::string& email, const std::string& name, int user_id);

    int  npp_usr_get_str(int ci, const std::string& us_key, char *us_val);
    int  npp_usr_get_str(int ci, const std::string& us_key, std::string& us_val);
#endif


#ifdef __cplusplus
extern "C" {
#endif

    int  npp_usr_login(int ci);

#ifdef NPP_CPP_STRINGS
    int  npp_usr_password_quality(const std::string& passwd);
#else
    int  npp_usr_password_quality(const char *passwd);
#endif

    int  npp_usr_create_account(int ci);

#ifdef NPP_CPP_STRINGS
    int  npp_usr_add_user(int ci, bool use_qs, const std::string& login, const std::string& email, const std::string& name, const std::string& passwd, const std::string& phone, const std::string& lang, const std::string& about, char group_id, char auth_level, char status);
#else
    int  npp_usr_add_user(int ci, bool use_qs, const char *login, const char *email, const char *name, const char *passwd, const char *phone, const char *lang, const char *about, char group_id, char auth_level, char status);
#endif

    int  npp_usr_send_message(int ci);
    int  npp_usr_save_account(int ci);
    int  npp_usr_email_registered(int ci);

#ifndef NPP_CPP_STRINGS
    char *npp_usr_name(const char *login, const char *email, const char *name, int user_id);
#endif

    int  npp_usr_send_passwd_reset_email(int ci);

#ifdef NPP_CPP_STRINGS
    int  npp_usr_verify_passwd_reset_key(int ci, const std::string& linkkey, int *user_id);
#else
    int  npp_usr_verify_passwd_reset_key(int ci, const char *linkkey, int *user_id);
#endif

    int  npp_usr_activate(int ci);
    int  npp_usr_save_avatar(int ci, int user_id);
    int  npp_usr_get_avatar(int ci, int user_id);
    int  npp_usr_change_password(int ci);
    int  npp_usr_reset_password(int ci);
    void npp_usr_logout(int ci);

#ifdef NPP_CPP_STRINGS
    int  npp_usr_set_str(int ci, const std::string& us_key, const std::string& us_val);
#else
    int  npp_usr_set_str(int ci, const char *us_key, const char *us_val);
#endif

#ifndef NPP_CPP_STRINGS
    int  npp_usr_get_str(int ci, const char *us_key, char *us_val);
#endif

#ifdef NPP_CPP_STRINGS
    int  npp_usr_set_int(int ci, const std::string& us_key, int us_val);
#else
    int  npp_usr_set_int(int ci, const char *us_key, int us_val);
#endif

#ifdef NPP_CPP_STRINGS
    int  npp_usr_get_int(int ci, const std::string& us_key, int *us_val);
#else
    int  npp_usr_get_int(int ci, const char *us_key, int *us_val);
#endif

    /* for the engine */

    void libusr_init(void);
    int  libusr_luses_ok(int ci);
    void libusr_luses_close_timeouted(void);
    void libusr_luses_save_csrft(void);
    void libusr_luses_downgrade(int si, int ci, bool usr_logout);

#ifdef __cplusplus
}   // extern "C"
#endif


#endif  /* NPP_USR_H */
