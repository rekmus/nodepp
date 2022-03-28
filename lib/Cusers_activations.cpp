/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-28 15:16:31, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#include "Cusers_activations.h"


bool Cusers_activations::slots_[CDB_MAX_INSTANCES]={0};


/* ---------------------------------------------------------------------------
   Constructor
--------------------------------------------------------------------------- */
Cusers_activations::Cusers_activations()
{
    setInstance(slots_);

    table_ = "users_activations";

    columnList_ =   "linkkey,"
                    "user_id,"
                    "created,"
                    "activated";

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
Cusers_activations::~Cusers_activations()
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
bool Cusers_activations::Fetch()
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
            Cdb::ThrowSQL("Cusers_activations::Fetch | mysql_stmt_fetch");
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
bool Cusers_activations::Get(const std::string& arg_linkkey)
{
    int ret;

    if ( firstGet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT linkkey,user_id,created,activated FROM users_activations WHERE linkkey=BINARY ?");
        ret = mysql_stmt_prepare(sGet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_activations::Get | mysql_stmt_prepare");
        firstGet_ = false;
    }

    bindKey(sGet_, arg_linkkey);

    if ( mysql_stmt_execute(sGet_) )
        Cdb::ThrowSQL("Cusers_activations::Get | mysql_stmt_execute");

    bindOutput(sGet_);

    if ( mysql_stmt_store_result(sGet_) )
        Cdb::ThrowSQL("Cusers_activations::Get | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sGet_);

    if ( ret != 0 )
    {
        if ( ret == 1 || ret == MYSQL_NO_DATA )
            return false;
        else
            Cdb::ThrowSQL("Cusers_activations::Get | mysql_stmt_fetch");
    }

    genDTStrings();

    return true;
}


/* ---------------------------------------------------------------------------
   Insert record
--------------------------------------------------------------------------- */
unsigned Cusers_activations::Insert()
{
    int ret;

    if ( firstInsert_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "INSERT INTO users_activations (linkkey,user_id,created,activated) VALUES (?,?,?,?)");
        ret = mysql_stmt_prepare(sInsert_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_activations::Insert | mysql_stmt_prepare");
        firstInsert_ = false;
    }

    bindInput(sInsert_);

    ret = mysql_stmt_execute(sInsert_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_activations::Insert | mysql_stmt_execute");

    return mysql_insert_id(dbConn_);
}


/* ---------------------------------------------------------------------------
   Update record by PK
--------------------------------------------------------------------------- */
void Cusers_activations::Update(const std::string& arg_linkkey)
{
    int ret;

    if ( firstUpdate_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "UPDATE users_activations SET linkkey=?,user_id=?,created=?,activated=? WHERE linkkey=BINARY ?");
        ret = mysql_stmt_prepare(sUpdate_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_activations::Update | mysql_stmt_prepare");
        firstUpdate_ = false;
    }

    bindInput(sUpdate_, true, arg_linkkey);

    ret = mysql_stmt_execute(sUpdate_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_activations::Update | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Delete record by PK
--------------------------------------------------------------------------- */
void Cusers_activations::Delete(const std::string& arg_linkkey)
{
    int ret;

    if ( firstDelete_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "DELETE FROM users_activations WHERE linkkey=BINARY ?");
        ret = mysql_stmt_prepare(sDelete_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_activations::Delete | mysql_stmt_prepare");
        firstDelete_ = false;
    }

    bindKey(sDelete_, arg_linkkey);

    ret = mysql_stmt_execute(sDelete_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_activations::Delete | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Insert or update record by PK
--------------------------------------------------------------------------- */
void Cusers_activations::Set(const std::string& arg_linkkey)
{
    int ret;

    if ( firstSet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT linkkey FROM users_activations WHERE linkkey=BINARY ?");
        ret = mysql_stmt_prepare(sSet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_activations::Set | mysql_stmt_prepare");
        firstSet_ = false;
    }

    bindKey(sSet_, arg_linkkey);

    if ( mysql_stmt_execute(sSet_) )
        Cdb::ThrowSQL("Cusers_activations::Set | mysql_stmt_execute");

    bindSetOutput();

    if ( mysql_stmt_store_result(sSet_) )
        Cdb::ThrowSQL("Cusers_activations::Set | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sSet_);

    if ( ret == 0 )   /* record existed */
    {
        Update(arg_linkkey);
    }
    else if ( ret == 1 || ret == MYSQL_NO_DATA )   /* not found ==> insert new */
    {
        strncpy(linkkey, arg_linkkey.c_str(), 20);
        linkkey[20] = EOS;

        Insert();
    }
    else
        Cdb::ThrowSQL("Cusers_activations::Set | mysql_stmt_fetch");
}


/* ---------------------------------------------------------------------------
   Bind key values
--------------------------------------------------------------------------- */
void Cusers_activations::bindKey(MYSQL_STMT *s, const std::string& arg_linkkey)
{
    strncpy(k_linkkey_, arg_linkkey.c_str(), 20);
    k_linkkey_[20] = EOS;

    k_linkkey_len_ = strlen(k_linkkey_);

    memset(&bndk_, 0, sizeof(bndk_));

    bndk_[0].buffer_type = MYSQL_TYPE_STRING;
    bndk_[0].buffer = (char*)k_linkkey_;
    bndk_[0].length = &k_linkkey_len_;

    if ( mysql_stmt_bind_param(s, bndk_) )
        Cdb::ThrowSQL("Cusers_activations::bindKey | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind input values
--------------------------------------------------------------------------- */
void Cusers_activations::bindInput(MYSQL_STMT *s, bool withKey, const std::string& arg_linkkey)
{
    linkkey_len_ = strlen(linkkey);

    set_datetime(&t_created_, created);
    set_datetime(&t_activated_, activated);

    memset(&bndi_, 0, sizeof(bndi_));

    bndi_[0].buffer_type = MYSQL_TYPE_STRING;
    bndi_[0].buffer = (char*)linkkey;
    bndi_[0].length = &linkkey_len_;

    bndi_[1].buffer_type = MYSQL_TYPE_LONG;
    bndi_[1].buffer = (char*)&user_id;

    bndi_[2].buffer_type = MYSQL_TYPE_DATETIME;
    bndi_[2].buffer = (char*)&t_created_;

    bndi_[3].buffer_type = MYSQL_TYPE_DATETIME;
    bndi_[3].buffer = (char*)&t_activated_;

    if ( withKey )   /* after WHERE */
    {
        strncpy(k_linkkey_, arg_linkkey.c_str(), 20);
        k_linkkey_[20] = EOS;

        k_linkkey_len_ = strlen(k_linkkey_);

        bndi_[4].buffer_type = MYSQL_TYPE_STRING;
        bndi_[4].buffer = (char*)k_linkkey_;
        bndi_[4].length = &k_linkkey_len_;

    }

    if ( mysql_stmt_bind_param(s, bndi_) )
        Cdb::ThrowSQL("Cusers_activations::bindInput | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind output values
--------------------------------------------------------------------------- */
void Cusers_activations::bindOutput(MYSQL_STMT *s)
{
    memset(&bndo_, 0, sizeof(bndo_));

    bndo_[0].buffer_type = MYSQL_TYPE_STRING;
    bndo_[0].buffer = (char*)linkkey;
    bndo_[0].buffer_length = 21;
    bndo_[0].is_null = &linkkey_is_null_;

    bndo_[1].buffer_type = MYSQL_TYPE_LONG;
    bndo_[1].buffer = (char*)&user_id;
    bndo_[1].is_null = &user_id_is_null_;

    bndo_[2].buffer_type = MYSQL_TYPE_DATETIME;
    bndo_[2].buffer = (char*)&t_created_;
    bndo_[2].is_null = &created_is_null_;

    bndo_[3].buffer_type = MYSQL_TYPE_DATETIME;
    bndo_[3].buffer = (char*)&t_activated_;
    bndo_[3].is_null = &activated_is_null_;

    if ( mysql_stmt_bind_result(s, bndo_) )
        Cdb::ThrowSQL("Cusers_activations::bindOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Bind output value for Set
--------------------------------------------------------------------------- */
void Cusers_activations::bindSetOutput()
{
static USERS_ACTIVATIONS_LINKKEY linkkey;    /* to be scrapped anyway */

    memset(&bndso_, 0, sizeof(bndso_));

    bndso_[0].buffer_type = MYSQL_TYPE_STRING;
    bndso_[0].buffer = (char*)linkkey;
    bndso_[0].buffer_length = 21;

    if ( mysql_stmt_bind_result(sSet_, bndso_) )
        Cdb::ThrowSQL("Cusers_activations::bindSetOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Generate date-time strings
--------------------------------------------------------------------------- */
void Cusers_activations::genDTStrings()
{
    if ( created_is_null_ )
        created[0] = EOS;
    else
        sprintf(created, "%04d-%02d-%02d %02d:%02d:%02d", t_created_.year, t_created_.month, t_created_.day, t_created_.hour, t_created_.minute, t_created_.second);

    if ( activated_is_null_ )
        activated[0] = EOS;
    else
        sprintf(activated, "%04d-%02d-%02d %02d:%02d:%02d", t_activated_.year, t_activated_.month, t_activated_.day, t_activated_.hour, t_activated_.minute, t_activated_.second);
}


/* ---------------------------------------------------------------------------
   Reset (zero) public variables
--------------------------------------------------------------------------- */
void Cusers_activations::Reset()
{
    linkkey[0] = EOS;
    user_id = 0;
    created[0] = EOS;
    activated[0] = EOS;
}
