#include <time.h>
#include <stdio.h>
#include <execinfo.h>
#include<string>
#include <unistd.h>
#include <stdlib.h>

#include <libnotify/notify.h>
#include <iostream>

void exit_with_backtrace(std::string errstr) 
{
  void *array[10];
  size_t size;

  fprintf(stderr, "%s", errstr.c_str());
  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  // todo: add line numbers and demangle symbols
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  _exit(1);
}

void logmsg(const char* format, ...)
{
    time_t now_tm;
    char *now;

    now_tm = time(NULL);
    now = ctime(&now_tm);

    fprintf(stdout, "[%d] [%.22s] ", getpid(), now);
    va_list ap;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
    fflush(stdout);
}

void notify_user(const std::string summary,const std::string body)
{

    time_t now_tm;
    char *now;

    now_tm = time(NULL);
    now = ctime(&now_tm);

    std::string temp(now);
    temp=temp+summary;
    NotifyNotification* notification(notify_notification_new(
                                            temp.c_str(),
                                            body.c_str(),
                                            0));

    notify_notification_set_timeout(notification, 1000); // 10 seconds

    if (!notify_notification_show(notification, 0)) 
    {
        exit_with_backtrace("error showing notification");
    }
}

void notify_init()
{
    if (!notify_init("Clippy"))
    {
        exit_with_backtrace("notify_init failed\n");
    }
}