#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h> 
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#define PID_FILE ".monitor_pid"

void createmonitorpid(const char*district)
{
   char fp[256];
   snprintf(fp,sizeof(fp),"%s/.monitor_pid",district);
   int fd=open(fp, O_WRONLY| O_CREAT | O_TRUNC,0644);//trunc -creates or overwrites, deci dacă există deja trebuie suprascris
   if(fd==-1)
   return ;
printf("Fisierul monitorpid a fost creat cu succes\n");
char buffer[256];
pid_t p=getpid();
int lung=snprintf(buffer,sizeof(buffer),"%d",p);
write(fd,buffer,lung);
close(fd);
}


 void signals(int sig)
 {


    if(sig==SIGUSR1)
    {
        const char msg[]="A fost detectat un raport nou adaugat!\n";
        write(STDOUT_FILENO,msg,strlen(msg));
    }
    if(sig==SIGINT)
    {
       const char msg[]="A fost detectat un semnal\n";
       write(STDOUT_FILENO,msg,strlen(msg));
       unlink(".monitor_pid");
      _exit(0);


    }


 }





int main()
{
  createmonitorpid(".");
  
    struct sigaction action_usr1;
    memset(&action_usr1, 0x00, sizeof(struct sigaction));
    action_usr1.sa_handler = signals;   
    sigemptyset(&action_usr1.sa_mask);
    action_usr1.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &action_usr1, NULL) < 0)
    {
        perror("sigaction SIGUSR1");
        unlink(PID_FILE);
        exit(-1);
    }

   
    struct sigaction action_int;
    memset(&action_int, 0x00, sizeof(struct sigaction));
    action_int.sa_handler = signals;
    sigemptyset(&action_int.sa_mask);
    action_int.sa_flags = 0;            

    if (sigaction(SIGINT, &action_int, NULL) < 0)
    {
        perror("sigaction SIGINT\n");
        unlink(PID_FILE);
        exit(-1);
    }

    printf("PID %d Astept semnale...\n", (int)getpid());

    while (1)
        pause();

    return 0;
    
}