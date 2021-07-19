# Node++

This is going to be [Silgy](https://github.com/silgy/silgy)'s successor.

There are a couple of breaches that I think need to be introduced in order to ensure the quality and ease of use of the code. I'll keep adding the notes here as I go.

1. OpenSSL linkage will be mandatory. All crypto functions that exist in OpenSSL will be removed from npp_lib. (DONE in v0.0.1)
1. npp_lib (formerly silgy_lib) will probably get split into smaller parts.
1. USERS will be based on the [ORM](https://silgy.org/mysqldaogen) layer, thus improving speed and safety. There will be easier to integrate new generated ORM classes into NPP projects.
1. There will be different default hashing algorithm for passwords. (DONE in v0.0.1)
1. HTTP/2
1. epoll will finally be implemented.
1. Popular packaging support.
1. OUT and similar macros will take std::strings as arguments (with C-style strings still supported).

Please add your comments, interest in contributing, wishes, etc. in Issues or silgy.help@gmail.com.


## Silgy project migration to Node++ project

### lib

1. Replace lib directory with the new one


### resmin

1. Rename `silgy.css` to `npp.css`
2. Rename `silgy.js` to `npp.js`


### src

1. Rename `silgy_app.h` to `npp_app.h`
2. Rename `silgy_app.cpp` to `npp_app.cpp`
3. Rename `silgy_svc.cpp` to `npp_svc.cpp`


In all your project files (sources and shell scripts), do case-sensitive replacements:

1. Replace all occurrences of `SILGYDIR` with `NPP_DIR`
2. Replace all occurrences of `silgy.h` with `npp.h`
3. Replace all occurrences of `silgy_` with `npp_`
4. Replace all occurrences of `SILGY_` with `NPP_`


### USERS

If you use USERS module: unless you want to force users to reset their passwords, to keep them in the old format:

1. Define `NPP_SILGY_PASSWORDS` in `npp_app.h`

Note that by default Node++ uses SHA256 hashes instead of SHA1 for passwords. They are longer and require `passwd1` and `passwd2` columns in `users` table to be 44 characters long.


## Ubuntu

Apart from installing GCC, this might be required:

```
sudo apt-get install libz-dev
sudo apt-get install libssl-dev
```
