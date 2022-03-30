/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-28 15:16:04, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#include "Cusers_settings.h"


bool Cusers_settings::slots_[CDB_MAX_INSTANCES]={0};


/* ---------------------------------------------------------------------------
   Constructor
--------------------------------------------------------------------------- */
Cusers_settings::Cusers_settings()
{
    setInstance(slots_);

    table_ = "users_settings";

    columnList_ =   "user_id,"
                    "us_key,"
                    "us_val";

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

    Reset();
}


/* ---------------------------------------------------------------------------
   Destructor
--------------------------------------------------------------------------- */
Cusers_settings::~Cusers_settings()
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
bool Cusers_settings::Fetch()
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
            Cdb::ThrowSQL("Cusers_settings::Fetch | mysql_stmt_fetch");
            return false;
        }
    }

    return true;
}


/* ---------------------------------------------------------------------------
   Get record by PK
   Not Found will return false
--------------------------------------------------------------------------- */
bool Cusers_settings::Get(int arg_user_id, const std::string& arg_us_key)
{
    int ret;

    if ( firstGet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT user_id,us_key,us_val FROM users_settings WHERE user_id=? AND us_key=?");
        ret = mysql_stmt_prepare(sGet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_settings::Get | mysql_stmt_prepare");
        firstGet_ = false;
    }

    bindKey(sGet_, arg_user_id, arg_us_key);

    if ( mysql_stmt_execute(sGet_) )
        Cdb::ThrowSQL("Cusers_settings::Get | mysql_stmt_execute");

    bindOutput(sGet_);

    if ( mysql_stmt_store_result(sGet_) )
        Cdb::ThrowSQL("Cusers_settings::Get | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sGet_);

    if ( ret != 0 )
    {
        if ( ret == 1 || ret == MYSQL_NO_DATA )
            return false;
        else
            Cdb::ThrowSQL("Cusers_settings::Get | mysql_stmt_fetch");
    }

    return true;
}


/* ---------------------------------------------------------------------------
   Insert record
--------------------------------------------------------------------------- */
unsigned Cusers_settings::Insert()
{
    int ret;

    if ( firstInsert_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "INSERT INTO users_settings (user_id,us_key,us_val) VALUES (?,?,?)");
        ret = mysql_stmt_prepare(sInsert_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_settings::Insert | mysql_stmt_prepare");
        firstInsert_ = false;
    }

    bindInput(sInsert_);

    ret = mysql_stmt_execute(sInsert_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_settings::Insert | mysql_stmt_execute");

    return mysql_insert_id(dbConn_);
}


/* ---------------------------------------------------------------------------
   Update record by PK
--------------------------------------------------------------------------- */
void Cusers_settings::Update(int arg_user_id, const std::string& arg_us_key)
{
    int ret;

    if ( firstUpdate_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "UPDATE users_settings SET user_id=?,us_key=?,us_val=? WHERE user_id=? AND us_key=?");
        ret = mysql_stmt_prepare(sUpdate_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_settings::Update | mysql_stmt_prepare");
        firstUpdate_ = false;
    }

    bindInput(sUpdate_, true, arg_user_id, arg_us_key);

    ret = mysql_stmt_execute(sUpdate_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_settings::Update | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Delete record by PK
--------------------------------------------------------------------------- */
void Cusers_settings::Delete(int arg_user_id, const std::string& arg_us_key)
{
    int ret;

    if ( firstDelete_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "DELETE FROM users_settings WHERE user_id=? AND us_key=?");
        ret = mysql_stmt_prepare(sDelete_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_settings::Delete | mysql_stmt_prepare");
        firstDelete_ = false;
    }

    bindKey(sDelete_, arg_user_id, arg_us_key);

    ret = mysql_stmt_execute(sDelete_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_settings::Delete | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Insert or update record by PK
--------------------------------------------------------------------------- */
void Cusers_settings::Set(int arg_user_id, const std::string& arg_us_key)
{
    int ret;

    if ( firstSet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT user_id FROM users_settings WHERE user_id=? AND us_key=?");
        ret = mysql_stmt_prepare(sSet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_settings::Set | mysql_stmt_prepare");
        firstSet_ = false;
    }

    bindKey(sSet_, arg_user_id, arg_us_key);

    if ( mysql_stmt_execute(sSet_) )
        Cdb::ThrowSQL("Cusers_settings::Set | mysql_stmt_execute");

    bindSetOutput();

    if ( mysql_stmt_store_result(sSet_) )
        Cdb::ThrowSQL("Cusers_settings::Set | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sSet_);

    if ( ret == 0 )   /* record existed */
    {
        Update(arg_user_id, arg_us_key);
    }
    else if ( ret == 1 || ret == MYSQL_NO_DATA )   /* not found ==> insert new */
    {
        user_id = arg_user_id;
        strncpy(us_key, arg_us_key.c_str(), 30);
        us_key[30] = EOS;

        Insert();
    }
    else
        Cdb::ThrowSQL("Cusers_settings::Set | mysql_stmt_fetch");
}


/* ---------------------------------------------------------------------------
   Bind key values
--------------------------------------------------------------------------- */
void Cusers_settings::bindKey(MYSQL_STMT *s, int arg_user_id, const std::string& arg_us_key)
{
    k_user_id_ = arg_user_id;
    strncpy(k_us_key_, arg_us_key.c_str(), 30);
    k_us_key_[30] = EOS;

    k_us_key_len_ = strlen(k_us_key_);

    memset(&bndk_, 0, sizeof(bndk_));

    bndk_[0].buffer_type = MYSQL_TYPE_LONG;
    bndk_[0].buffer = (char*)&k_user_id_;

    bndk_[1].buffer_type = MYSQL_TYPE_STRING;
    bndk_[1].buffer = (char*)k_us_key_;
    bndk_[1].length = &k_us_key_len_;

    if ( mysql_stmt_bind_param(s, bndk_) )
        Cdb::ThrowSQL("Cusers_settings::bindKey | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind input values
--------------------------------------------------------------------------- */
void Cusers_settings::bindInput(MYSQL_STMT *s, bool withKey, int arg_user_id, const std::string& arg_us_key)
{
    us_key_len_ = strlen(us_key);
    us_val_len_ = strlen(us_val);

    memset(&bndi_, 0, sizeof(bndi_));

    bndi_[0].buffer_type = MYSQL_TYPE_LONG;
    bndi_[0].buffer = (char*)&user_id;

    bndi_[1].buffer_type = MYSQL_TYPE_STRING;
    bndi_[1].buffer = (char*)us_key;
    bndi_[1].length = &us_key_len_;

    bndi_[2].buffer_type = MYSQL_TYPE_STRING;
    bndi_[2].buffer = (char*)us_val;
    bndi_[2].length = &us_val_len_;

    if ( withKey )   /* after WHERE */
    {
        k_user_id_ = arg_user_id;
        strncpy(k_us_key_, arg_us_key.c_str(), 30);
        k_us_key_[30] = EOS;

        k_us_key_len_ = strlen(k_us_key_);

        bndi_[3].buffer_type = MYSQL_TYPE_LONG;
        bndi_[3].buffer = (char*)&k_user_id_;

        bndi_[4].buffer_type = MYSQL_TYPE_STRING;
        bndi_[4].buffer = (char*)k_us_key_;
        bndi_[4].length = &k_us_key_len_;

    }

    if ( mysql_stmt_bind_param(s, bndi_) )
        Cdb::ThrowSQL("Cusers_settings::bindInput | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind output values
--------------------------------------------------------------------------- */
void Cusers_settings::bindOutput(MYSQL_STMT *s)
{
    memset(&bndo_, 0, sizeof(bndo_));

    bndo_[0].buffer_type = MYSQL_TYPE_LONG;
    bndo_[0].buffer = (char*)&user_id;
    bndo_[0].is_null = &user_id_is_null_;

    bndo_[1].buffer_type = MYSQL_TYPE_STRING;
    bndo_[1].buffer = (char*)us_key;
    bndo_[1].buffer_length = 31;
    bndo_[1].is_null = &us_key_is_null_;

    bndo_[2].buffer_type = MYSQL_TYPE_STRING;
    bndo_[2].buffer = (char*)us_val;
    bndo_[2].buffer_length = 251;
    bndo_[2].is_null = &us_val_is_null_;

    if ( mysql_stmt_bind_result(s, bndo_) )
        Cdb::ThrowSQL("Cusers_settings::bindOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Bind output value for Set
--------------------------------------------------------------------------- */
void Cusers_settings::bindSetOutput()
{
static int user_id;    /* to be scrapped anyway */

    memset(&bndso_, 0, sizeof(bndso_));

    bndso_[0].buffer_type = MYSQL_TYPE_LONG;
    bndso_[0].buffer = (char*)&user_id;

    if ( mysql_stmt_bind_result(sSet_, bndso_) )
        Cdb::ThrowSQL("Cusers_settings::bindSetOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Reset (zero) public variables
--------------------------------------------------------------------------- */
void Cusers_settings::Reset()
{
    user_id = 0;
    us_key[0] = EOS;
    us_val[0] = EOS;
}
