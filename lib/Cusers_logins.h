/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-28 14:28:27, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#ifndef CUSERS_LOGINS_H
#define CUSERS_LOGINS_H


#include <npp_mysql.h>


typedef char USERS_LOGINS_SESSID[15+1];
typedef char USERS_LOGINS_UAGENT[250+1];
typedef char USERS_LOGINS_IP[45+1];
typedef char USERS_LOGINS_CSRFT[7+1];
typedef char USERS_LOGINS_CREATED[19+1];
typedef char USERS_LOGINS_LAST_USED[19+1];


typedef struct
{
    USERS_LOGINS_SESSID    sessid;
    USERS_LOGINS_UAGENT    uagent;
    USERS_LOGINS_IP        ip;
    int                    user_id;
    USERS_LOGINS_CSRFT     csrft;
    USERS_LOGINS_CREATED   created;
    USERS_LOGINS_LAST_USED last_used;
} USERS_LOGINS_REC;


class Cusers_logins : public USERS_LOGINS_REC, public Cdb
{
public:
    Cusers_logins();
    ~Cusers_logins();

    // Get the next record
    // Return false if end of record set
    bool Fetch();

    // Get one record by PK
    // Not Found will return false
    bool Get(const std::string& arg_sessid);

    // Insert record
    unsigned Insert();

    // Update record by PK
    void Update(const std::string& arg_sessid);

    // Delete record by PK
    void Delete(const std::string& arg_sessid);

    // Insert or update record by PK
    void Set(const std::string& arg_sessid);

    // Reset all values
    void Reset();


private:
static bool slots_[CDB_MAX_INSTANCES];

    USERS_LOGINS_SESSID k_sessid_;

    unsigned long sessid_len_;
    unsigned long uagent_len_;
    unsigned long ip_len_;
    unsigned long csrft_len_;
    MYSQL_TIME    t_created_;
    MYSQL_TIME    t_last_used_;

    unsigned long k_sessid_len_;

    my_bool sessid_is_null_;
    my_bool uagent_is_null_;
    my_bool ip_is_null_;
    my_bool user_id_is_null_;
    my_bool csrft_is_null_;
    my_bool created_is_null_;
    my_bool last_used_is_null_;

    MYSQL_BIND bndk_[1];
    MYSQL_BIND bndi_[8];
    MYSQL_BIND bndo_[7];

    void bindKey(MYSQL_STMT *s, const std::string& arg_sessid);
    void bindInput(MYSQL_STMT *s, bool withKey=false, const std::string& arg_sessid="");
    void bindOutput(MYSQL_STMT *s);
    void bindSetOutput();

    void genDTStrings();
};


#endif  /* CUSERS_LOGINS_H */
