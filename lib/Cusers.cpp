/* ---------------------------------------------------------------------------
   Table access class
   Generated on nodepp.org on 2022-03-29 20:05:41, generator v.2.0.1
   Using C-style strings
   Using exceptions
--------------------------------------------------------------------------- */

#include "Cusers.h"


bool Cusers::slots_[CDB_MAX_INSTANCES]={0};


/* ---------------------------------------------------------------------------
   Constructor
--------------------------------------------------------------------------- */
Cusers::Cusers()
{
    setInstance(slots_);

    table_ = "users";

    columnList_ =   "id,"
                    "login,"
                    "login_u,"
                    "email,"
                    "email_u,"
                    "name,"
                    "phone,"
                    "passwd1,"
                    "passwd2,"
                    "lang,"
                    "about,"
                    "group_id,"
                    "auth_level,"
                    "status,"
                    "created,"
                    "last_login,"
                    "visits,"
                    "ula_cnt,"
                    "ula_time";

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
Cusers::~Cusers()
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
bool Cusers::Fetch()
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
            Cdb::ThrowSQL("Cusers::Fetch | mysql_stmt_fetch");
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
bool Cusers::Get(int arg_id)
{
    int ret;

    if ( firstGet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT id,login,login_u,email,email_u,name,phone,passwd1,passwd2,lang,about,group_id,auth_level,status,created,last_login,visits,ula_cnt,ula_time FROM users WHERE id=?");
        ret = mysql_stmt_prepare(sGet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers::Get | mysql_stmt_prepare");
        firstGet_ = false;
    }

    bindKey(sGet_, arg_id);

    if ( mysql_stmt_execute(sGet_) )
        Cdb::ThrowSQL("Cusers::Get | mysql_stmt_execute");

    bindOutput(sGet_);

    if ( mysql_stmt_store_result(sGet_) )
        Cdb::ThrowSQL("Cusers::Get | mysql_stmt_store_result");

    ret = mysql_stmt_fetch(sGet_);

    if ( ret != 0 )
    {
        if ( ret == 1 || ret == MYSQL_NO_DATA )
            return false;
        else
            Cdb::ThrowSQL("Cusers::Get | mysql_stmt_fetch");
    }

    genDTStrings();

    return true;
}


/* ---------------------------------------------------------------------------
   Insert record
--------------------------------------------------------------------------- */
unsigned Cusers::Insert()
{
    int ret;

    if ( firstInsert_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "INSERT INTO users (login,login_u,email,email_u,name,phone,passwd1,passwd2,lang,about,group_id,auth_level,status,created,last_login,visits,ula_cnt,ula_time) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
        ret = mysql_stmt_prepare(sInsert_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers::Insert | mysql_stmt_prepare");
        firstInsert_ = false;
    }

    bindInput(sInsert_);

    ret = mysql_stmt_execute(sInsert_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers::Insert | mysql_stmt_execute");

    id = mysql_insert_id(dbConn_);

    return id;
}


/* ---------------------------------------------------------------------------
   Update record by PK
--------------------------------------------------------------------------- */
void Cusers::Update(int arg_id)
{
    int ret;

    if ( firstUpdate_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "UPDATE users SET login=?,login_u=?,email=?,email_u=?,name=?,phone=?,passwd1=?,passwd2=?,lang=?,about=?,group_id=?,auth_level=?,status=?,created=?,last_login=?,visits=?,ula_cnt=?,ula_time=? WHERE id=?");
        ret = mysql_stmt_prepare(sUpdate_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers::Update | mysql_stmt_prepare");
        firstUpdate_ = false;
    }

    bindInput(sUpdate_, true, arg_id);

    ret = mysql_stmt_execute(sUpdate_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers::Update | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Delete record by PK
--------------------------------------------------------------------------- */
void Cusers::Delete(int arg_id)
{
    int ret;

    if ( firstDelete_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "DELETE FROM users WHERE id=?");
        ret = mysql_stmt_prepare(sDelete_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers::Delete | mysql_stmt_prepare");
        firstDelete_ = false;
    }

    bindKey(sDelete_, arg_id);

    ret = mysql_stmt_execute(sDelete_);

    if ( ret != 0 )
        Cdb::ThrowSQL("Cusers::Delete | mysql_stmt_execute");
}


/* ---------------------------------------------------------------------------
   Insert or update record by PK
--------------------------------------------------------------------------- */
void Cusers::Set(int arg_id)
{
    int ret;

    if ( firstSet_ )
    {
        char q[CDB_SQLBUF];
        sprintf(q, "SELECT id FROM users WHERE id=?");
        ret = mysql_stmt_prepare(sSet_, q, strlen(q));
        if ( ret != 0 )
            Cdb::ThrowSQL("Cusers::Set | mysql_stmt_prepare");
        firstSet_ = false;
    }

    bindKey(sSet_, arg_id);

    if ( mysql_stmt_execute(sSet_) )
        Cdb::ThrowSQL("Cusers::Set | mysql_stmt_execute");

    bindSetOutput();

    if ( mysql_stmt_store_result(sSet_) )
        Cdb::ThrowSQL("Cusers::Set | mysql_stmt_store_result");

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
        Cdb::ThrowSQL("Cusers::Set | mysql_stmt_fetch");
}


/* ---------------------------------------------------------------------------
   Bind key values
--------------------------------------------------------------------------- */
void Cusers::bindKey(MYSQL_STMT *s, int arg_id)
{
    k_id_ = arg_id;

    memset(&bndk_, 0, sizeof(bndk_));

    bndk_[0].buffer_type = MYSQL_TYPE_LONG;
    bndk_[0].buffer = (char*)&k_id_;

    if ( mysql_stmt_bind_param(s, bndk_) )
        Cdb::ThrowSQL("Cusers::bindKey | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind input values
--------------------------------------------------------------------------- */
void Cusers::bindInput(MYSQL_STMT *s, bool withKey, int arg_id)
{
    login_len_ = strlen(login);
    login_u_len_ = strlen(login_u);
    email_len_ = strlen(email);
    email_u_len_ = strlen(email_u);
    name_len_ = strlen(name);
    phone_len_ = strlen(phone);
    passwd1_len_ = strlen(passwd1);
    passwd2_len_ = strlen(passwd2);
    lang_len_ = strlen(lang);
    about_len_ = strlen(about);

    set_datetime(&t_created_, created);
    set_datetime(&t_last_login_, last_login);
    set_datetime(&t_ula_time_, ula_time);

    memset(&bndi_, 0, sizeof(bndi_));

    bndi_[0].buffer_type = MYSQL_TYPE_STRING;
    bndi_[0].buffer = (char*)login;
    bndi_[0].length = &login_len_;

    bndi_[1].buffer_type = MYSQL_TYPE_STRING;
    bndi_[1].buffer = (char*)login_u;
    bndi_[1].length = &login_u_len_;

    bndi_[2].buffer_type = MYSQL_TYPE_STRING;
    bndi_[2].buffer = (char*)email;
    bndi_[2].length = &email_len_;

    bndi_[3].buffer_type = MYSQL_TYPE_STRING;
    bndi_[3].buffer = (char*)email_u;
    bndi_[3].length = &email_u_len_;

    bndi_[4].buffer_type = MYSQL_TYPE_STRING;
    bndi_[4].buffer = (char*)name;
    bndi_[4].length = &name_len_;

    bndi_[5].buffer_type = MYSQL_TYPE_STRING;
    bndi_[5].buffer = (char*)phone;
    bndi_[5].length = &phone_len_;

    bndi_[6].buffer_type = MYSQL_TYPE_STRING;
    bndi_[6].buffer = (char*)passwd1;
    bndi_[6].length = &passwd1_len_;

    bndi_[7].buffer_type = MYSQL_TYPE_STRING;
    bndi_[7].buffer = (char*)passwd2;
    bndi_[7].length = &passwd2_len_;

    bndi_[8].buffer_type = MYSQL_TYPE_STRING;
    bndi_[8].buffer = (char*)lang;
    bndi_[8].length = &lang_len_;

    bndi_[9].buffer_type = MYSQL_TYPE_STRING;
    bndi_[9].buffer = (char*)about;
    bndi_[9].length = &about_len_;

    bndi_[10].buffer_type = MYSQL_TYPE_LONG;
    bndi_[10].buffer = (char*)&group_id;

    bndi_[11].buffer_type = MYSQL_TYPE_TINY;
    bndi_[11].buffer = (char*)&auth_level;

    bndi_[12].buffer_type = MYSQL_TYPE_TINY;
    bndi_[12].buffer = (char*)&status;

    bndi_[13].buffer_type = MYSQL_TYPE_DATETIME;
    bndi_[13].buffer = (char*)&t_created_;

    bndi_[14].buffer_type = MYSQL_TYPE_DATETIME;
    bndi_[14].buffer = (char*)&t_last_login_;

    bndi_[15].buffer_type = MYSQL_TYPE_LONG;
    bndi_[15].buffer = (char*)&visits;

    bndi_[16].buffer_type = MYSQL_TYPE_LONG;
    bndi_[16].buffer = (char*)&ula_cnt;

    bndi_[17].buffer_type = MYSQL_TYPE_DATETIME;
    bndi_[17].buffer = (char*)&t_ula_time_;

    if ( withKey )   /* after WHERE */
    {
        k_id_ = arg_id;

        bndi_[18].buffer_type = MYSQL_TYPE_LONG;
        bndi_[18].buffer = (char*)&k_id_;

    }

    if ( mysql_stmt_bind_param(s, bndi_) )
        Cdb::ThrowSQL("Cusers::bindInput | mysql_stmt_bind_param");
}


/* ---------------------------------------------------------------------------
   Bind output values
--------------------------------------------------------------------------- */
void Cusers::bindOutput(MYSQL_STMT *s)
{
    memset(&bndo_, 0, sizeof(bndo_));

    bndo_[0].buffer_type = MYSQL_TYPE_LONG;
    bndo_[0].buffer = (char*)&id;
    bndo_[0].is_null = &id_is_null_;

    bndo_[1].buffer_type = MYSQL_TYPE_STRING;
    bndo_[1].buffer = (char*)login;
    bndo_[1].buffer_length = 31;
    bndo_[1].is_null = &login_is_null_;

    bndo_[2].buffer_type = MYSQL_TYPE_STRING;
    bndo_[2].buffer = (char*)login_u;
    bndo_[2].buffer_length = 31;
    bndo_[2].is_null = &login_u_is_null_;

    bndo_[3].buffer_type = MYSQL_TYPE_STRING;
    bndo_[3].buffer = (char*)email;
    bndo_[3].buffer_length = 121;
    bndo_[3].is_null = &email_is_null_;

    bndo_[4].buffer_type = MYSQL_TYPE_STRING;
    bndo_[4].buffer = (char*)email_u;
    bndo_[4].buffer_length = 121;
    bndo_[4].is_null = &email_u_is_null_;

    bndo_[5].buffer_type = MYSQL_TYPE_STRING;
    bndo_[5].buffer = (char*)name;
    bndo_[5].buffer_length = 121;
    bndo_[5].is_null = &name_is_null_;

    bndo_[6].buffer_type = MYSQL_TYPE_STRING;
    bndo_[6].buffer = (char*)phone;
    bndo_[6].buffer_length = 31;
    bndo_[6].is_null = &phone_is_null_;

    bndo_[7].buffer_type = MYSQL_TYPE_STRING;
    bndo_[7].buffer = (char*)passwd1;
    bndo_[7].buffer_length = 45;
    bndo_[7].is_null = &passwd1_is_null_;

    bndo_[8].buffer_type = MYSQL_TYPE_STRING;
    bndo_[8].buffer = (char*)passwd2;
    bndo_[8].buffer_length = 45;
    bndo_[8].is_null = &passwd2_is_null_;

    bndo_[9].buffer_type = MYSQL_TYPE_STRING;
    bndo_[9].buffer = (char*)lang;
    bndo_[9].buffer_length = 6;
    bndo_[9].is_null = &lang_is_null_;

    bndo_[10].buffer_type = MYSQL_TYPE_STRING;
    bndo_[10].buffer = (char*)about;
    bndo_[10].buffer_length = 251;
    bndo_[10].is_null = &about_is_null_;

    bndo_[11].buffer_type = MYSQL_TYPE_LONG;
    bndo_[11].buffer = (char*)&group_id;
    bndo_[11].is_null = &group_id_is_null_;

    bndo_[12].buffer_type = MYSQL_TYPE_TINY;
    bndo_[12].buffer = (char*)&auth_level;
    bndo_[12].is_null = &auth_level_is_null_;

    bndo_[13].buffer_type = MYSQL_TYPE_TINY;
    bndo_[13].buffer = (char*)&status;
    bndo_[13].is_null = &status_is_null_;

    bndo_[14].buffer_type = MYSQL_TYPE_DATETIME;
    bndo_[14].buffer = (char*)&t_created_;
    bndo_[14].is_null = &created_is_null_;

    bndo_[15].buffer_type = MYSQL_TYPE_DATETIME;
    bndo_[15].buffer = (char*)&t_last_login_;
    bndo_[15].is_null = &last_login_is_null_;

    bndo_[16].buffer_type = MYSQL_TYPE_LONG;
    bndo_[16].buffer = (char*)&visits;
    bndo_[16].is_null = &visits_is_null_;

    bndo_[17].buffer_type = MYSQL_TYPE_LONG;
    bndo_[17].buffer = (char*)&ula_cnt;
    bndo_[17].is_null = &ula_cnt_is_null_;

    bndo_[18].buffer_type = MYSQL_TYPE_DATETIME;
    bndo_[18].buffer = (char*)&t_ula_time_;
    bndo_[18].is_null = &ula_time_is_null_;

    if ( mysql_stmt_bind_result(s, bndo_) )
        Cdb::ThrowSQL("Cusers::bindOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Bind output value for Set
--------------------------------------------------------------------------- */
void Cusers::bindSetOutput()
{
static int id;    /* to be scrapped anyway */

    memset(&bndso_, 0, sizeof(bndso_));

    bndso_[0].buffer_type = MYSQL_TYPE_LONG;
    bndso_[0].buffer = (char*)&id;

    if ( mysql_stmt_bind_result(sSet_, bndso_) )
        Cdb::ThrowSQL("Cusers::bindSetOutput | mysql_stmt_bind_result");
}


/* ---------------------------------------------------------------------------
   Generate date-time strings
--------------------------------------------------------------------------- */
void Cusers::genDTStrings()
{
    if ( created_is_null_ )
        created[0] = EOS;
    else
        sprintf(created, "%04d-%02d-%02d %02d:%02d:%02d", t_created_.year, t_created_.month, t_created_.day, t_created_.hour, t_created_.minute, t_created_.second);

    if ( last_login_is_null_ )
        last_login[0] = EOS;
    else
        sprintf(last_login, "%04d-%02d-%02d %02d:%02d:%02d", t_last_login_.year, t_last_login_.month, t_last_login_.day, t_last_login_.hour, t_last_login_.minute, t_last_login_.second);

    if ( ula_time_is_null_ )
        ula_time[0] = EOS;
    else
        sprintf(ula_time, "%04d-%02d-%02d %02d:%02d:%02d", t_ula_time_.year, t_ula_time_.month, t_ula_time_.day, t_ula_time_.hour, t_ula_time_.minute, t_ula_time_.second);

}


/* ---------------------------------------------------------------------------
   Reset (zero) public variables
--------------------------------------------------------------------------- */
void Cusers::Reset()
{
    id = 0;
    login[0] = EOS;
    login_u[0] = EOS;
    email[0] = EOS;
    email_u[0] = EOS;
    name[0] = EOS;
    phone[0] = EOS;
    passwd1[0] = EOS;
    passwd2[0] = EOS;
    lang[0] = EOS;
    about[0] = EOS;
    group_id = 0;
    auth_level = 0;
    status = 0;
    created[0] = EOS;
    last_login[0] = EOS;
    visits = 0;
    ula_cnt = 0;
    ula_time[0] = EOS;
}
