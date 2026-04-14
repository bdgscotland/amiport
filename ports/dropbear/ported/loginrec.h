/* amiport: loginrec.h stub -- no utmp/wtmp/lastlog on AmigaOS */
#ifndef DROPBEAR_LOGINREC_H_
#define DROPBEAR_LOGINREC_H_

struct logininfo {
    char username[64];
    char hostname[256];
    char line[64];
    int pid;
    int uid;
    int type;
    unsigned long tv_sec;
};

#endif
