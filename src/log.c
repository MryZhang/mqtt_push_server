#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

FILE *fp;
static int SESSION_TRACKER;

char* print_time()
{
    time_t t;
    char *buf;

    time(&t);
    char *c_t = ctime(&t);
    buf = (char *) malloc(strlen(ctime(&t)) + 1);
    snprintf(buf, strlen(ctime(&t)), "%s ", ctime(&t));

    return buf;
}

void log_print(char *filename, int line, char *fmt,...)
{
    va_list list;
    char *p, *r;
    int e;

    if(SESSION_TRACKER > 0)
        fp = fopen("log.txt", "a+");
    else
        fp = fopen("log.txt", "w");

    fprintf(fp, "%s ", print_time());
    va_start(list, fmt);

    for(p = fmt; *p; ++p)
    {
        if( *p != '%')
        {
            fputc(*p,fp);
        }else
        {
            switch(*++p)
            {
            case 's':
                r = va_arg(list, char*);
                fprintf(fp, "%s", r);
                continue;
                break;
            case 'd':
                e = va_arg(list, int);
                fprintf(fp, "%d", e);
                continue;
                break;
            default:
                fputc(*p, fp);
            }
        }
    }
    va_end(list);
    fprintf(fp, " [%s][line: %d] ", filename, line);
    fputc('\n', fp);
    SESSION_TRACKER++;
    fclose(fp);
}
