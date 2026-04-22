#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<unistd.h>

typedef struct report {
    int ID;
    char inspectorname[31];
    float latitude, longitude;
    char category[12];
    int severitylevel;
    time_t timestamp;
    char description[101];
} report;

//functie care creeaza fisier
void creazafisier(const char*cale,int permis)
{
    //se creeaza fisierul daca nu exista, scriu in el si tot ce scriu pun la final
    int fd = open(cale, O_CREAT|O_WRONLY|O_APPEND, permis);

   //cand e gata creat inchid
    if(fd != -1) {
        close(fd);
    }
    //dam permisiunile cerute
    chmod(cale,permis);
}


void setaredistrict(char* district)
{
    char filepath[150];

    // creez folderul districtului
    //vine 0 in fata deoarece sunt pe 8 biti numerele
    mkdir(district, 0750);
    chmod(district, 0750);

    // creez reports.dat (664)
    sprintf(filepath, "%s/reports.dat", district);
    creazafisier(filepath, 0664);

    // creez district.cfg (640)
    sprintf(filepath, "%s/district.cfg", district);
    creazafisier(filepath, 0640);

    // 4. creez logged_district (644)
    sprintf(filepath, "%s/logged_district", district);
    creazafisier(filepath, 0644);
}


int main(int argc, char *argv[]) {
    //variabilele in care stocam ce anume alege utilizatorul
    char role[20] = "";
    char user[31] = "";
    char district[50] = "";
    char action[20] = ""; 

    //
    for (int i=1;i<argc;i++)
    {
        if (strcmp(argv[i],"--role")==0 && i+1<argc)
        {
            strcpy(role,argv[i+1]);
            i++;
        } 
        else if (strcmp(argv[i],"--user")==0 && i+1<argc)
        {
            strcpy(user,argv[i+1]);
            i++;
        } 
        else if (strcmp(argv[i], "--add")==0 && i+1<argc) {
            strcpy(action,"add");
            strcpy(district,argv[i+1]);
            i++;
        }
        else if (strcmp(argv[i],"--list")==0 && i+1<argc) {
            strcpy(action,"list");
            strcpy(district,argv[i+1]);
            i++;
        }

    }



    return 0;
}
