-- ----------------------------------------------------------------------------
-- Node++ USERS module tables -- MySQL version
-- nodepp.org
-- ----------------------------------------------------------------------------

-- users

CREATE TABLE users
(
    id INT auto_increment PRIMARY KEY,
    login CHAR(30),
    login_u CHAR(30),               -- uppercase version
    email VARCHAR(120),
    email_u VARCHAR(120),           -- uppercase version
    name VARCHAR(120),
    phone VARCHAR(30),
    passwd1 CHAR(44),               -- SHA256 hash in base64
    passwd2 CHAR(44),               -- SHA256 hash in base64
    lang CHAR(5),
    about VARCHAR(250),
    group_id INT,
    auth_level TINYINT,             -- 10 = user, 20 = customer, 30 = staff, 40 = moderator, 50 = admin, 100 = root
    status TINYINT,                 -- 0 = inactive, 1 = active, 2 = locked, 3 = requires password change, 9 = deleted
    created DATETIME,
    last_login DATETIME,
    visits INT,
    ula_cnt INT,                    -- unsuccessful login attempt count
    ula_time DATETIME               -- and time
);

CREATE INDEX users_login ON users (login_u);
CREATE INDEX users_email ON users (email_u);
CREATE INDEX users_last_login ON users (last_login);


-- avatars

CREATE TABLE users_avatars
(
    user_id INT PRIMARY KEY,
    avatar_name VARCHAR(120),
    avatar_data BLOB,               -- 64 KiB
    avatar_len INT
);


-- groups

CREATE TABLE users_groups
(
    id INT auto_increment PRIMARY KEY,
    name VARCHAR(120),
    about VARCHAR(250),
    auth_level TINYINT
);


-- user settings

CREATE TABLE users_settings
(
    user_id INT,
    us_key CHAR(30),
    us_val VARCHAR(250),
    PRIMARY KEY (user_id, us_key)
);


-- user logins

CREATE TABLE users_logins
(
    sessid CHAR(15) CHARACTER SET latin1 COLLATE latin1_bin PRIMARY KEY,
    uagent VARCHAR(250),
    ip CHAR(45),
    user_id INT,
    csrft CHAR(7),
    created DATETIME,
    last_used DATETIME
);

CREATE INDEX users_logins_uid ON users_logins (user_id);


-- account activations

CREATE TABLE users_activations
(
    linkkey CHAR(20) CHARACTER SET latin1 COLLATE latin1_bin PRIMARY KEY,
    user_id INT,
    created DATETIME,
    activated DATETIME
);


-- password resets

CREATE TABLE users_p_resets
(
    linkkey CHAR(20) CHARACTER SET latin1 COLLATE latin1_bin PRIMARY KEY,
    user_id INT,
    created DATETIME,
    tries SMALLINT
);

CREATE INDEX users_p_resets_uid ON users_p_resets (user_id);


-- messages

CREATE TABLE users_messages
(
    user_id INT,
    msg_id INT,
    email VARCHAR(120),
    message TEXT,               -- 64 KiB limit
    created DATETIME,
    PRIMARY KEY (user_id, msg_id)
);
