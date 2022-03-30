/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-28 15:16:04, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#ifndef CUSERS_SETTINGS_H
#define CUSERS_SETTINGS_H


#include <npp_mysql.h>


typedef char USERS_SETTINGS_US_KEY[30+1];
typedef char USERS_SETTINGS_US_VAL[250+1];


typedef struct
{
    int                    user_id;
    USERS_SETTINGS_US_KEY  us_key;
    USERS_SETTINGS_US_VAL  us_val;
} USERS_SETTINGS_REC;


class Cusers_settings : public USERS_SETTINGS_REC, public Cdb
{
public:
    Cusers_settings();
    ~Cusers_settings();

    // Get the next record
    // Return false if end of record set
    bool Fetch();

    // Get one record by PK
    // Not Found will return false
    bool Get(int arg_user_id, const std::string& arg_us_key);

    // Insert record
    unsigned Insert();

    // Update record by PK
    void Update(int arg_user_id, const std::string& arg_us_key);

    // Delete record by PK
    void Delete(int arg_user_id, const std::string& arg_us_key);

    // Insert or update record by PK
    void Set(int arg_user_id, const std::string& arg_us_key);

    // Reset all values
    void Reset();


private:
static bool slots_[CDB_MAX_INSTANCES];

    int k_user_id_;
    USERS_SETTINGS_US_KEY k_us_key_;

    unsigned long us_key_len_;
    unsigned long us_val_len_;

    unsigned long k_us_key_len_;

    my_bool user_id_is_null_;
    my_bool us_key_is_null_;
    my_bool us_val_is_null_;

    MYSQL_BIND bndk_[2];
    MYSQL_BIND bndi_[5];
    MYSQL_BIND bndo_[3];

    void bindKey(MYSQL_STMT *s, int arg_user_id, const std::string& arg_us_key);
    void bindInput(MYSQL_STMT *s, bool withKey=false, int arg_user_id=0, const std::string& arg_us_key="");
    void bindOutput(MYSQL_STMT *s);
    void bindSetOutput();

};


#endif  /* CUSERS_SETTINGS_H */
