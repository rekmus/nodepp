/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-28 15:17:27, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#ifndef CUSERS_MESSAGES_H
#define CUSERS_MESSAGES_H


#include <npp_mysql.h>


typedef char USERS_MESSAGES_EMAIL[120+1];
typedef char USERS_MESSAGES_MESSAGE[65535+1];
typedef char USERS_MESSAGES_CREATED[19+1];


typedef struct
{
    int                    user_id;
    int                    msg_id;
    USERS_MESSAGES_EMAIL   email;
    USERS_MESSAGES_MESSAGE message;
    USERS_MESSAGES_CREATED created;
} USERS_MESSAGES_REC;


class Cusers_messages : public USERS_MESSAGES_REC, public Cdb
{
public:
    Cusers_messages();
    ~Cusers_messages();

    // Get the next record
    // Return false if end of record set
    bool Fetch();

    // Get one record by PK
    // Not Found will return false
    bool Get(int arg_user_id, int arg_msg_id);

    // Insert record
    unsigned Insert();

    // Update record by PK
    void Update(int arg_user_id, int arg_msg_id);

    // Delete record by PK
    void Delete(int arg_user_id, int arg_msg_id);

    // Insert or update record by PK
    void Set(int arg_user_id, int arg_msg_id);

    // Reset all values
    void Reset();


private:
static bool slots_[CDB_MAX_INSTANCES];

    int k_user_id_;
    int k_msg_id_;

    unsigned long email_len_;
    unsigned long message_len_;
    MYSQL_TIME    t_created_;


    my_bool user_id_is_null_;
    my_bool msg_id_is_null_;
    my_bool email_is_null_;
    my_bool message_is_null_;
    my_bool created_is_null_;

    MYSQL_BIND bndk_[2];
    MYSQL_BIND bndi_[7];
    MYSQL_BIND bndo_[5];

    void bindKey(MYSQL_STMT *s, int arg_user_id, int arg_msg_id);
    void bindInput(MYSQL_STMT *s, bool withKey=false, int arg_user_id=0, int arg_msg_id=0);
    void bindOutput(MYSQL_STMT *s);
    void bindSetOutput();

    void genDTStrings();
};


#endif  /* CUSERS_MESSAGES_H */
