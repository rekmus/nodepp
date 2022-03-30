/* --------------------------------------------------------------------------

    MIT License

    Copyright (c) 2020-2022 Jurek Muszynski (rekmus)

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
static bool M_test=FALSE;
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

    while ( (c=getopt(argc, argv, "ftu")) != -1 )
        switch (c)
        {
            case 'f':
                M_force_update = TRUE;
                break;
            case 't':
//                M_test = TRUE;
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
    DDBG("%d %d %d", *major, *minor, *patch);
    return OK;
}


/* --------------------------------------------------------------------------
   Check local Node++ version
-------------------------------------------------------------------------- */
static int check_local_version()
{
    int ret=OK;

    DDBG("Opening npp.h...");

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

    fd = CreateFile(TEXT(npp_h), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

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
        CloseHandle(fd);
        return FAIL;
    }

    fsize = li_fsize.QuadPart;

#else   /* Linux */

    if ( NULL == (fd=fopen(npp_h, "r")) )
    {
        ERR("Couldn't open npp.h");
        return FAIL;
    }

    fseek(fd, 0, SEEK_END);     /* determine the file size */

    fsize = ftell(fd);

    rewind(fd);

#endif

    /* read file into the buffer */

    char *data;

    if ( !(data=(char*)malloc(fsize+1)) )
    {
        ERR("Couldn't allocate %lld bytes for buffer", fsize+1);
#ifdef _WIN32
        CloseHandle(fd);
#else   /* Linux */
        fclose(fd);
#endif
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
#ifdef _WIN32
        CloseHandle(fd);
#else   /* Linux */
        fclose(fd);
#endif
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

    DDBG("verstr [%s]", verstr);

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
    sprintf(url, "https://nodepp.org/api/v2/update_info");
#else
    sprintf(url, "http://nodepp.org/api/v2/update_info");
#endif

    if ( !CALL_HTTP(NULL, &data, "GET", url, FALSE) )
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

    DDBG("data [%s]", data);

    if ( !JSON_FROM_STRING(&M_j, data) )
    {
        ERR("Couldn't parse response as JSON");
        return FAIL;
    }

    DBG("parsed pretty: [%s]", JSON_TO_STRING_PRETTY(&M_j));

    char verstr[32];

#ifdef NPP_JSON_V1
    COPY(verstr, JSON_GET_STR(&M_j, "version"), 31);
#else
    if ( !JSON_GET_STR(&M_j, "version", verstr, 31) )
    {
        ERR("Couldn't find version in response");
        return FAIL;
    }
#endif
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

    INF("%d files to update", cnt);

    int  i;
    char path[NPP_JSON_STR_LEN+1];
    char fname[128];
    char url[512];
static char data[CALL_HTTP_MAX_RESPONSE_LEN];
    FILE *fd;

    for ( i=0; i<cnt; ++i )
    {
        JSON j_rec;
        JSON_RESET(&j_rec);

        JSON_GET_RECORD_A(&j_files, i, &j_rec);

#ifdef NPP_JSON_V1
        COPY(path, JSON_GET_STR(&j_rec, "path"), NPP_JSON_STR_LEN);
#else
        if ( !JSON_GET_STR(&j_rec, "path", path, NPP_JSON_STR_LEN) )
        {
            ERR("Couldn't find path in response");
            return FAIL;
        }
#endif
        COPY(fname, npp_get_fname_from_path(path), 127);

        DDBG("fname [%s]", fname);

        /* ------------------------------------------------ */
        /* extract directory from path */

        char directory[64];
        int  pos;

        char *p = strrchr(path, '/');

        if ( p == NULL )
        {
            ERR("No / in path");
            continue;
        }

        pos = p - path;

        char temp[256];
        strncpy(temp, path, pos);
        temp[pos] = EOS;

        DDBG("temp [%s]", temp);

        p = strrchr(temp, '/');

        if ( p == NULL )
        {
            ERR("No / in temp");
            continue;
        }

        COPY(directory, p+1, 63);

        DDBG("directory [%s]", directory);

        /* ------------------------------------------------ */
        /* local path */

        char local_path[512];

#ifdef _WIN32
        sprintf(local_path, "%s\\%s\\%s", G_appdir, directory, fname);
#else
        sprintf(local_path, "%s/%s/%s", G_appdir, directory, fname);
#endif

        /* ------------------------------------------------ */
        /* get flags */

        char flags[NPP_JSON_STR_LEN+1]="";

#ifdef NPP_JSON_V1
        strcpy(flags, JSON_GET_STR(&j_rec, "flags"));
#else
        if ( !JSON_GET_STR(&j_rec, "flags", flags, NPP_JSON_STR_LEN) )
        {
            WAR("Couldn't find flags in response");
        }
#endif
        /* ------------------------------------------------ */
        /* delete? */

        if ( flags[0]=='d' )
        {
            INF("Deleting %s...", local_path);
            remove(local_path);
        }
        else    /* download */
        {
#ifdef NPP_HTTPS
            sprintf(url, "https://nodepp.org%s", path);
#else
            sprintf(url, "http://nodepp.org%s", path);
#endif
            DBG("url [%s]", url);

            if ( !CALL_HTTP(NULL, &data, "GET", url, FALSE) )
            {
                ERR("Couldn't connect to nodepp.org");
                return FAIL;
            }
            else if ( CALL_HTTP_STATUS != 200 )
            {
                ERR("Couldn't download %s from nodepp.org, status = %d", path, CALL_HTTP_STATUS);
                return FAIL;
            }

            DDBG("CALL_HTTP_RESPONSE_LEN = %d", CALL_HTTP_RESPONSE_LEN);

#ifdef _WIN32
            if ( NULL == (fd=fopen(local_path, "wb")) )
#else
            if ( NULL == (fd=fopen(local_path, "w")) )
#endif
            {
                ERR("Couldn't create %s", local_path);
                return FAIL;
            }

            /* ------------------------------------------------ */
            /* save file */

            ALWAYS("Updating %s...", local_path);

            if ( fwrite(data, CALL_HTTP_RESPONSE_LEN, 1, fd) != 1 )
            {
                ERR("Couldn't write to %s", fname);
                fclose(fd);
                return FAIL;
            }

            fclose(fd);

            INF("File has been written to %s", fname);

#ifndef _WIN32

            if ( flags[0]=='x' )    /* execute permission required */
            {
                struct stat fstat;

                if ( stat(local_path, &fstat) != 0 )
                {
                    ERR("stat for [%s] failed, errno = %d (%s)", local_path, errno, strerror(errno));
                    continue;
                }

                if ( !(fstat.st_mode & S_IXUSR) )   /* user does not have execute permission */
                {
                    DBG("Setting executable flag...");
                    fstat.st_mode |= S_IXUSR;
                    chmod(local_path, fstat.st_mode);
                }
            }

#endif  /* _WIN32 */

        }
    }

    ALWAYS("Update successful.");

    return OK;
}


/* --------------------------------------------------------------------------
   Print usage
-------------------------------------------------------------------------- */
void usage(int argc, char *argv[])
{
    ALWAYS("");
    ALWAYS("Node++ library update. Usage:");
    ALWAYS("");
    ALWAYS("%s [-uf]", argv[0]);
    ALWAYS("");
    ALWAYS("\tno options = show info", argv[0]);
    ALWAYS("\t-u = update");
    ALWAYS("\t-f = force update (if major version has changed)");
}


/* --------------------------------------------------------------------------
   main
-------------------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    if ( !npp_lib_init(FALSE, NULL) )
        return EXIT_FAILURE;

    int ret = parse_command_line(argc, argv);

    if ( M_test )
    {
        G_logLevel = LOG_DBG;
        DBG("TEST MODE");
    }
    else
        G_logLevel = LOG_ERR;

    if ( ret == FAIL )
    {
        ERR("Couldn't parse command line arguments");
        return EXIT_FAILURE;
    }
    else if ( ret == 1 )
    {
        return EXIT_SUCCESS;
    }

    DDBG("M_force_update = %d", M_force_update);
    DDBG("M_update = %d", M_update);

    if ( !M_update )
        usage(argc, argv);
    else
    {
        ALWAYS("");
        ALWAYS("Node++ library update.");
    }

    ALWAYS("");

    if ( !G_appdir[0] )
        strcpy(G_appdir, "..");

    ret = check_local_version();

    if ( ret == OK )
        ALWAYS(" Local version: %d.%d.%d", M_local_major, M_local_minor, M_local_patch);

    if ( ret == OK )
        ret = check_latest_version();

    if ( ret == OK )
        ALWAYS("Latest version: %d.%d.%d", M_latest_major, M_latest_minor, M_latest_patch);

    if ( ret == OK )
    {
        if ( M_test || (M_latest_major > M_local_major
                || (M_latest_major == M_local_major && M_latest_minor > M_local_minor)
                || (M_latest_major == M_local_major && M_latest_minor == M_local_minor && M_latest_patch > M_local_patch)) )
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
                        DDBG("c = '%c'", c);
                    }

                    DDBG("answer = '%c'", answer);

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
            ALWAYS("");
            ALWAYS("Nothing to update.");
        }
    }

    ALWAYS("");
    ALWAYS("More info: nodepp.org");
#ifndef _WIN32
    ALWAYS("");
#endif

    npp_lib_done();

    if ( ret != OK )
        return EXIT_FAILURE;
    
    return EXIT_SUCCESS;
}
