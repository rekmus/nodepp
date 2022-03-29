/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-29 20:05:41, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#ifndef CUSERS_H
#define CUSERS_H


#include <npp_mysql.h>


typedef char USERS_LOGIN[30+1];
typedef char USERS_LOGIN_U[30+1];
typedef char USERS_EMAIL[120+1];
typedef char USERS_EMAIL_U[120+1];
typedef char USERS_NAME[120+1];
typedef char USERS_PHONE[30+1];
typedef char USERS_PASSWD1[44+1];
typedef char USERS_PASSWD2[44+1];
typedef char USERS_LANG[5+1];
typedef char USERS_ABOUT[250+1];
typedef char USERS_CREATED[19+1];
typedef char USERS_LAST_LOGIN[19+1];
typedef char USERS_ULA_TIME[19+1];


typedef struct
{
    int              id;
    USERS_LOGIN      login;
    USERS_LOGIN_U    login_u;
    USERS_EMAIL      email;
    USERS_EMAIL_U    email_u;
    USERS_NAME       name;
    USERS_PHONE      phone;
    USERS_PASSWD1    passwd1;
    USERS_PASSWD2    passwd2;
    USERS_LANG       lang;
    USERS_ABOUT      about;
    int              group_id;
    char             auth_level;
    char             status;
    USERS_CREATED    created;
    USERS_LAST_LOGIN last_login;
    int              visits;
    int              ula_cnt;
    USERS_ULA_TIME   ula_time;
} USERS_REC;


class Cusers : public USERS_REC, public Cdb
{
public:
    Cusers();
    ~Cusers();

    // Get the next record
    // Return false if end of record set
    bool Fetch();

    // Get one record by PK
    // Not Found will return false
    bool Get(int arg_id);

    // Insert record
    unsigned Insert();

    // Update record by PK
    void Update(int arg_id);

    // Delete record by PK
    void Delete(int arg_id);

    // Insert or update record by PK
    void Set(int arg_id);

    // Reset all values
    void Reset();


private:
static bool slots_[CDB_MAX_INSTANCES];

    int k_id_;

    unsigned long login_len_;
    unsigned long login_u_len_;
    unsigned long email_len_;
    unsigned long email_u_len_;
    unsigned long name_len_;
    unsigned long phone_len_;
    unsigned long passwd1_len_;
    unsigned long passwd2_len_;
    unsigned long lang_len_;
    unsigned long about_len_;
    MYSQL_TIME    t_created_;
    MYSQL_TIME    t_last_login_;
    MYSQL_TIME    t_ula_time_;


    my_bool id_is_null_;
    my_bool login_is_null_;
    my_bool login_u_is_null_;
    my_bool email_is_null_;
    my_bool email_u_is_null_;
    my_bool name_is_null_;
    my_bool phone_is_null_;
    my_bool passwd1_is_null_;
    my_bool passwd2_is_null_;
    my_bool lang_is_null_;
    my_bool about_is_null_;
    my_bool group_id_is_null_;
    my_bool auth_level_is_null_;
    my_bool status_is_null_;
    my_bool created_is_null_;
    my_bool last_login_is_null_;
    my_bool visits_is_null_;
    my_bool ula_cnt_is_null_;
    my_bool ula_time_is_null_;

    MYSQL_BIND bndk_[1];
    MYSQL_BIND bndi_[20];
    MYSQL_BIND bndo_[19];

    void bindKey(MYSQL_STMT *s, int arg_id);
    void bindInput(MYSQL_STMT *s, bool withKey=false, int arg_id=0);
    void bindOutput(MYSQL_STMT *s);
    void bindSetOutput();

    void genDTStrings();
};


#endif  /* CUSERS_H */
