# Node++

In 1995, when I landed my first computer job, my PC had an Intel 286 processor and 1 MB of RAM. Disks had spinning plates, nobody heard of SSD. Our office had Novell file server. And whatever we'd do, programs responded **immediately**.

Fast forward to 2018 and my PC has Intel i5 processor and 8 GB of RAM. Everyone can download GCC for free and there's Stackoverflow.com. And guess what? Web applications doing the same things in my intranet now are **painfully slow**.

That's why I've written Node++. I think all the web applications in the world should be written in it. World would be much better off.

In Node++ you just compile and link your logic into one executable that responds immediately to HTTP requests, without creating a new thread or — God forbid — process. No layers, no dependencies, no layers, translations, layers, converters, layers...

What you get with Node++:

* **Speed** − measured in µ-seconds.
* **Safety** − nobody can ever see your application logic nor wander through your filesystem nor run scripts. It has build in protection against most popular attacks.
* **Small memory footprint** − a couple of MB for demo app − can be easily reduced for embedded apps.
* **Simple coding** − straightforward approach, easy to understand even for a beginner programmer [see Hello Worlds](https://nodepp.org/hello).
* **All-In-One** − no need to install external modules; Node++ source already contains all the logic required to run the application.
* **Simple deployment / cloud vendor independency** − only one executable file (or files in gateway/services model) to move around.
* **Low TCO** − ~$3 per month for hosting small web application with MySQL server (AWS t2.micro), not even mentioning planet-friendliness.

Node++ is written in ANSI C in order to support as many platforms as possible and it's C++ compilers compatible. Sample [npp_app.cpp](https://github.com/silgy/nodepp/blob/master/src/npp_app.cpp) source module can be C as well as C++ code. Typical application code will look almost the same as in any of the C family language: C++, Java, JavaScript (before it has been destroyed with ES6 and all that arrow functions hell everyone thinks must be used, and hardly anyone really understands). All that could be automated, is automated.

It aims to be All-In-One solution for writing typical web application — traditional HTML rendering model, SPA or mixed. It handles HTTPS, and anonymous and registered user sessions — even with forgotten passwords. Larger applications or those using potentially blocking resources may want to split logic into the set of services talking to the gateway via POSIX queues in an asynchronous manner, using Node++'s [ASYNC](https://github.com/silgy/silgy#async) facility. Macros [CALL_ASYNC](https://github.com/silgy/silgy#void-call_asyncconst-char-service-const-char-data-int-timeout) and [CALL_ASYNC_NR](https://github.com/silgy/silgy#void-call_async_nrconst-char-service-const-char-data) make it as simple as possible.

Web applications like [Budgeter](https://budgeter.org) or [minishare](https://minishare.com) based on Node++, fit in free 1GB AWS t2.micro instance, together with MySQL server. Typical processing time (between reading HTTP request and writing response to a socket) on 1 CPU t2.micro is around 100 µs (microseconds). Even with the network latency [it still shows](https://tools.pingdom.com/#!/bu4p3i/https://budgeter.org).
  
<div align="center">
<img src="https://minishare.com/show?p=MWPcAbmY&i=2" width=418/>
</div>


## Simplicity

In npp_app.cpp:
```source.c++
void npp_app_main(int ci)
{
    OUT("Hello World!");
}
```
Compile with `m` script and run `npp_app` binary (`npp_app.exe` on Windows). That's it, your application is now listening on the port 80 :) (If you want different port, add it as a command line argument)


## Some more details

Node++ supports HTTPS, anonymous and registered user sessions, binary data upload and rudimentary asynchronous services mechanism using shared memory/POSIX queues (Linux/UNIX).

Node++ requires Linux/UNIX or Windows computer with C or C++ compiler for development. GCC is recommended (which is known as MinGW on Windows). Fuss-free deployment and cloud vendor independency means that production machine requires only operating system and `npp_app` executable file(s), and optionally database server if your application uses one.


## Priorities / tradeoffs

Every project on Earth has them. So you'd better know.

1. *Speed*. Usually whenever I get to choose between speed and code duplication, I choose speed.

2. *Speed*. Usually whenever I get to choose between speed and saving memory, I choose speed. So there are mostly statics, stack and arrays, only a few of mallocs. For the same reason static files are read only at the startup and served straight from the memory.

3. *User code simplicity*. Usually whenever I get to choose between versatility and simplicity, I choose simplicity. 99.999% of applications do not require 10 levels of nesting in JSON. If you do need this, there is selection of [libraries](http://json.org) to choose from, or you're a beginner like every programmer once was, and you still need to sweat your way to simple and clean code.

4. *Independency*. I try to include everything I think a typical web application may need in Node++ engine. If there are external libraries, I try to use most ubiquitous and generic ones, like i.e. OpenSSL and link statically. Of course you may prefer to add any library you want and/or link dynamically, there's nothing in Node++ that prevents you from doing so.

5. *What does deployment mean?* If you've written your app in Node++, it means copying executable file to production machine which has nothing but operating system installed. OK, add jpegs and css. Oh — wait a minute — you prefer to learn [how to develop on Kubernetes](https://kubernetes.io/blog/2018/05/01/developing-on-kubernetes/) first, because everyone talks so cool about it... Then I can't help you. I'm actually learning it but only because my organization handles tens or hundreds of thousands requests per second, we have money for servers, development teams, admin teams and my boss made me. If you're Google or Amazon then you definitely need to have something. There is also a [hundred or so](https://en.wikipedia.org/wiki/List_of_build_automation_software) of other build automation software. Good luck with choosing the right one. And good luck with paying for the infrastructure. One of my priorities was to make Node++ app not needing this at all.


## Getting Started (Linux)

1. Install GCC

```
sudo yum install gcc-c++    # RH
sudo apt-get install g++    # Ubuntu
```

2. Install OpenSSL

```
sudo yum install openssl-devel    # RH
sudo apt-get install libssl-dev   # Ubuntu
```

3. Compilation script:

```
cd src
chmod +x m
./m
```

4. Quick test (start the app in foreground):

```
chmod +x t
./t
```

5. Browser:

```
localhost:8080
```
You can now browse through the simple web application. At the bottom of each page there's a rendering function name to quickly grasp how Node++ works.

Press `Ctrl`+`C` to stop.


## Directory structure

Although not necessary, it's good to have $NPP_DIR set in the environment, pointing to the project directory. Node++ engine always first looks in `$NPP_DIR/<dir>` for the particular file, with `<dir>` being one of the below:

### `src`

*(Required only for development)*

* Application sources. It has to contain at least `npp_app.h` and `npp_app.cpp` with `npp_app_main()` inside.
* Compilation script: `m`

### `lib`

*(Required only for development)*

* Node++ engine & library source
* `users.sql`

### `bin`

* Executables, i.e. `npp_app`, `npp_svc`, `npp_watcher`
* Runtime scripts, i.e. `nppstart`, `nppstop`
* Configuration: `npp.conf`
* Strings in additional languages: `strings.LL-CC`
* Blacklist, i.e. `blacklist.txt`
* Whitelist, i.e. `whitelist.txt`
* 10,000 most common passwords: `passwords.txt`

### `res`

Static resources. The whole tree under `res` is publicly available. All the files are read on startup and served straight from the memory. File list is updated once a minute.

* images
* static HTML files
* text files
* fonts
* ...

### `resmin`

Static resources to be minified. The whole tree under `resmin` is publicly available. All the files are read on startup, minified and served straight from the memory. File list is updated once a minute.

* CSS
* JS

### `snippets`

Static parts of rendered content, i.e. HTML snippets.

### `logs`

* Log files


## If you came across Silgy before

Node++ is a [Silgy](https://github.com/silgy/silgy)'s successor.

Among new features you'll find:

1. OpenSSL linkage will be mandatory. All crypto functions that exist in OpenSSL will be removed from npp_lib. (DONE in v0.0.1)
1. There will be different default hashing algorithm for passwords. (DONE in v0.0.1)
1. HTTP/2 (in progress).
1. Semantic versioning. (DONE in v1.0.0)
1. Functions, globals and macros naming consistency. (DONE in v1.0.0)
1. epoll will be added to socket monitoring options.
1. USERS will be based on the [ORM](https://nodepp.org/mysqldaogen) layer, thus improving speed and safety. There will be easier to integrate new generated ORM classes into NPP projects.
1. Popular Linux packaging support.
1. OUT and similar macros will take std::strings as arguments (with C-style strings still supported).

Please add your comments, interest in contributing, wishes, etc. in Issues or silgy.help@gmail.com.

I try to post updates on [Node++'s Facebook page](https://www.facebook.com/nodepp).


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


In `m` and `m.bat` scripts:

1. For `npp_app` executable replace `npp_eng.c` with `npp_eng_app.c`
2. For `npp_svc` executable replace `npp_eng.c` with `npp_eng_svc.c`


In `npp_app.h` add the following line:

`#define NPP_SILGY_COMPATIBILITY`


### USERS

1. In SQL:

```sql
alter table users modify passwd1 char(44);
alter table users modify passwd2 char(44);
alter table users_logins change sesid sessid char(15);
```

If you use USERS module: unless you want to force users to reset their passwords, to keep them in the old format:

1. Define `NPP_SILGY_PASSWORDS` in `npp_app.h`


## Ubuntu

Apart from installing GCC, this might be required:

```
sudo apt-get install libz-dev
sudo apt-get install libssl-dev
```


## Alpine docker

```
RUN apk update && apk add build-base zlib-dev openssl-dev
```
