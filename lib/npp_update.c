/* --------------------------------------------------------------------------

    MIT License

    Copyright (c) 2020 Jurek Muszynski

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

-----------------------------------------------------------------------------

    Node++ Web App Engine
    Update Node++ lib
    nodepp.org

-------------------------------------------------------------------------- */


#include "npp.h"
#include <getopt.h>


static bool M_force_update=FALSE;
static bool M_update=FALSE;

static int  M_local_major=0;
static int  M_local_minor=0;
static int  M_local_patch=0;

static int  M_latest_major=0;
static int  M_latest_minor=0;
static int  M_latest_patch=0;

static JSON M_j={0};


/* --------------------------------------------------------------------------
   Parse command line
-------------------------------------------------------------------------- */
static int parse_command_line(int argc, char *argv[])
{
    int c;

    while ( (c=getopt(argc, argv, "fu")) != -1 )
        switch (c)
        {
            case 'f':
                M_force_update = TRUE;
                break;
            case 'u':
                M_update = TRUE;
                break;
            case '?':
                if ( optopt == 'c' )
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if ( isprint(optopt) )
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                return FAIL;
            default:
                return FAIL;
        }

    return OK;
}


/* --------------------------------------------------------------------------
   Parse version string
-------------------------------------------------------------------------- */
static int parse_verstr(const char *str, int *major, int *minor, int *patch)
{
    sscanf(str, "%d.%d.%d", major, minor, patch);
    DBG("%d %d %d", *major, *minor, *patch);
    return OK;
}


