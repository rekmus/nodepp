/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-29 20:06:23, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#include "Cusers_avatars.h"


bool Cusers_avatars::slots_[CDB_MAX_INSTANCES]={0};


/* ---------------------------------------------------------------------------
   Constructor
--------------------------------------------------------------------------- */
Cusers_avatars::Cusers_avatars()
{
    setInstance(slots_);

    table_ = "users_avatars";

    columnList_ =   "user_id,"
                    "avatar_name,"
                    "avatar_data,"
                    "avatar_len";

    if ( !(s_=mysql_stmt_init(dbConn_)) )
        ThrowSQL("mysql_stmt_init");
    if ( !(sc_=mysql_stmt_init(dbConn_)) )
        ThrowSQL("mysql_stmt_init");
    if ( !(sGet_=mysql_stmt_init(dbConn_)) )
        ThrowSQL("mysql_stmt_init");
    if ( !(sUpdate_=mysql_stmt_init(dbConn_)) )
        ThrowSQL("mysql_stmt_init");
    if ( !(sInsert_=mysql_stmt_init(dbConn_)) )
        ThrowSQL("mysql_stmt_init");
    if ( !(sDelete_=mysql_stmt_init(dbConn_)) )
        ThrowSQL("mysql_stmt_init");
    if ( !(sSet_=mysql_stmt_init(dbConn_)) )
        ThrowSQL("mysql_stmt_init");

    avatar_data_len = 0;

    Reset();
}


/* ---------------------------------------------------------------------------
   Destructor
--------------------------------------------------------------------------- */
Cusers_avatars::~Cusers_avatars()
{
    mysql_stmt_close(s_);
    mysql_stmt_close(sGet_);
    mysql_stmt_close(sUpdate_);
    mysql_stmt_close(sInsert_);
    mysql_stmt_close(sDelete_);
    mysql_stmt_close(sSet_);

    slots_[instance_] = false;
}


/* ---------------------------------------------------------------------------
   Get the next record
   Return false if end of record set
--------------------------------------------------------------------------- */
bool Cusers_avatars::Fetch()
{
    int ret;

    ret = mysql_stmt_fetch(s_);

    if ( ret != 0 )
    {
        Reset();

        if ( ret == 1 || ret == MYSQL_NO_DATA )
            return false;
        else
        {
            Cdb::ThrowSQL("Cusers_avatars::Fetch | mysql_stmt_fetch");
            return false;
        }
    }

    genDTStrings();

    return true;
}


