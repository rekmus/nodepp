/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-28 15:15:25, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#ifndef CUSERS_GROUPS_H
#define CUSERS_GROUPS_H


#include <npp_mysql.h>


typedef char USERS_GROUPS_NAME[120+1];
typedef char USERS_GROUPS_ABOUT[250+1];


typedef struct
{
    int                     id;
    USERS_GROUPS_NAME       name;
    USERS_GROUPS_ABOUT      about;
    char                    auth_level;
} USERS_GROUPS_REC;


class Cusers_groups : public USERS_GROUPS_REC, public Cdb
{
public:
    Cusers_groups();
    ~Cusers_groups();

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

    unsigned long name_len_;
    unsigned long about_len_;


    my_bool id_is_null_;
    my_bool name_is_null_;
    my_bool about_is_null_;
    my_bool auth_level_is_null_;

    MYSQL_BIND bndk_[1];
    MYSQL_BIND bndi_[5];
    MYSQL_BIND bndo_[4];

    void bindKey(MYSQL_STMT *s, int arg_id);
    void bindInput(MYSQL_STMT *s, bool withKey=false, int arg_id=0);
    void bindOutput(MYSQL_STMT *s);
    void bindSetOutput();

};


#endif  /* CUSERS_GROUPS_H */
