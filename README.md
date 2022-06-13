# Node++

Node++ is an asynchronous HTTP(S) engine and backend framework for low-latency C/C++ web applications and RESTful APIs. C++ backend can render pages in **microseconds**, even with a database, when used with efficient [DAO/ORM class](https://nodepp.org/generators/mysqldao) (see [live demo](https://nodepp.org/products)).

It can act as:

* a static web server;
* a standalone, self-contained, single-process web application;
* a standalone, multi-process web application, connected and load-balanced via standard POSIX queues;
* an HTTP servlet/microservice that makes up a building block of the more complex, distributed architecture.

Node++ library (paired with OpenSSL and optionally MySQL) contains everything that is necessary to build a complete, production-grade solution, including **session management**, **users accounts**, **REST calls** and **multi-language support**.

There is a [RESTful API generator](https://nodepp.org/generators) to generate almost the whole backend code in a few clicks, requiring only an SQL table definition.


## Framework

The "backend framework" means that there are 7 server-side events you can react to:

Event|Function called
-----|----
HTTP request|`npp_app_main`
Application start|`npp_app_init`
Application stop|`npp_app_done`
Session start|`npp_app_session_init`
Session stop|`npp_app_session_done`
Session authentication (login)|`npp_app_user_login`
Session logout|`npp_app_user_logout`

[npp_app.cpp](https://github.com/rekmus/nodepp/blob/master/src/npp_app.cpp) has to contain these definitions (even if empty).

The whole session and sessid cookie management is handled by the engine.


## Performance

Node++'s efficiency makes single CPU, 1 GB AWS EC2 t2.micro free instance sufficient to host a fully-fledged web application with a database for thousands of users.

Node++ applications consistently get "Faster than 100% of tested sites" badge from [Pingdom](https://tools.pingdom.com).

<div align="center">
<img src="https://minishare.com/show?p=MWPcAbmY&i=2" width=300>
</div>

Or – as they now don't publish the percentage – it shows the **complete page load in 67 ms**:

<div align="center">
<img src="https://minishare.com/show?p=PRRizqb2&i=2" width=600>
</div>


## Security

Node++ has built-in (and enabled by default) protection against most popular attacks, including BEAST, SQL-injection, XSS, and password and cookie brute-force. It does not directly expose the filesystem nor allows any scripting. Its random string generator is FIPS-compliant. CSRF protection is as easy as adding [3 lines to the code](https://github.com/rekmus/nodepp/wiki/CSRFT_OK).

Default SSL settings give this result:

<div align="center">
<img src="https://minishare.com/show?p=K8GvQDag&i=3" width=600>
</div>


## Programming

### Basic convention

<div align="center">
<img src="https://minishare.com/show?p=4xlHEJwL&i=5" width=600>
</div>

If `resource` is a file present in `res` or `resmin` (i.e. an image or css), it will be served and `npp_app_main()` will not be called.


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
        OUT("<p><a href=\"/about\">About</a></p>");
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

This is a simple 2-pages application demonstrating [QS()](https://github.com/rekmus/nodepp/wiki/QS) macro usage to get a value provided by client in the query string.

`QS` works with all popular HTTP methods and payload types.

```source.c++
void npp_app_main(int ci)
{
    if ( REQ("") )  // landing page
    {
        OUT_HTML_HEADER;    // generic HTML header with opening body tag

        OUT("<h1>%s</h1>", NPP_APP_NAME);

        OUT("<h2>Welcome to my web app!</h2>");

        /* show client type */

        if ( REQ_DSK )
            OUT("<p>You're on desktop, right?</p>");
        else if ( REQ_TAB )
            OUT("<p>You're on tablet, right?</p>");
        else  /* REQ_MOB */
            OUT("<p>You're on the phone, right?</p>");

        /* link to welcome */

        OUT("<p>Click <a href=\"/welcome\">here</a> to try my welcoming bot.</p>");

        OUT_HTML_FOOTER;    // body and html closing tags
    }
    else if ( REQ("welcome") )  // welcoming bot
    {
        OUT_HTML_HEADER;
        OUT("<h1>%s</h1>", NPP_APP_NAME);

        /* display form */

        OUT("<p>Please enter your name:</p>");
        OUT("<form action=\"welcome\"><input name=\"name\" autofocus> <input type=\"submit\" value=\"Run\"></form>");

        /* try to get the query string value */

        QSVAL qs_name;   // query string value

        if ( QS("name", qs_name) )    // if present, bid welcome
            OUT("<p>Welcome %s, my dear friend!</p>", qs_name);

        OUT("<p><a href=\"/\">Back to landing page</a></p>");

        OUT_HTML_FOOTER;
    }
    else  // page not found
    {
        RES_STATUS(404);
    }
}
```

Complete 4-page application example is included in the package (see [npp_app.cpp](https://github.com/rekmus/nodepp/blob/master/src/npp_app.cpp)).

More examples are available here: [Node++ examples](https://nodepp.org/docs/examples).


## Configuration

Configuration file ([bin/npp.conf](https://github.com/rekmus/nodepp/blob/master/bin/npp.conf)) contains a handful of settings, starting with logLevel, httpPort, httpsPort, SSL certificate paths, database credentials and so on.

You can also add your own and read them in `npp_app_init()` with [npp_read_param_str()](https://github.com/rekmus/nodepp/wiki/npp_read_param_str) or [npp_read_param_int()](https://github.com/rekmus/nodepp/wiki/npp_read_param_int).


## Documentation

### [Step-by-step tutorial how to set up complete system on AWS EC2 Linux instance](https://nodepp.org/docs/tutorials)

### [Getting Started on Linux](https://github.com/rekmus/nodepp/wiki/Node++-Hello-World-%E2%80%93-Getting-Started-on-Linux)

### [Getting Started on Windows](https://github.com/rekmus/nodepp/wiki/Node++-Hello-World-%E2%80%93-Getting-Started-on-Windows)

### [Switches and constants](https://github.com/rekmus/nodepp/wiki/Node++-switches-and-constants)

### [Functions and macros](https://github.com/rekmus/nodepp/wiki/Node++-functions-and-macros)

### [Configuration parameters](https://github.com/rekmus/nodepp/wiki/Node++-configuration-parameters)

### [How to enable HTTPS](https://github.com/rekmus/nodepp/wiki/How-to-enable-HTTPS-in-Node++)

### [Sessions in Node++](https://github.com/rekmus/nodepp/wiki/Sessions-in-Node++)

### [ASYNC (multi-process mode)](https://github.com/rekmus/nodepp/wiki/Node++-ASYNC)

### [Memory models](https://github.com/rekmus/nodepp/wiki/Node++-memory-models)

### [Multi-language support](https://github.com/rekmus/nodepp/wiki/Node++-multi-language-support)

### [USERS Module](https://github.com/rekmus/nodepp/wiki/USERS-Module)

### [Error codes](https://github.com/rekmus/nodepp/wiki/Node++-error-codes)

### [RESTful calls from Node++](https://github.com/rekmus/nodepp/wiki/RESTful-calls-from-Node++)

### [What is npp_watcher](https://github.com/rekmus/nodepp/wiki/What-is-npp_watcher)




## Directory structure

Although not necessary, it's good to have **$NPP_DIR** set in the environment, pointing to the project directory. Node++ engine always first looks in `$NPP_DIR/<dir>` for the particular file, with `<dir>` being one of the below:

### `src`

*(Required only for development)*

* Application sources. It has to contain at least `npp_app.h` and `npp_app.cpp` with `npp_app_main()` inside.
* Customizable compilation script: `m`

### `lib`

*(Required only for development)*

* Node++ engine & library source
* `users.sql`

### `bin`

* Main compilation script: `nppmake`
* Executables, i.e. `npp_app`, `npp_svc`, `npp_watcher`, `npp_update`
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

Static parts of rendered content, i.e. HTML or markdown snippets.

### `logs`

* Log files