/* ---------------------------------------------------------------------------
   Get record by PK
   Not Found will return false
--------------------------------------------------------------------------- */
bool Cusers_avatars::Get(int arg_user_id)
{
    int ret;

    if ( firstGet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT user_id,avatar_name,avatar_data,avatar_len FROM users_avatars WHERE user_id=?");
        ret = mysql_stmt_prepare(sGet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_avatars::Get | mysql_stmt_prepare");
        firstGet_ = false;
    }

    bindKey(sGet_, arg_user_id);

    if ( mysql_stmt_execute(sGet_) )
        Cdb::ThrowSQL("Cusers_avatars::Get | mysql_stmt_execute");

    bindOutput(sGet_);

    if ( mysql_stmt_store_result(sGet_) )
        Cdb::ThrowSQL("Cusers_avatars::Get | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sGet_);

    if ( ret != 0 )
    {
        if ( ret == 1 || ret == MYSQL_NO_DATA )
            return false;
        else
            Cdb::ThrowSQL("Cusers_avatars::Get | mysql_stmt_fetch");
    }

    genDTStrings();

    return true;
}


/* ---------------------------------------------------------------------------
   Insert record
--------------------------------------------------------------------------- */
unsigned Cusers_avatars::Insert()
{
    int ret;

    if ( firstInsert_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "INSERT INTO users_avatars (user_id,avatar_name,avatar_data,avatar_len) VALUES (?,?,?,?)");
        ret = mysql_stmt_prepare(sInsert_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_avatars::Insert | mysql_stmt_prepare");
        firstInsert_ = false;
    }

    bindInput(sInsert_);

    ret = mysql_stmt_execute(sInsert_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_avatars::Insert | mysql_stmt_execute");

    return mysql_insert_id(dbConn_);
}


/* ---------------------------------------------------------------------------
   Update record by PK
--------------------------------------------------------------------------- */
void Cusers_avatars::Update(int arg_user_id)
{
    int ret;

    if ( firstUpdate_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "UPDATE users_avatars SET user_id=?,avatar_name=?,avatar_data=?,avatar_len=? WHERE user_id=?");
        ret = mysql_stmt_prepare(sUpdate_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_avatars::Update | mysql_stmt_prepare");
        firstUpdate_ = false;
    }

    bindInput(sUpdate_, true, arg_user_id);

    ret = mysql_stmt_execute(sUpdate_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_avatars::Update | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Delete record by PK
--------------------------------------------------------------------------- */
void Cusers_avatars::Delete(int arg_user_id)
{
    int ret;

    if ( firstDelete_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "DELETE FROM users_avatars WHERE user_id=?");
        ret = mysql_stmt_prepare(sDelete_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_avatars::Delete | mysql_stmt_prepare");
        firstDelete_ = false;
    }

    bindKey(sDelete_, arg_user_id);

    ret = mysql_stmt_execute(sDelete_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_avatars::Delete | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Insert or update record by PK
--------------------------------------------------------------------------- */
void Cusers_avatars::Set(int arg_user_id)
{
    int ret;

    if ( firstSet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT user_id FROM users_avatars WHERE user_id=?");
        ret = mysql_stmt_prepare(sSet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_avatars::Set | mysql_stmt_prepare");
        firstSet_ = false;
    }

    bindKey(sSet_, arg_user_id);

    if ( mysql_stmt_execute(sSet_) )
        Cdb::ThrowSQL("Cusers_avatars::Set | mysql_stmt_execute");

    bindSetOutput();

    if ( mysql_stmt_store_result(sSet_) )
        Cdb::ThrowSQL("Cusers_avatars::Set | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sSet_);

    if ( ret == 0 )   /* record existed */
    {
        Update(arg_user_id);
    }
    else if ( ret == 1 || ret == MYSQL_NO_DATA )   /* not found ==> insert new */
    {
        user_id = arg_user_id;

        Insert();
    }
    else
        Cdb::ThrowSQL("Cusers_avatars::Set | mysql_stmt_fetch");
}


/* ---------------------------------------------------------------------------
   Bind key values
--------------------------------------------------------------------------- */
void Cusers_avatars::bindKey(MYSQL_STMT *s, int arg_user_id)
{
    k_user_id_ = arg_user_id;

    memset(&bndk_, 0, sizeof(bndk_));

    bndk_[0].buffer_type = MYSQL_TYPE_LONG;
    bndk_[0].buffer = (char*)&k_user_id_;

    if ( mysql_stmt_bind_param(s, bndk_) )
        Cdb::ThrowSQL("Cusers_avatars::bindKey | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind input values
--------------------------------------------------------------------------- */
void Cusers_avatars::bindInput(MYSQL_STMT *s, bool withKey, int arg_user_id)
{
    avatar_name_len_ = strlen(avatar_name);


    memset(&bndi_, 0, sizeof(bndi_));

    bndi_[0].buffer_type = MYSQL_TYPE_LONG;
    bndi_[0].buffer = (char*)&user_id;

    bndi_[1].buffer_type = MYSQL_TYPE_STRING;
    bndi_[1].buffer = (char*)avatar_name;
    bndi_[1].length = &avatar_name_len_;

    bndi_[2].buffer_type = MYSQL_TYPE_BLOB;
    bndi_[2].buffer = (char*)avatar_data;
    bndi_[2].length = &avatar_data_len;

    bndi_[3].buffer_type = MYSQL_TYPE_LONG;
    bndi_[3].buffer = (char*)&avatar_len;

    if ( withKey )   /* after WHERE */
    {
        k_user_id_ = arg_user_id;

        bndi_[4].buffer_type = MYSQL_TYPE_LONG;
        bndi_[4].buffer = (char*)&k_user_id_;

    }

    if ( mysql_stmt_bind_param(s, bndi_) )
        Cdb::ThrowSQL("Cusers_avatars::bindInput | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind output values
--------------------------------------------------------------------------- */
void Cusers_avatars::bindOutput(MYSQL_STMT *s)
{
    memset(&bndo_, 0, sizeof(bndo_));

    bndo_[0].buffer_type = MYSQL_TYPE_LONG;
    bndo_[0].buffer = (char*)&user_id;
    bndo_[0].is_null = &user_id_is_null_;

    bndo_[1].buffer_type = MYSQL_TYPE_STRING;
    bndo_[1].buffer = (char*)avatar_name;
    bndo_[1].buffer_length = 121;
    bndo_[1].is_null = &avatar_name_is_null_;

    bndo_[2].buffer_type = MYSQL_TYPE_BLOB;
    bndo_[2].buffer = (char*)avatar_data;
    bndo_[2].buffer_length = 65536;
    bndo_[2].length = &avatar_data_len;
    bndo_[2].is_null = &avatar_data_is_null_;

    bndo_[3].buffer_type = MYSQL_TYPE_LONG;
    bndo_[3].buffer = (char*)&avatar_len;
    bndo_[3].is_null = &avatar_len_is_null_;

    if ( mysql_stmt_bind_result(s, bndo_) )
        Cdb::ThrowSQL("Cusers_avatars::bindOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Bind output value for Set
--------------------------------------------------------------------------- */
void Cusers_avatars::bindSetOutput()
{
static int user_id;    /* to be scrapped anyway */

    memset(&bndso_, 0, sizeof(bndso_));

    bndso_[0].buffer_type = MYSQL_TYPE_LONG;
    bndso_[0].buffer = (char*)&user_id;

    if ( mysql_stmt_bind_result(sSet_, bndso_) )
        Cdb::ThrowSQL("Cusers_avatars::bindSetOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Generate date-time strings
--------------------------------------------------------------------------- */
void Cusers_avatars::genDTStrings()
{
}


/* ---------------------------------------------------------------------------
   Reset (zero) public variables
--------------------------------------------------------------------------- */
void Cusers_avatars::Reset()
{
    user_id = 0;
    avatar_name[0] = EOS;
    avatar_data[0] = EOS;
    avatar_len = 0;
}
