/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-28 15:15:25, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#include "Cusers_groups.h"


bool Cusers_groups::slots_[CDB_MAX_INSTANCES]={0};


/* ---------------------------------------------------------------------------
   Constructor
--------------------------------------------------------------------------- */
Cusers_groups::Cusers_groups()
{
    setInstance(slots_);

    table_ = "users_groups";

    columnList_ =   "id,"
                    "name,"
                    "about,"
                    "auth_level";

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
Cusers_groups::~Cusers_groups()
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
bool Cusers_groups::Fetch()
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
            Cdb::ThrowSQL("Cusers_groups::Fetch | mysql_stmt_fetch");
            return false;
        }
    }

    return true;
}


/* ---------------------------------------------------------------------------
   Get record by PK
   Not Found will return false
--------------------------------------------------------------------------- */
bool Cusers_groups::Get(int arg_id)
{
    int ret;

    if ( firstGet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT id,name,about,auth_level FROM users_groups WHERE id=?");
        ret = mysql_stmt_prepare(sGet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_groups::Get | mysql_stmt_prepare");
        firstGet_ = false;
    }

    bindKey(sGet_, arg_id);

    if ( mysql_stmt_execute(sGet_) )
        Cdb::ThrowSQL("Cusers_groups::Get | mysql_stmt_execute");

    bindOutput(sGet_);

    if ( mysql_stmt_store_result(sGet_) )
        Cdb::ThrowSQL("Cusers_groups::Get | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sGet_);

    if ( ret != 0 )
    {
        if ( ret == 1 || ret == MYSQL_NO_DATA )
            return false;
        else
            Cdb::ThrowSQL("Cusers_groups::Get | mysql_stmt_fetch");
    }

    return true;
}


/* ---------------------------------------------------------------------------
   Insert record
--------------------------------------------------------------------------- */
unsigned Cusers_groups::Insert()
{
    int ret;

    if ( firstInsert_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "INSERT INTO users_groups (name,about,auth_level) VALUES (?,?,?)");
        ret = mysql_stmt_prepare(sInsert_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_groups::Insert | mysql_stmt_prepare");
        firstInsert_ = false;
    }

    bindInput(sInsert_);

    ret = mysql_stmt_execute(sInsert_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_groups::Insert | mysql_stmt_execute");

    id = mysql_insert_id(dbConn_);

    return id;
}


/* ---------------------------------------------------------------------------
   Update record by PK
--------------------------------------------------------------------------- */
void Cusers_groups::Update(int arg_id)
{
    int ret;

    if ( firstUpdate_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "UPDATE users_groups SET name=?,about=?,auth_level=? WHERE id=?");
        ret = mysql_stmt_prepare(sUpdate_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_groups::Update | mysql_stmt_prepare");
        firstUpdate_ = false;
    }

    bindInput(sUpdate_, true, arg_id);

    ret = mysql_stmt_execute(sUpdate_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_groups::Update | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Delete record by PK
--------------------------------------------------------------------------- */
void Cusers_groups::Delete(int arg_id)
{
    int ret;

    if ( firstDelete_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "DELETE FROM users_groups WHERE id=?");
        ret = mysql_stmt_prepare(sDelete_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_groups::Delete | mysql_stmt_prepare");
        firstDelete_ = false;
    }

    bindKey(sDelete_, arg_id);

    ret = mysql_stmt_execute(sDelete_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_groups::Delete | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Insert or update record by PK
--------------------------------------------------------------------------- */
void Cusers_groups::Set(int arg_id)
{
    int ret;

    if ( firstSet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT id FROM users_groups WHERE id=?");
        ret = mysql_stmt_prepare(sSet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_groups::Set | mysql_stmt_prepare");
        firstSet_ = false;
    }

    bindKey(sSet_, arg_id);

    if ( mysql_stmt_execute(sSet_) )
        Cdb::ThrowSQL("Cusers_groups::Set | mysql_stmt_execute");

    bindSetOutput();

    if ( mysql_stmt_store_result(sSet_) )
        Cdb::ThrowSQL("Cusers_groups::Set | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sSet_);

    if ( ret == 0 )   /* record existed */
    {
        Update(arg_id);
    }
    else if ( ret == 1 || ret == MYSQL_NO_DATA )   /* not found ==> insert new */
    {
        id = arg_id;

        Insert();
    }
    else
        Cdb::ThrowSQL("Cusers_groups::Set | mysql_stmt_fetch");
}


/* ---------------------------------------------------------------------------
   Bind key values
--------------------------------------------------------------------------- */
void Cusers_groups::bindKey(MYSQL_STMT *s, int arg_id)
{
    k_id_ = arg_id;

    memset(&bndk_, 0, sizeof(bndk_));

    bndk_[0].buffer_type = MYSQL_TYPE_LONG;
    bndk_[0].buffer = (char*)&k_id_;

    if ( mysql_stmt_bind_param(s, bndk_) )
        Cdb::ThrowSQL("Cusers_groups::bindKey | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind input values
--------------------------------------------------------------------------- */
void Cusers_groups::bindInput(MYSQL_STMT *s, bool withKey, int arg_id)
{
    name_len_ = strlen(name);
    about_len_ = strlen(about);

    memset(&bndi_, 0, sizeof(bndi_));

    bndi_[0].buffer_type = MYSQL_TYPE_STRING;
    bndi_[0].buffer = (char*)name;
    bndi_[0].length = &name_len_;

    bndi_[1].buffer_type = MYSQL_TYPE_STRING;
    bndi_[1].buffer = (char*)about;
    bndi_[1].length = &about_len_;

    bndi_[2].buffer_type = MYSQL_TYPE_TINY;
    bndi_[2].buffer = (char*)&auth_level;

    if ( withKey )   /* after WHERE */
    {
        k_id_ = arg_id;

        bndi_[3].buffer_type = MYSQL_TYPE_LONG;
        bndi_[3].buffer = (char*)&k_id_;

    }

    if ( mysql_stmt_bind_param(s, bndi_) )
        Cdb::ThrowSQL("Cusers_groups::bindInput | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind output values
--------------------------------------------------------------------------- */
void Cusers_groups::bindOutput(MYSQL_STMT *s)
{
    memset(&bndo_, 0, sizeof(bndo_));

    bndo_[0].buffer_type = MYSQL_TYPE_LONG;
    bndo_[0].buffer = (char*)&id;
    bndo_[0].is_null = &id_is_null_;

    bndo_[1].buffer_type = MYSQL_TYPE_STRING;
    bndo_[1].buffer = (char*)name;
    bndo_[1].buffer_length = 121;
    bndo_[1].is_null = &name_is_null_;

    bndo_[2].buffer_type = MYSQL_TYPE_STRING;
    bndo_[2].buffer = (char*)about;
    bndo_[2].buffer_length = 251;
    bndo_[2].is_null = &about_is_null_;

    bndo_[3].buffer_type = MYSQL_TYPE_TINY;
    bndo_[3].buffer = (char*)&auth_level;
    bndo_[3].is_null = &auth_level_is_null_;

    if ( mysql_stmt_bind_result(s, bndo_) )
        Cdb::ThrowSQL("Cusers_groups::bindOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Bind output value for Set
--------------------------------------------------------------------------- */
void Cusers_groups::bindSetOutput()
{
static int id;    /* to be scrapped anyway */

    memset(&bndso_, 0, sizeof(bndso_));

    bndso_[0].buffer_type = MYSQL_TYPE_LONG;
    bndso_[0].buffer = (char*)&id;

    if ( mysql_stmt_bind_result(sSet_, bndso_) )
        Cdb::ThrowSQL("Cusers_groups::bindSetOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Reset (zero) public variables
--------------------------------------------------------------------------- */
void Cusers_groups::Reset()
{
    id = 0;
    name[0] = EOS;
    about[0] = EOS;
    auth_level = 0;
}
