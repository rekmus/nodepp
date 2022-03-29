/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-29 20:06:23, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#ifndef CUSERS_AVATARS_H
#define CUSERS_AVATARS_H


#include <npp_mysql.h>


typedef char USERS_AVATARS_AVATAR_NAME[120+1];
typedef char USERS_AVATARS_AVATAR_DATA[65535+1];


typedef struct
{
    int                       user_id;
    USERS_AVATARS_AVATAR_NAME avatar_name;
    USERS_AVATARS_AVATAR_DATA avatar_data;
    int                       avatar_len;
} USERS_AVATARS_REC;


class Cusers_avatars : public USERS_AVATARS_REC, public Cdb
{
public:
    unsigned long avatar_data_len;

    Cusers_avatars();
    ~Cusers_avatars();

    // Get the next record
    // Return false if end of record set
    bool Fetch();

    // Get one record by PK
    // Not Found will return false
    bool Get(int arg_user_id);

    // Insert record
    unsigned Insert();

    // Update record by PK
    void Update(int arg_user_id);

    // Delete record by PK
    void Delete(int arg_user_id);

    // Insert or update record by PK
    void Set(int arg_user_id);

    // Reset all values
    void Reset();


private:
static bool slots_[CDB_MAX_INSTANCES];

    int k_user_id_;

    unsigned long avatar_name_len_;


    my_bool user_id_is_null_;
    my_bool avatar_name_is_null_;
    my_bool avatar_data_is_null_;
    my_bool avatar_len_is_null_;

    MYSQL_BIND bndk_[1];
    MYSQL_BIND bndi_[5];
    MYSQL_BIND bndo_[4];

    void bindKey(MYSQL_STMT *s, int arg_user_id);
    void bindInput(MYSQL_STMT *s, bool withKey=false, int arg_user_id=0);
    void bindOutput(MYSQL_STMT *s);
    void bindSetOutput();

    void genDTStrings();
};


#endif  /* CUSERS_AVATARS_H */
