/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-28 14:28:27, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#include "Cusers_logins.h"


bool Cusers_logins::slots_[CDB_MAX_INSTANCES]={0};


/* ---------------------------------------------------------------------------
   Constructor
--------------------------------------------------------------------------- */
Cusers_logins::Cusers_logins()
{
    setInstance(slots_);

    table_ = "users_logins";

    columnList_ =   "sessid,"
                    "uagent,"
                    "ip,"
                    "user_id,"
                    "csrft,"
                    "created,"
                    "last_used";

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
Cusers_logins::~Cusers_logins()
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
bool Cusers_logins::Fetch()
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
            Cdb::ThrowSQL("Cusers_logins::Fetch | mysql_stmt_fetch");
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
bool Cusers_logins::Get(const std::string& arg_sessid)
{
    int ret;

    if ( firstGet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT sessid,uagent,ip,user_id,csrft,created,last_used FROM users_logins WHERE sessid=BINARY ?");
        ret = mysql_stmt_prepare(sGet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_logins::Get | mysql_stmt_prepare");
        firstGet_ = false;
    }

    bindKey(sGet_, arg_sessid);

    if ( mysql_stmt_execute(sGet_) )
        Cdb::ThrowSQL("Cusers_logins::Get | mysql_stmt_execute");

    bindOutput(sGet_);

    if ( mysql_stmt_store_result(sGet_) )
        Cdb::ThrowSQL("Cusers_logins::Get | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sGet_);

    if ( ret != 0 )
    {
        if ( ret == 1 || ret == MYSQL_NO_DATA )
            return false;
        else
            Cdb::ThrowSQL("Cusers_logins::Get | mysql_stmt_fetch");
    }

    genDTStrings();

    return true;
}


/* ---------------------------------------------------------------------------
   Insert record
--------------------------------------------------------------------------- */
unsigned Cusers_logins::Insert()
{
    int ret;

    if ( firstInsert_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "INSERT INTO users_logins (sessid,uagent,ip,user_id,csrft,created,last_used) VALUES (?,?,?,?,?,?,?)");
        ret = mysql_stmt_prepare(sInsert_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_logins::Insert | mysql_stmt_prepare");
        firstInsert_ = false;
    }

    bindInput(sInsert_);

    ret = mysql_stmt_execute(sInsert_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_logins::Insert | mysql_stmt_execute");

    return mysql_insert_id(dbConn_);
}


/* ---------------------------------------------------------------------------
   Update record by PK
--------------------------------------------------------------------------- */
void Cusers_logins::Update(const std::string& arg_sessid)
{
    int ret;

    if ( firstUpdate_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "UPDATE users_logins SET sessid=?,uagent=?,ip=?,user_id=?,csrft=?,created=?,last_used=? WHERE sessid=BINARY ?");
        ret = mysql_stmt_prepare(sUpdate_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_logins::Update | mysql_stmt_prepare");
        firstUpdate_ = false;
    }

    bindInput(sUpdate_, true, arg_sessid);

    ret = mysql_stmt_execute(sUpdate_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_logins::Update | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Delete record by PK
--------------------------------------------------------------------------- */
void Cusers_logins::Delete(const std::string& arg_sessid)
{
    int ret;

    if ( firstDelete_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "DELETE FROM users_logins WHERE sessid=BINARY ?");
        ret = mysql_stmt_prepare(sDelete_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_logins::Delete | mysql_stmt_prepare");
        firstDelete_ = false;
    }

    bindKey(sDelete_, arg_sessid);

    ret = mysql_stmt_execute(sDelete_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers_logins::Delete | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Insert or update record by PK
--------------------------------------------------------------------------- */
void Cusers_logins::Set(const std::string& arg_sessid)
{
    int ret;

    if ( firstSet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT sessid FROM users_logins WHERE sessid=BINARY ?");
        ret = mysql_stmt_prepare(sSet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers_logins::Set | mysql_stmt_prepare");
        firstSet_ = false;
    }

    bindKey(sSet_, arg_sessid);

    if ( mysql_stmt_execute(sSet_) )
        Cdb::ThrowSQL("Cusers_logins::Set | mysql_stmt_execute");

    bindSetOutput();

    if ( mysql_stmt_store_result(sSet_) )
        Cdb::ThrowSQL("Cusers_logins::Set | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sSet_);

    if ( ret == 0 )   /* record existed */
    {
        Update(arg_sessid);
    }
    else if ( ret == 1 || ret == MYSQL_NO_DATA )   /* not found ==> insert new */
    {
        strncpy(sessid, arg_sessid.c_str(), 15);
        sessid[15] = EOS;

        Insert();
    }
    else
        Cdb::ThrowSQL("Cusers_logins::Set | mysql_stmt_fetch");
}


/* ---------------------------------------------------------------------------
   Bind key values
--------------------------------------------------------------------------- */
void Cusers_logins::bindKey(MYSQL_STMT *s, const std::string& arg_sessid)
{
    strncpy(k_sessid_, arg_sessid.c_str(), 15);
    k_sessid_[15] = EOS;

    k_sessid_len_ = strlen(k_sessid_);

    memset(&bndk_, 0, sizeof(bndk_));

    bndk_[0].buffer_type = MYSQL_TYPE_STRING;
    bndk_[0].buffer = (char*)k_sessid_;
    bndk_[0].length = &k_sessid_len_;

    if ( mysql_stmt_bind_param(s, bndk_) )
        Cdb::ThrowSQL("Cusers_logins::bindKey | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind input values
--------------------------------------------------------------------------- */
void Cusers_logins::bindInput(MYSQL_STMT *s, bool withKey, const std::string& arg_sessid)
{
    sessid_len_ = strlen(sessid);
    uagent_len_ = strlen(uagent);
    ip_len_ = strlen(ip);
    csrft_len_ = strlen(csrft);

    set_datetime(&t_created_, created);
    set_datetime(&t_last_used_, last_used);

    memset(&bndi_, 0, sizeof(bndi_));

    bndi_[0].buffer_type = MYSQL_TYPE_STRING;
    bndi_[0].buffer = (char*)sessid;
    bndi_[0].length = &sessid_len_;

    bndi_[1].buffer_type = MYSQL_TYPE_STRING;
    bndi_[1].buffer = (char*)uagent;
    bndi_[1].length = &uagent_len_;

    bndi_[2].buffer_type = MYSQL_TYPE_STRING;
    bndi_[2].buffer = (char*)ip;
    bndi_[2].length = &ip_len_;

    bndi_[3].buffer_type = MYSQL_TYPE_LONG;
    bndi_[3].buffer = (char*)&user_id;

    bndi_[4].buffer_type = MYSQL_TYPE_STRING;
    bndi_[4].buffer = (char*)csrft;
    bndi_[4].length = &csrft_len_;

    bndi_[5].buffer_type = MYSQL_TYPE_DATETIME;
    bndi_[5].buffer = (char*)&t_created_;

    bndi_[6].buffer_type = MYSQL_TYPE_DATETIME;
    bndi_[6].buffer = (char*)&t_last_used_;

    if ( withKey )   /* after WHERE */
    {
        strncpy(k_sessid_, arg_sessid.c_str(), 15);
        k_sessid_[15] = EOS;

        k_sessid_len_ = strlen(k_sessid_);

        bndi_[7].buffer_type = MYSQL_TYPE_STRING;
        bndi_[7].buffer = (char*)k_sessid_;
        bndi_[7].length = &k_sessid_len_;

    }

    if ( mysql_stmt_bind_param(s, bndi_) )
        Cdb::ThrowSQL("Cusers_logins::bindInput | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind output values
--------------------------------------------------------------------------- */
void Cusers_logins::bindOutput(MYSQL_STMT *s)
{
    memset(&bndo_, 0, sizeof(bndo_));

    bndo_[0].buffer_type = MYSQL_TYPE_STRING;
    bndo_[0].buffer = (char*)sessid;
    bndo_[0].buffer_length = 16;
    bndo_[0].is_null = &sessid_is_null_;

    bndo_[1].buffer_type = MYSQL_TYPE_STRING;
    bndo_[1].buffer = (char*)uagent;
    bndo_[1].buffer_length = 251;
    bndo_[1].is_null = &uagent_is_null_;

    bndo_[2].buffer_type = MYSQL_TYPE_STRING;
    bndo_[2].buffer = (char*)ip;
    bndo_[2].buffer_length = 46;
    bndo_[2].is_null = &ip_is_null_;

    bndo_[3].buffer_type = MYSQL_TYPE_LONG;
    bndo_[3].buffer = (char*)&user_id;
    bndo_[3].is_null = &user_id_is_null_;

    bndo_[4].buffer_type = MYSQL_TYPE_STRING;
    bndo_[4].buffer = (char*)csrft;
    bndo_[4].buffer_length = 8;
    bndo_[4].is_null = &csrft_is_null_;

    bndo_[5].buffer_type = MYSQL_TYPE_DATETIME;
    bndo_[5].buffer = (char*)&t_created_;
    bndo_[5].is_null = &created_is_null_;

    bndo_[6].buffer_type = MYSQL_TYPE_DATETIME;
    bndo_[6].buffer = (char*)&t_last_used_;
    bndo_[6].is_null = &last_used_is_null_;

    if ( mysql_stmt_bind_result(s, bndo_) )
        Cdb::ThrowSQL("Cusers_logins::bindOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Bind output value for Set
--------------------------------------------------------------------------- */
void Cusers_logins::bindSetOutput()
{
static USERS_LOGINS_SESSID sessid;    /* to be scrapped anyway */

    memset(&bndso_, 0, sizeof(bndso_));

    bndso_[0].buffer_type = MYSQL_TYPE_STRING;
    bndso_[0].buffer = (char*)sessid;
    bndso_[0].buffer_length = 16;

    if ( mysql_stmt_bind_result(sSet_, bndso_) )
        Cdb::ThrowSQL("Cusers_logins::bindSetOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Generate date-time strings
--------------------------------------------------------------------------- */
void Cusers_logins::genDTStrings()
{
    if ( created_is_null_ )
        created[0] = EOS;
    else
        sprintf(created, "%04d-%02d-%02d %02d:%02d:%02d", t_created_.year, t_created_.month, t_created_.day, t_created_.hour, t_created_.minute, t_created_.second);

    if ( last_used_is_null_ )
        last_used[0] = EOS;
    else
        sprintf(last_used, "%04d-%02d-%02d %02d:%02d:%02d", t_last_used_.year, t_last_used_.month, t_last_used_.day, t_last_used_.hour, t_last_used_.minute, t_last_used_.second);
}


/* ---------------------------------------------------------------------------
   Reset (zero) public variables
--------------------------------------------------------------------------- */
void Cusers_logins::Reset()
{
    sessid[0] = EOS;
    uagent[0] = EOS;
    ip[0] = EOS;
    user_id = 0;
    csrft[0] = EOS;
    created[0] = EOS;
    last_used[0] = EOS;
}
