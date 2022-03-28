/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-28 15:17:27, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#include "Cusers_messages.h"


bool Cusers_messages::slots_[CDB_MAX_INSTANCES]={0};


/* ---------------------------------------------------------------------------
   Constructor
--------------------------------------------------------------------------- */
Cusers_messages::Cusers_messages()
{
    setInstance(slots_);

    table_ = "users_messages";

    columnList_ =   "user_id,"
                    "msg_id,"
                    "email,"
                    "message,"
                    "created";

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
Cusers_messages::~Cusers_messages()
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
bool Cusers_messages::Fetch()
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
            Cdb::ThrowSQL("Cusers_messages::Fetch | mysql_stmt_fetch");
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
bool Cusers_messages::Get(int arg_user_id, int arg_msg_id)
{
    int ret;

    if ( firstGet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT user_id,msg_id,email,message,created FROM users_messages WHERE user_id=? AND msg_id=?");
        ret = mysql_stmt_prepare(sGet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_messages::Get | mysql_stmt_prepare");
        firstGet_ = false;
    }

    bindKey(sGet_, arg_user_id, arg_msg_id);

    if ( mysql_stmt_execute(sGet_) )
        Cdb::ThrowSQL("Cusers_messages::Get | mysql_stmt_execute");

    bindOutput(sGet_);

    if ( mysql_stmt_store_result(sGet_) )
        Cdb::ThrowSQL("Cusers_messages::Get | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sGet_);

    if ( ret != 0 )
    {
        if ( ret == 1 || ret == MYSQL_NO_DATA )
            return false;
        else
            Cdb::ThrowSQL("Cusers_messages::Get | mysql_stmt_fetch");
    }

    genDTStrings();

    return true;
}


/* ---------------------------------------------------------------------------
   Insert record
--------------------------------------------------------------------------- */
unsigned Cusers_messages::Insert()
{
    int ret;

    if ( firstInsert_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "INSERT INTO users_messages (user_id,msg_id,email,message,created) VALUES (?,?,?,?,?)");
        ret = mysql_stmt_prepare(sInsert_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_messages::Insert | mysql_stmt_prepare");
        firstInsert_ = false;
    }

    bindInput(sInsert_);

    ret = mysql_stmt_execute(sInsert_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_messages::Insert | mysql_stmt_execute");

    return mysql_insert_id(dbConn_);
}


/* ---------------------------------------------------------------------------
   Update record by PK
--------------------------------------------------------------------------- */
void Cusers_messages::Update(int arg_user_id, int arg_msg_id)
{
    int ret;

    if ( firstUpdate_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "UPDATE users_messages SET user_id=?,msg_id=?,email=?,message=?,created=? WHERE user_id=? AND msg_id=?");
        ret = mysql_stmt_prepare(sUpdate_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_messages::Update | mysql_stmt_prepare");
        firstUpdate_ = false;
    }

    bindInput(sUpdate_, true, arg_user_id, arg_msg_id);

    ret = mysql_stmt_execute(sUpdate_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_messages::Update | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Delete record by PK
--------------------------------------------------------------------------- */
void Cusers_messages::Delete(int arg_user_id, int arg_msg_id)
{
    int ret;

    if ( firstDelete_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "DELETE FROM users_messages WHERE user_id=? AND msg_id=?");
        ret = mysql_stmt_prepare(sDelete_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_messages::Delete | mysql_stmt_prepare");
        firstDelete_ = false;
    }

    bindKey(sDelete_, arg_user_id, arg_msg_id);

    ret = mysql_stmt_execute(sDelete_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_messages::Delete | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Insert or update record by PK
--------------------------------------------------------------------------- */
void Cusers_messages::Set(int arg_user_id, int arg_msg_id)
{
    int ret;

    if ( firstSet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT user_id FROM users_messages WHERE user_id=? AND msg_id=?");
        ret = mysql_stmt_prepare(sSet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_messages::Set | mysql_stmt_prepare");
        firstSet_ = false;
    }

    bindKey(sSet_, arg_user_id, arg_msg_id);

    if ( mysql_stmt_execute(sSet_) )
        Cdb::ThrowSQL("Cusers_messages::Set | mysql_stmt_execute");

    bindSetOutput();

    if ( mysql_stmt_store_result(sSet_) )
        Cdb::ThrowSQL("Cusers_messages::Set | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sSet_);

    if ( ret == 0 )   /* record existed */
    {
        Update(arg_user_id, arg_msg_id);
    }
    else if ( ret == 1 || ret == MYSQL_NO_DATA )   /* not found ==> insert new */
    {
        user_id = arg_user_id;
        msg_id = arg_msg_id;

        Insert();
    }
    else
        Cdb::ThrowSQL("Cusers_messages::Set | mysql_stmt_fetch");
}


/* ---------------------------------------------------------------------------
   Bind key values
--------------------------------------------------------------------------- */
void Cusers_messages::bindKey(MYSQL_STMT *s, int arg_user_id, int arg_msg_id)
{
    k_user_id_ = arg_user_id;
    k_msg_id_ = arg_msg_id;

    memset(&bndk_, 0, sizeof(bndk_));

    bndk_[0].buffer_type = MYSQL_TYPE_LONG;
    bndk_[0].buffer = (char*)&k_user_id_;

    bndk_[1].buffer_type = MYSQL_TYPE_LONG;
    bndk_[1].buffer = (char*)&k_msg_id_;

    if ( mysql_stmt_bind_param(s, bndk_) )
        Cdb::ThrowSQL("Cusers_messages::bindKey | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind input values
--------------------------------------------------------------------------- */
void Cusers_messages::bindInput(MYSQL_STMT *s, bool withKey, int arg_user_id, int arg_msg_id)
{
    email_len_ = strlen(email);
    message_len_ = strlen(message);

    set_datetime(&t_created_, created);

    memset(&bndi_, 0, sizeof(bndi_));

    bndi_[0].buffer_type = MYSQL_TYPE_LONG;
    bndi_[0].buffer = (char*)&user_id;

    bndi_[1].buffer_type = MYSQL_TYPE_LONG;
    bndi_[1].buffer = (char*)&msg_id;

    bndi_[2].buffer_type = MYSQL_TYPE_STRING;
    bndi_[2].buffer = (char*)email;
    bndi_[2].length = &email_len_;

    bndi_[3].buffer_type = MYSQL_TYPE_STRING;
    bndi_[3].buffer = (char*)message;
    bndi_[3].length = &message_len_;

    bndi_[4].buffer_type = MYSQL_TYPE_DATETIME;
    bndi_[4].buffer = (char*)&t_created_;

    if ( withKey )   /* after WHERE */
    {
        k_user_id_ = arg_user_id;
        k_msg_id_ = arg_msg_id;

        bndi_[5].buffer_type = MYSQL_TYPE_LONG;
        bndi_[5].buffer = (char*)&k_user_id_;

        bndi_[6].buffer_type = MYSQL_TYPE_LONG;
        bndi_[6].buffer = (char*)&k_msg_id_;

    }

    if ( mysql_stmt_bind_param(s, bndi_) )
        Cdb::ThrowSQL("Cusers_messages::bindInput | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind output values
--------------------------------------------------------------------------- */
void Cusers_messages::bindOutput(MYSQL_STMT *s)
{
    memset(&bndo_, 0, sizeof(bndo_));

    bndo_[0].buffer_type = MYSQL_TYPE_LONG;
    bndo_[0].buffer = (char*)&user_id;
    bndo_[0].is_null = &user_id_is_null_;

    bndo_[1].buffer_type = MYSQL_TYPE_LONG;
    bndo_[1].buffer = (char*)&msg_id;
    bndo_[1].is_null = &msg_id_is_null_;

    bndo_[2].buffer_type = MYSQL_TYPE_STRING;
    bndo_[2].buffer = (char*)email;
    bndo_[2].buffer_length = 121;
    bndo_[2].is_null = &email_is_null_;

    bndo_[3].buffer_type = MYSQL_TYPE_STRING;
    bndo_[3].buffer = (char*)message;
    bndo_[3].buffer_length = 65536;
    bndo_[3].is_null = &message_is_null_;

    bndo_[4].buffer_type = MYSQL_TYPE_DATETIME;
    bndo_[4].buffer = (char*)&t_created_;
    bndo_[4].is_null = &created_is_null_;

    if ( mysql_stmt_bind_result(s, bndo_) )
        Cdb::ThrowSQL("Cusers_messages::bindOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Bind output value for Set
--------------------------------------------------------------------------- */
void Cusers_messages::bindSetOutput()
{
static int user_id;    /* to be scrapped anyway */

    memset(&bndso_, 0, sizeof(bndso_));

    bndso_[0].buffer_type = MYSQL_TYPE_LONG;
    bndso_[0].buffer = (char*)&user_id;

    if ( mysql_stmt_bind_result(sSet_, bndso_) )
        Cdb::ThrowSQL("Cusers_messages::bindSetOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Generate date-time strings
--------------------------------------------------------------------------- */
void Cusers_messages::genDTStrings()
{
    if ( created_is_null_ )
        created[0] = EOS;
    else
        sprintf(created, "%04d-%02d-%02d %02d:%02d:%02d", t_created_.year, t_created_.month, t_created_.day, t_created_.hour, t_created_.minute, t_created_.second);

}


/* ---------------------------------------------------------------------------
   Reset (zero) public variables
--------------------------------------------------------------------------- */
void Cusers_messages::Reset()
{
    user_id = 0;
    msg_id = 0;
    email[0] = EOS;
    message[0] = EOS;
    created[0] = EOS;
}