/* --------------------------------------------------------------------------
   Check local Node++ version
-------------------------------------------------------------------------- */
static int check_local_version()
{
    int ret=OK;

    DBG("Opening npp.h...");

    char npp_h[512];

#ifdef _WIN32
    sprintf(npp_h, "%s\\lib\\npp.h", G_appdir);
    HANDLE fd;
#else   /* Linux */
    sprintf(npp_h, "%s/lib/npp.h", G_appdir);
    FILE *fd;
#endif
    int fsize;

#ifdef _WIN32

    fd = CreateFile(TEXT(npp_h), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if ( fd == INVALID_HANDLE_VALUE )
    {
        ERR("Couldn't open npp.h");
        return FAIL;
    }

    /* 64-bit integers complicated by Microsoft... */

    LARGE_INTEGER li_fsize;

    if ( !GetFileSizeEx(fd, &li_fsize) )
    {
        ERR("Couldn't read npp.h's size");
        return FAIL;
    }

    fsize = li_fsize.QuadPart;

//    human_size_t hs;
//    human_size(fsize, &hs);

//    DBG("File size: %lld bytes (%lld KiB / %d MiB / %d GiB)", fsize, hs.kib, hs.mib, hs.gib);

#else   /* Linux */

    if ( NULL == (fd=fopen(npp_h, "r")) )
    {
        ERR("Couldn't open npp.h");
        return FAIL;
    }

    fseek(fd, 0, SEEK_END);     /* determine the file size */

    fsize = ftell(fd);

    rewind(fd);

//    human_size_t hs;
//    human_size(fsize, &hs);

//    DBG("File size: %" PRId64 " bytes (%" PRId64 " KiB / %d MiB / %d GiB)", fsize, hs.kib, hs.mib, hs.gib);

#endif

    /* read file into the buffer */

    char *data;

    if ( !(data=(char*)malloc(fsize+1)) )
    {
        ERR("Couldn't allocate %lld bytes for buffer", fsize+1);
        return FAIL;
    }

#ifdef _WIN32
    DWORD bytes;
    if ( !ReadFile(fd, data, fsize, &bytes, NULL) )
#else   /* Linux */
    if ( fread(data, fsize, 1, fd) != 1 )
#endif
    {
        ERR("Couldn't read from npp.h");
        free(data);
        return FAIL;
    }

#ifdef _WIN32
    CloseHandle(fd);
#else   /* Linux */
    fclose(fd);
#endif

    /* ---------------------------------------------- */

    data[fsize] = EOS;

    /* ---------------------------------------------- */

    char *p_verstr = strstr(data, "NPP_VERSION");

    if ( !p_verstr )
    {
        ERR("Couldn't find NPP_VERSION in npp.h");
        free(data);
        return FAIL;
    }

    p_verstr += 12;

    int count = p_verstr - data;
    char *c=p_verstr;

    while ( *c != '"' && count < fsize )
    {
        ++c;
        ++count;
    }

    ++c;    /* skip '"' */

    char verstr[32];
    int i=0;

    while ( *c != '"' && i < 31 && count < fsize )
    {
        verstr[i++] = *c++;
        ++count;
    }

    verstr[i] = EOS;

    DBG("verstr [%s]", verstr);

    /* ---------------------------------------------- */

    free(data);

    /* ---------------------------------------------- */

    ret = parse_verstr(verstr, &M_local_major, &M_local_minor, &M_local_patch);

    /* ---------------------------------------------- */

    return ret;
}


/* --------------------------------------------------------------------------
   Check latest Node++ version
-------------------------------------------------------------------------- */
static int check_latest_version()
{
    int  ret=OK;
    char url[1024];
    char data[NPP_JSON_BUFSIZE];

#ifdef NPP_HTTPS
    sprintf(url, "https://nodepp.org/update_info");
#else
    sprintf(url, "http://nodepp.org/update_info");
#endif

    if ( !CALL_HTTP(NULL, &data, "GET", url) )
    {
        ERR("Couldn't connect to nodepp.org");
        return FAIL;
    }
    else if ( CALL_HTTP_STATUS != 200 )
    {
        ERR("Couldn't get update_info from nodepp.org, status = %d", CALL_HTTP_STATUS);
        return FAIL;
    }

    data[CALL_HTTP_RESPONSE_LEN] = EOS;

    DBG("data [%s]", data);

    if ( !JSON_FROM_STRING(&M_j, data) )
    {
        ERR("Couldn't parse response as JSON");
        return FAIL;
    }

    char verstr[32];

    COPY(verstr, JSON_GET_STR(&M_j, "version"), 31);

    ret = parse_verstr(verstr, &M_latest_major, &M_latest_minor, &M_latest_patch);

    return ret;
}


/* --------------------------------------------------------------------------
   Update
-------------------------------------------------------------------------- */
static int update_lib()
{
    ALWAYS("Updating Node++ lib...");

    JSON j_files={0};

    if ( !JSON_GET_ARRAY(&M_j, "files", &j_files) )
    {
        ERR("Couldn't get file list from the response");
        return FAIL;
    }

    int cnt=JSON_COUNT(&j_files);

    INF("%d files to download", cnt);

    int  i;
    char path[512];
    char fname[256];
    char url[1024];
static char data[CALL_HTTP_MAX_RESPONSE_LEN];
    char local_path[1024];
    FILE *fd;

    for ( i=0; i<cnt; ++i )
    {
        COPY(path, JSON_GET_STR_A(&j_files, i), 511);

        COPY(fname, npp_get_fname_from_path(path), 255);

//        DBG("fname [%s]", fname);

#ifdef NPP_HTTPS
        sprintf(url, "https://nodepp.org%s", path);
#else
        sprintf(url, "http://nodepp.org%s", path);
#endif
        INF("url [%s]", url);

        if ( !CALL_HTTP(NULL, &data, "GET", url) )
        {
            ERR("Couldn't connect to nodepp.org");
            return FAIL;
        }
        else if ( CALL_HTTP_STATUS != 200 )
        {
            ERR("Couldn't download %s from nodepp.org, status = %d", path, CALL_HTTP_STATUS);
            return FAIL;
        }

        DBG("CALL_HTTP_RESPONSE_LEN = %d", CALL_HTTP_RESPONSE_LEN);

#ifdef _WIN32

        sprintf(local_path, "%s\\lib\\%s", G_appdir, fname);

        if ( NULL == (fd=fopen(local_path, "wb")) )
        {
            ERR("Couldn't create %s", local_path);
            return FAIL;
        }

#else   /* Linux */

        sprintf(local_path, "%s/lib/%s", G_appdir, fname);

        if ( NULL == (fd=fopen(local_path, "w")) )
        {
            ERR("Couldn't create %s", local_path);
            return FAIL;
        }

#endif  /* _WIN32 */

        ALWAYS("Updating %s...", local_path);

        if ( fwrite(data, CALL_HTTP_RESPONSE_LEN, 1, fd) != 1 )
        {
            ERR("Couldn't write to %s", fname);
            fclose(fd);
            return FAIL;
        }

        fclose(fd);

        INF("File has been written to %s", fname);
    }

    ALWAYS("Update successful.");

    return OK;
}


/* --------------------------------------------------------------------------
   main
-------------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    if ( !npp_lib_init() )
        return EXIT_FAILURE;

    G_logLevel = 1;

    DBG("Starting...");

    int ret = parse_command_line(argc, argv);

    if ( ret == FAIL )
    {
        ERR("Couldn't parse command line arguments");
        return EXIT_FAILURE;
    }
    else if ( ret == 1 )
    {
        return EXIT_SUCCESS;
    }

    DBG("M_force_update = %d", M_force_update);
    DBG("M_update = %d", M_update);

    if ( !G_appdir[0] )
    {
        ERR("NPP_DIR is required");
        return EXIT_FAILURE;
    }

    ret = check_local_version();

    if ( ret == OK )
        ALWAYS(" Local version: %d.%d.%d", M_local_major, M_local_minor, M_local_patch);

    if ( ret == OK )
        ret = check_latest_version();

    if ( ret == OK )
        ALWAYS("Latest version: %d.%d.%d", M_latest_major, M_latest_minor, M_latest_patch);

    if ( ret == OK )
    {
        if ( M_latest_major > M_local_major || M_latest_minor > M_local_minor || M_latest_patch > M_local_patch )
        {
            if ( M_update )
            {
                if ( M_latest_major > M_local_major && !M_force_update )    /* confirmation required */
                {
                    printf("Major version has changed. It may be incompatible with your app code. Are you sure you want to proceed [y/N]? ");

                    int answer = getchar();

                    if ( answer != '\n' )
                    {
                        int c;
                        while ( (c=getchar()) != '\n') continue;
                        DBG("c = '%c'", c);
                    }

                    DBG("answer = '%c'", answer);

                    if ( answer == 'y' || answer == 'Y' )
                        ret = update_lib();

                    ALWAYS("To remove prompt add -f option to force update");
                }
                else
                    ret = update_lib();
            }
            else    /* info only */
            {
                ALWAYS("Add -u option to update");
            }
        }
        else    /* same version */
        {
            ALWAYS("Nothing to update.");
        }
    }

    npp_lib_done();
    
    if ( ret != OK )
        return EXIT_FAILURE;
    
    return EXIT_SUCCESS;
}
