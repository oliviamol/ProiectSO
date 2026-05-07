#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

void createmonitorpid(const char*district)
{
   char fp[256];
   snprintf(fp,sizeof(fp),"%s/.monitor_pid",district);
   int fd=open(fp, O_WRONLY| O_CREAT | O_APPEND,0644);
   if(fd==-1)
   return ;
printf("Fisierul monitorpid a fost creat cu succes");
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
        const char msg[]="A fost detectat un raport nou adaugat!";
        write(STDOUT_FILENO,msg,strlen(msg));
    }
    if(sig=SIGINT)
    {
       const char msg[]="A fost detectat un semnal";
       write(STDOUT_FILENO,msg,strlen(msg));
       unlink(".monitor_pid");


    }


 }





int main()
{
  createmonitorpid(".");
    return 0;
}