# Node++

Node++ is an extreme speed asynchronous HTTP(S) engine and framework for C/C++ web applications. C++ backend can render pages in **microseconds**, even with a database, when used with efficient [DAO/ORM class](https://nodepp.org/mysqldaogen) (see [live demo](https://nodepp.org/products)).

It can act as:

* a static web server;
* a standalone, self-contained, single-process web application;
* a standalone, multi-process web application, connected and load-balanced via standard POSIX queues;
* an HTTP servlet/microservice that makes up a building block of the more complex, distributed architecture.

Node++ library (paired with OpenSSL and optionally MySQL) contains everything that is necessary to build a complete, production-grade solution, including **session management**, **users accounts**, **REST calls** and **multi-language support**.

There is a [RESTful API generator](https://nodepp.org/generators) to generate almost the whole backend code in a few clicks, requiring only an SQL table definition.

Note that the large part of the documentation still points to [Silgy wiki](https://github.com/silgy/silgy/wiki). This is changing as we speak, so to speak...


## Performance

The last test I've done in 2019 show that Node++ Hello World handles [~20,000 requests per second](https://github.com/silgy/silgy/wiki/Performance-test:-select()-vs-poll()) on a single CPU, free AWS EC2 instance.

In case of using Node++ under heavy load or with external API calls, there's the [ASYNC facility](https://github.com/silgy/silgy/wiki/Silgy-ASYNC) designed to prevent main (npp_app) process blocking. ASYNC allows developer to split (or move) the functionality between gateway and multiple service processes.

Node++'s efficiency makes single CPU, 1 GB AWS EC2 t2.micro free instance sufficient to host a fully-fledged web application with a database for thousands of users.

Node++ applications consistently get "Faster than 100% of tested sites" badge from [Pingdom](https://tools.pingdom.com).

<div align="center">
<img src="https://minishare.com/show?p=MWPcAbmY&i=2" width=300/>
</div>

Or – as they now don't publish the percentage – it shows the **complete page load in 67 ms**:

<div align="center">
<img src="https://minishare.com/show?p=PRRizqb2&i=2" width=700/>
</div>


## Security

Node++ has built-in (and enabled by default) protection against most popular attacks, including BEAST, SQL-injection, XSS, and password and cookie brute-force. It does not directly expose the filesystem nor allows any scripting. Its random string generator is FIPS-compliant. CSRF protection is as easy as adding [3 lines to the code](https://github.com/silgy/silgy/wiki/CSRFT_OK).

Default SSL settings give this result:

<div align="center">
<img src="https://minishare.com/show?p=K8GvQDag&i=3" width=700/>
</div>


## Programming

## Basic concept

<div align="center">
<img src="https://minishare.com/show?p=4xlHEJwL&i=3" width=700/>
</div>

### Simplest Hello World

Return static file if present, otherwise "Hello World!".

```source.c++
void npp_app_main(int ci)
{
    OUT("Hello World!");
}
```

### Simple HTML with 2 pages

Application, yet without moving parts.

```source.c++
void npp_app_main(int ci)
{
    if ( REQ("") )  // landing page
    {
        OUT_HTML_HEADER;
        OUT("<h1>%s</h1>", NPP_APP_NAME);
        OUT("<h2>Welcome to my web app!</h2>");
        OUT_HTML_FOOTER;
    }
    else if ( REQ("about") )
    {
        OUT_HTML_HEADER;
        OUT("<h1>%s</h1>", NPP_APP_NAME);
        OUT("<h2>About</h2>");
        OUT("<p>Hello World Sample Node++ Web Application</p>");
        OUT("<p><a href=\"/\">Back to landing page</a></p>");
        OUT_HTML_FOOTER;
    }
    else  // page not found
    {
        RES_STATUS(404);
    }
}
```

### Using query string value

Finally some logic. [QS()](https://github.com/silgy/silgy/wiki/QS) will automatically choose between query string or payload, depending on the HTTP request method.

```source.c++
void npp_app_main(int ci)
{
    if ( REQ("") )  // landing page
    {
        OUT_HTML_HEADER;

        OUT("<h1>%s</h1>", NPP_APP_NAME);
        OUT("<h2>Welcome to my web app!</h2>");

        if ( REQ_DSK )
            OUT("<p>You're on desktop.</p>");
        else  /* REQ_MOB */
            OUT("<p>You're on the phone.</p>");

        OUT("<p>Click <a href=\"/welcome\">here</a> to try my welcoming bot.</p>");

        OUT_HTML_FOOTER;
    }
    else if ( REQ("welcome") )  // welcoming bot
    {
        OUT_HTML_HEADER;
        OUT("<h1>%s</h1>", NPP_APP_NAME);

        OUT("<p>Please enter your name:</p>");
        OUT("<form action=\"welcome\"><input name=\"firstname\" autofocus> <input type=\"submit\" value=\"Run\"></form>");

        QSVAL qs_firstname;   // query string value

        if ( QS("firstname", qs_firstname) )    // if present, bid welcome
            OUT("<p>Welcome %s, my dear friend!</p>", qs_firstname);

        OUT("<p><a href=\"/\">Back to landing page</a></p>");

        OUT_HTML_FOOTER;
    }
    else  // page not found
    {
        RES_STATUS(404);
    }
}
```

Complete 4-page application example is included in the package (see [npp_app.cpp](https://github.com/silgy/nodepp/blob/master/src/npp_app.cpp)).

More examples are available here: [Node++ examples](https://nodepp.org/docs/examples).


## Background

The first [Silgy](https://github.com/silgy/silgy) version I had published on Github, had the following explanation for why I ever started this project:

In 1995, when I landed my first computer job, my PC had an Intel 286 processor and 1 MB of RAM. Disks had spinning plates, nobody heard of SSD. Our office had Novell file server. And whatever we'd do, programs responded **immediately**.

Fast forward to 2018 and my PC has Intel i5 processor and 8 GB of RAM. Everyone can download GCC for free and there's Stackoverflow.com. And guess what? Web applications doing the same things in my intranet now are **painfully slow**.

That's why I've written Silgy. I think all the web applications in the world should be written in it. World would be much better off.

In Silgy you just compile and link your logic into one executable that responds immediately to HTTP requests, without creating a new thread or – God forbid – process. No layers, no dependencies, no layers, translations, layers, converters, layers...

What you get with Silgy:

* **Speed** − measured in µ-seconds.
* **Safety** − nobody can ever see your application logic nor wander through your filesystem nor run scripts. It has build in protection against most popular attacks.
* **Small memory footprint** − a couple of MB for demo app − can be easily reduced for embedded apps.
* **Simple coding** − straightforward approach, easy to understand even for a beginner programmer [see Hello World](https://nodepp.org/docs/examples).
* **All-In-One** − no need to install external modules (besides OpenSSL); Node++ source already contains all the logic required to run the application.
* **Simple deployment / cloud vendor independency** − only one executable file (or files in gateway/services model) to move around.
* **Low TCO** − ~$3 per month for hosting small web application with MySQL server (AWS t2.micro), not even mentioning planet-friendliness.

Web applications like [Budgeter](https://budgeter.org) or [minishare](https://minishare.com) based on Silgy, fit in free 1GB AWS t2.micro instance, together with MySQL server. Typical processing time (between reading HTTP request and writing response to a socket) on 1 CPU t2.micro is around 100 µs (microseconds). Even with the network latency [it still shows](https://minishare.com/show?p=PRRizqb2).
  

## Getting Started (Linux)

I typically use default [Amazon Linux 2 AMI](https://aws.amazon.com/amazon-linux-2) for my applications. It qualifies for free 12 month trial and is well adjusted for their hardware. I've been using several Amazon Linux instances since 2015 and they run for years without a glitch! So I can recommend them with a clear conscience. Use `RH` command versions below if you have chosen this system.

For other distributions see our [cheat sheets](https://nodepp.org/docs).

1. Install GCC

```source.sh
sudo yum install gcc-c++    # RH
sudo apt-get install g++    # Ubuntu
```

2. Get the package

```source.sh
wget https://nodepp.org/nodepp_1.2.0.tar.gz
```

3. Extract the package:

```source.sh
mkdir npp_hello
tar xpzf nodepp_1.2.0.tar.gz -C npp_hello
```

4. Compile:

```source.sh
cd npp_hello/src
./m
```

5. Quick test (start the app in foreground):

```source.sh
./t
```

6. Browser:

```source.sh
<host>:8080
```

You can now browse through the simple web application. At the bottom of each page there's a rendering function name to quickly grasp how Node++ works.

<div align="center">
<img src="https://minishare.com/show?p=mbdOLtBE&i=2" width=745/>
</div>

Press `Ctrl`+`C` to stop.

If you want to start the application in background, use [nppstart](https://github.com/silgy/nodepp/blob/master/bin/nppstart) and [nppstop](https://github.com/silgy/nodepp/blob/master/bin/nppstop) in [bin](https://github.com/silgy/nodepp/tree/master/bin).


## Getting Started (Windows)

For Windows setup I prepared two ready-to-use packages, one with MinGW, the second one with matching OpenSSL and MySQL libraries.

Downloads and quick manual is available here: [Windows Setup](https://nodepp.org/docs/windows_setup).


## Directory structure

Although not necessary, it's good to have **$NPP_DIR** set in the environment, pointing to the project directory. Node++ engine always first looks in `$NPP_DIR/<dir>` for the particular file, with `<dir>` being one of the below:

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


# If you came across Silgy before

Node++ is a [Silgy](https://github.com/silgy/silgy)'s successor.


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
