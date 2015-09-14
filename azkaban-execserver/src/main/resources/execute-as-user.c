#include <dirent.h>
#include <fcntl.h>
#include <fts.h>
#include <errno.h>
#include <grp.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <pwd.h>

  FILE *LOGFILE = NULL;
  FILE *ERRORFILE = NULL;
  int SETUID_OPER_FAILED=10;

/**
 *  * Change the real and effective user and group to abandon the super user
 *   * priviledges.
 *    */
int change_user(uid_t user, gid_t group) {
    if (user == getuid() && user == geteuid() &&
            group == getgid() && group == getegid()) {
        return 0;
    }

    if (seteuid(0) != 0) {
        fprintf(LOGFILE, "unable to reacquire root - %s\n", strerror(errno));
        fprintf(LOGFILE, "Real: %d:%d; Effective: %d:%d\n",
                getuid(), getgid(), geteuid(), getegid());
        return SETUID_OPER_FAILED;
    }
    if (setgid(group) != 0) {
        fprintf(LOGFILE, "unable to set group to %d - %s\n", group,
                strerror(errno));
        fprintf(LOGFILE, "Real: %d:%d; Effective: %d:%d\n",
                getuid(), getgid(), geteuid(), getegid());
        return SETUID_OPER_FAILED;
    }
    if (setuid(user) != 0) {
        fprintf(LOGFILE, "unable to set user to %d - %s\n", user, strerror(errno));
        fprintf(LOGFILE, "Real: %d:%d; Effective: %d:%d\n",
                getuid(), getgid(), geteuid(), getegid());
        return SETUID_OPER_FAILED;
    }

    return 0;
}

int main(int argc, char **argv){
    if (argc < 3) {
        fprintf(ERRORFILE, "Requires at least 3 variables: ./execute-as-user uid command [args]");
    }

    if(!LOGFILE)
        LOGFILE=stdout;
    if(!ERRORFILE)
        ERRORFILE=stderr;



    char *uid = argv[1];

    // for loop to calculate the length to malloc
    int i;
    int total_len = 0;
    for(i=2;i<argc;i++){
        total_len += strlen(argv[i])+1;
    }
    printf("total_len: %d\n", total_len);

    // allocate memory and clear memory
    char *cmd = malloc(total_len+2);
    memset(cmd, ' ', total_len+2);

    // change user 
    struct passwd *userInfo = getpwnam(uid);
    fprintf(LOGFILE, "Changing user: user: %s, uid: %d, gid: %d\n", uid, userInfo->pw_uid, userInfo->pw_gid);
    int retval = change_user(userInfo->pw_uid, userInfo->pw_gid);
    printf("retval: %d\n", retval);
    if( retval != 0){
        fprintf(LOGFILE, "Error changing user to %s\n", uid);
        return SETUID_OPER_FAILED;
    }

    // create the command
    char *cur = cmd;
    int len;
    for(i=2;i<argc;i++){
        len = strlen(argv[i]);
        memcpy(cur, argv[i], len);
        cur+=len+1;
        printf("%s\n", cmd);
    }

    retval = system(cmd);
    fprintf(LOGFILE, "system call return value: %d", retval);

    // sometimes system(cmd) returns 256, which is interpreted to 0, making a failed job a successful job
    // hence this goofy piece of if statement.
    if(retval != 0)
        return 1;
    else
        return 0;

}
