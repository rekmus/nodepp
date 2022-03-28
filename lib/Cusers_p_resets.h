/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-28 15:16:49, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#ifndef CUSERS_P_RESETS_H
#define CUSERS_P_RESETS_H


#include <npp_mysql.h>


typedef char USERS_P_RESETS_LINKKEY[20+1];
typedef char USERS_P_RESETS_CREATED[19+1];


typedef struct
{
    USERS_P_RESETS_LINKKEY linkkey;
    int                    user_id;
    USERS_P_RESETS_CREATED created;
    short                  tries;
} USERS_P_RESETS_REC;


class Cusers_p_resets : public USERS_P_RESETS_REC, public Cdb
{
public:
    Cusers_p_resets();
    ~Cusers_p_resets();

    // Get the next record
    // Return false if end of record set
    bool Fetch();

    // Get one record by PK
    // Not Found will return false
    bool Get(const std::string& arg_linkkey);

    // Insert record
    unsigned Insert();

    // Update record by PK
    void Update(const std::string& arg_linkkey);

    // Delete record by PK
    void Delete(const std::string& arg_linkkey);

    // Insert or update record by PK
    void Set(const std::string& arg_linkkey);

    // Reset all values
    void Reset();


private:
static bool slots_[CDB_MAX_INSTANCES];

    USERS_P_RESETS_LINKKEY k_linkkey_;

    unsigned long linkkey_len_;
    MYSQL_TIME    t_created_;

    unsigned long k_linkkey_len_;

    my_bool linkkey_is_null_;
    my_bool user_id_is_null_;
    my_bool created_is_null_;
    my_bool tries_is_null_;

    MYSQL_BIND bndk_[1];
    MYSQL_BIND bndi_[5];
    MYSQL_BIND bndo_[4];

    void bindKey(MYSQL_STMT *s, const std::string& arg_linkkey);
    void bindInput(MYSQL_STMT *s, bool withKey=false, const std::string& arg_linkkey="");
    void bindOutput(MYSQL_STMT *s);
    void bindSetOutput();

    void genDTStrings();
};


#endif  /* CUSERS_P_RESETS_H */
