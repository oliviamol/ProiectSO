#include <stdio.h>
#include <stdlib.h> 
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct report {
    int ID;
    char inspectorname[31];
    float latitude, longitude;
    char category[12];
    int severitylevel;
    time_t timestamp;
    char description[101];
}Report;

//functie care creeaza fisier
void creazafisier(const char *cale, int permis)
{
    //se creeaza fisierul daca nu exista, scriu in el si tot ce scriu pun la final
    int fd=open(cale,O_CREAT|O_WRONLY|O_APPEND,permis);

    //cand e gata creat inchid
    if(fd!=-1)
        close(fd);

    //dam permisiunile cerute
    chmod(cale,permis);
}

void setaredistrict(char *district)
{
    char filepath[150];

    // creez folderul districtului
    //vine 0 in fata deoarece sunt pe 8 biti numerele
    mkdir(district,0750);
    chmod(district,0750);

    // creez reports.dat (664)
    sprintf(filepath,"%s/reports.dat",district);
    creazafisier(filepath,0664);

    // creez district.cfg (640)
    sprintf(filepath,"%s/district.cfg",district);
    creazafisier(filepath,0640);

    // creez logged_district (644)
    sprintf(filepath,"%s/logged_district",district);
    creazafisier(filepath,0644);
}

//mode t mode o variabila extrasa din structura stat a fisierului, stie daca fisiserul e director link si ce permisiuni are
//str-locul unde scrie rw-r----
void permisiuni(mode_t mode, char *str)
{
    //plec de la ideea ca nu are nicio permisiune
    strcpy(str,"---------");
    //verific permisiunile pentru OWNER (User)
    if(mode & S_IRUSR) str[0]='r'; // read
    if(mode & S_IWUSR) str[1]='w'; // write
    if(mode & S_IXUSR) str[2]='x'; // eXecute
    //verific permisiunile pentru GROUP
    if(mode & S_IRGRP) str[3]='r';
    if(mode & S_IWGRP) str[4]='w';
    if(mode & S_IXGRP) str[5]='x';
    //verific permisiunile pentru OTHERS
    if(mode & S_IROTH) str[6]='r';
    if(mode & S_IWOTH) str[7]='w';
    if(mode & S_IXOTH) str[8]='x';
}

int verifica_acces(const char *cale, const char *rol, char mod_cerut)
{
    struct stat st;
    if(stat(cale,&st)==-1)
    {
        perror("Eroare stat");
        return 0;
    }
    mode_t mode=st.st_mode;
    if(strcmp(rol,"manager")==0)
    {
        //managerii sunt owneri
        if(mod_cerut=='r' && (mode & S_IRUSR)) return 1;
        if(mod_cerut=='w' && (mode & S_IWUSR)) return 1;
    }
    else if(strcmp(rol,"inspector")==0)
    {
        //inspectorii sunt in grup
        if(mod_cerut=='r' && (mode & S_IRGRP)) return 1;
        if(mod_cerut=='w' && (mode & S_IWGRP)) return 1;
    }

    printf("Acces refuzat pentru rolul %s (lipsa permisiuni %c)\n",rol,mod_cerut);
    return 0;
}

// Functie generata cu ajutorul AI pentru parsarea conditiei de filtrare
// Separa un string de forma "field:operator:value" in cele 3 parti ale sale
// Returneaza 1 daca merge 0 daca da fail
int parse_condition(const char *input, char *field, char *op, char *value)
{
    const char *p=input;
    int i=0;

    // extragem campul pana la primul ':'
    while(*p && *p!=':')
        field[i++]=*p++;
    field[i]='\0';
    if(*p!=':') return 0;
    p++; // sarim ':'

    // extragem operatorul pana la urmatorul ':'
    i=0;
    while(*p && *p!=':')
        op[i++]=*p++;
    op[i]='\0';
    if(*p!=':') return 0;
    p++; // sarim ':'

    // extragem valoarea (restul sirului)
    i=0;
    while(*p)
        value[i++]=*p++;
    value[i]='\0';

    // verificam ca niciun camp nu e gol
    return (field[0]!='\0' && op[0]!='\0' && value[0]!='\0') ? 1 : 0;
}

//functie generata cu ajutorul AI care verifica daca un raport satisface o conditie
//suporta campuri: severity, category, inspector, timestamp
//suporta operatori: ==,!=,<,<=,>,>=
//returneaza 1 daca conditia e indeplinita, 0 altfel
int match_condition(Report *r, const char *field, const char *op, const char *value)
{
    if(strcmp(field,"severity")==0)
    {
        int val=atoi(value);
        int sev=r->severitylevel;
        if(strcmp(op,"==")==0) return sev==val;
        if(strcmp(op,"!=")==0) return sev!=val;
        if(strcmp(op,"<")==0)  return sev<val;
        if(strcmp(op,"<=")==0) return sev<=val;
        if(strcmp(op,">")==0)  return sev>val;
        if(strcmp(op,">=")==0) return sev>=val;
    }
    else if(strcmp(field,"category")==0)
    {
        int cmp=strcmp(r->category,value);
        if(strcmp(op,"==")==0) return cmp==0;
        if(strcmp(op,"!=")==0) return cmp!=0;
    }
    else if(strcmp(field,"inspector")==0)
    {
        int cmp=strcmp(r->inspectorname,value);
        if(strcmp(op,"==")==0) return cmp==0;
        if(strcmp(op,"!=")==0) return cmp!=0;
    }
    else if(strcmp(field,"timestamp")==0)
    {
        // folosim atol() in loc de atoi() deoarece time_t poate depasi int pe 64 biti
        time_t val=(time_t)atol(value);
        time_t ts=r->timestamp;
        if(strcmp(op,"==")==0) return ts==val;
        if(strcmp(op,"!=")==0) return ts!=val;
        if(strcmp(op,"<")==0)  return ts<val;
        if(strcmp(op,"<=")==0) return ts<=val;
        if(strcmp(op,">")==0)  return ts>val;
        if(strcmp(op,">=")==0) return ts>=val;
    }

    return 1; // camp necunoscut - nu filtram
}

//district- in ce folder, username-nume utilizator user, autorul raportului
void add(char *district, char *username)
{
    Report r; // facem o "cutie" goala pentru raportul nostru
    char filepath[150];

    //completam datele care se pun automat
    r.ID=(int)time(NULL); //timp curent ca ID unic
    r.timestamp=time(NULL); //timpul la care s-a facut raportul
    strcpy(r.inspectorname,username); //salvam numele celui care a dat comanda

    //cerem restul datelor de la tastatura
    printf("X: ");
    scanf("%f",&r.latitude);

    printf("Y: ");
    scanf("%f",&r.longitude);

    printf("Category (road/lighting/flooding/other): ");
    scanf("%11s",r.category); // citim un cuvant

    printf("Severity level (1/2/3): ");
    scanf("%d",&r.severitylevel);

    // pentru descriere, curatam enter-ul ramas
    getchar();
    printf("Description: ");
    fgets(r.description,101,stdin); //citim toata propozitia
    r.description[strcspn(r.description,"\n")]=0; // taiem enter-ul de la final, strcspn ul detecteaza pozitia unde se gaseste \n

    // salvam "cutia" in fisierul binar
    sprintf(filepath,"%s/reports.dat",district);

    // deschidem fisierul doar pentru scriere si adaugare la final
    int fd=open(filepath,O_WRONLY|O_APPEND);
    if(fd!=-1)
    {
        // write ia tot struct-ul si il scrie
        write(fd,&r,sizeof(Report));
        close(fd);
        printf("Raportul a fost salvat cu succes in %s!\n",filepath);
    }
    else
        printf("Eroare: Nu am gasit fisierul reports.dat!\n");
}

//ne arata tot ce s a scris toate detaliile
//district-in ce folder sa intre
//detalii tehnice,listeaza probleme
void list(char *district)
{
    char cale[150];
    sprintf(cale,"%s/reports.dat",district);

    struct stat date_fisier;
    if(stat(cale,&date_fisier)==-1)
    {
        printf("Nu gasesc fisierul %s\n",cale);
        return;
    }

    char perm[10];
    permisiuni(date_fisier.st_mode,perm);

    printf("Fisier reports.dat\nPermisiuni: %s\nMarime: %ld bytes\nUltima modificare: %s",
           perm,date_fisier.st_size,ctime(&date_fisier.st_mtime));

    int fd=open(cale,O_RDONLY);
    if(fd==-1) return;

    Report r;
    int gasite=0;

    while(read(fd,&r,sizeof(Report))==sizeof(Report))
    {
        printf("ID: %d | Categ: %s | Grav: %d | Insp: %s\n",r.ID,r.category,r.severitylevel,r.inspectorname);
        printf("Descriere: %s\nData: %s\n",r.description,ctime(&r.timestamp));
        gasite++;
    }

    if(gasite==0) printf("E gol sau filtru prea dur.\n");
    close(fd);
}

// afiseaza detaliile complete ale unui raport cautat dupa ID
// disponibil pentru ambele roluri
void view_report(char *district, int id_cautat)
{
    char cale[150];
    sprintf(cale,"%s/reports.dat",district);

    int fd=open(cale,O_RDONLY);
    if(fd==-1)
    {
        printf("Eroare: Nu am putut deschide reports.dat!\n");
        return;
    }

    Report r;
    int gasit=0;

    while(read(fd,&r,sizeof(Report))==sizeof(Report))
    {
        if(r.ID==id_cautat)
        {
            printf("=== Raport detaliat ===\n");
            printf("ID:          %d\n",  r.ID);
            printf("Inspector:   %s\n",  r.inspectorname);
            printf("Latitudine:  %.4f\n",r.latitude);
            printf("Longitudine: %.4f\n",r.longitude);
            printf("Categorie:   %s\n",  r.category);
            printf("Severitate:  %d\n",  r.severitylevel);
            printf("Timestamp:   %ld\n", (long)r.timestamp);
            printf("Data:        %s",    ctime(&r.timestamp));
            printf("Descriere:   %s\n",  r.description);
            gasit=1;
            break;
        }
    }

    if(!gasit)
        printf("Raportul cu ID %d nu a fost gasit.\n",id_cautat);

    close(fd);
}

//actualizeaza pragul de severitate
void update_threshold(char *district, int prag, const char *rol)
{
    char filepath[150];
    sprintf(filepath,"%s/district.cfg",district);

    // verificam ca permisiunile sunt exact 640 inainte de scriere
    // daca cineva le-a schimbat, refuzam operatiunea
    struct stat st;
    if(stat(filepath,&st)==-1)
    {
        perror("Eroare stat district.cfg");
        return;
    }
    if((st.st_mode & 0777)!=0640)
    {
        char perm[10];
        permisiuni(st.st_mode,perm);
        printf("Eroare: Permisiunile lui district.cfg nu sunt 640 (sunt: %s). Operatiune refuzata!\n",perm);
        return;
    }

    if(!verifica_acces(filepath,rol,'w')) return;

    int fd=open(filepath,O_WRONLY|O_TRUNC);
    if(fd!=-1)
    {
        char buffer[32];
        sprintf(buffer,"threshold=%d\n",prag);
        write(fd,buffer,strlen(buffer));
        close(fd);
        printf("Prag updatat la %d.\n",prag);
    }
}

//functia remove_report
void sterge(char *district, int id_cautat)
{
    char cale[150];
    sprintf(cale,"%s/reports.dat",district);
    int fd=open(cale,O_RDWR);
    if(fd==-1) return;

    Report r;
    int index_gasit=-1;
    int pozitie_curenta=0;
    while(read(fd,&r,sizeof(Report))==sizeof(Report))
    {
        if(r.ID==id_cautat)
        {
            index_gasit=pozitie_curenta;
            break;
        }
        pozitie_curenta++;
    }

    if(index_gasit!=-1)
    {
        off_t citeste_de_la=(off_t)(index_gasit+1)*sizeof(Report);
        off_t scrie_la=(off_t)index_gasit*sizeof(Report);
        while(1)
        {
            lseek(fd,citeste_de_la,SEEK_SET);
            if(read(fd,&r,sizeof(Report))!=(ssize_t)sizeof(Report))
                break;
            lseek(fd,scrie_la,SEEK_SET);
            write(fd,&r,sizeof(Report));
            citeste_de_la+=sizeof(Report);
            scrie_la+=sizeof(Report);
        }
        struct stat date_fisier;
        fstat(fd,&date_fisier);
        ftruncate(fd,date_fisier.st_size-sizeof(Report));
        printf("Sters cu succes!\n");
    }
    else
        printf("Raportul cu ID %d nu a fost gasit.\n",id_cautat);

    close(fd);
}

// filtreaza rapoartele dupa una sau mai multe conditii (AND implicit)
// logica de filtrare scrisa de student; parse_condition si match_condition - AI-assisted
void filter_reports(char *district, int nr_conditii, char **conditii)
{
    char cale[150];
    sprintf(cale,"%s/reports.dat",district);

    struct stat date_fisier;
    if(stat(cale,&date_fisier)==-1)
    {
        printf("Nu gasesc fisierul %s\n",cale);
        return;
    }

    char perm[10];
    permisiuni(date_fisier.st_mode,perm);
    printf("Fisier reports.dat\nPermisiuni: %s\nMarime: %ld bytes\n",perm,date_fisier.st_size);

    int fd=open(cale,O_RDONLY);
    if(fd==-1) return;

    Report r;
    int gasite=0;

    while(read(fd,&r,sizeof(Report))==sizeof(Report))
    {
        int match=1;
        // testam toate conditiile - AND implicit, ne oprim la primul esec
        for(int i=0;i<nr_conditii && match;i++)
        {
            char field[50],op[10],value[50];
            if(!parse_condition(conditii[i],field,op,value))
            {
                printf("Avertisment: conditie invalida: '%s'\n",conditii[i]);
                match=0;
                break;
            }
            if(!match_condition(&r,field,op,value))
                match=0;
        }

        if(match)
        {
            printf("ID: %d | Categ: %s | Grav: %d | Insp: %s\n",r.ID,r.category,r.severitylevel,r.inspectorname);
            printf("Descriere: %s\nData: %s\n",r.description,ctime(&r.timestamp));
            gasite++;
        }
    }

    if(gasite==0) printf("E gol sau filtru prea dur.\n");
    close(fd);
}

//district- unde lucreez
//action-ce operatiune am terminat
//rol manager,insp
//user-nume pers
void loggeddistrict(const char *district, const char *action, const char *role, const char *user)
{
    char filepath[150];
    char log_entry[256];
    sprintf(filepath,"%s/logged_district",district);

    // verificam ca rolul are drept de scriere pe logged_district (644 - doar ownerul/managerul scrie)
    if(!verifica_acces(filepath,role,'w'))
        return;

    int fd=open(filepath,O_WRONLY|O_APPEND);
    if(fd!=-1)
    {
        time_t t=time(NULL);
        // Formatul cerut: timestamp, user, role, action
        sprintf(log_entry,"%ld %s %s %s\n",t,user,role,action);
        write(fd,log_entry,strlen(log_entry));
        close(fd);
    }
    else
        perror("Eroare la deschiderea jurnalului");
}

void gestioneaza_link(char *district)
{
    char tinta[150],nume_link[100];
    sprintf(tinta,"%s/reports.dat",district);
    sprintf(nume_link,"active_reports-%s",district);
    unlink(nume_link);

    // verificam ca tinta exista inainte de creare (detectare dangling)
    struct stat st;
    if(lstat(tinta,&st)==-1)
    {
        printf("Avertisment: Fisierul tinta %s nu exista (link-ul ar fi dangling)!\n",tinta);
        return;
    }

    symlink(tinta,nume_link);
}

// verifica daca un symlink existent este dangling (tinta nu mai exista)
// foloseste lstat (pentru link) + stat (pentru tinta)
void verifica_link_dangling(const char *nume_link)
{
    struct stat lst;

    // lstat ne da info despre link in sine, nu urmareste tinta
    if(lstat(nume_link,&lst)==-1)
        return; // link-ul nu exista, nimic de facut

    if(S_ISLNK(lst.st_mode))
    {
        struct stat st;
        // stat incearca sa urmeze link-ul; esueaza daca tinta lipseste
        if(stat(nume_link,&st)==-1)
            printf("Avertisment: Symlink-ul '%s' este dangling (tinta nu mai exista)!\n",nume_link);
    }
}










//PHASE 2 
//1. functia de stergere district si tot ce contine

void remove_district(const char *role, const char *district)
{
  char nsymlink[256];
  snprintf(nsymlink,sizeof(nsymlink),"active_reports-%s",district);
    if(strcmp(role,"manager")!=0)
    {
        printf("Rol incorect !!!!!!");
        return;
    }

pid_t s=fork();
if(s<0)
{
    printf("eroare la fork");
    return;
}
if( s == 0)
{
    //procesul copil
 execlp("rm","rm","-rf",district,NULL);
 perror("eroare la execlp");

}
if(s != 0 )
{
   //procesul parinte
    waitpid(s,NULL,0);
if(unlink(nsymlink)==-1)
{
    printf("SYMLINK ul nu a fost sters\n");
    return;
}
else{
    printf("SYMLINK ul a fost sters cu succes\n");
}

}


}






//functia de la punctul 3 phase2

void notifica_monitor(const char *district, const char *role, const char *user)
{
    char log_msg[256];
    time_t t = time(NULL);

    int fd = open(".monitor_pid",O_RDONLY);
    if (fd == -1)
    {
        snprintf(log_msg, sizeof(log_msg),
                 "%ld %s %s monitor_NOT_notified (no .monitor_pid)\n",
                 (long)t, user, role);
    }
    else
    {
        char buf[32] = {0};
        read(fd, buf, sizeof(buf) - 1);
        close(fd);
        pid_t monitor_pid = (pid_t)atoi(buf);
        if (monitor_pid <= 0 || kill(monitor_pid, SIGUSR1) == -1)
            snprintf(log_msg, sizeof(log_msg),
                     "%ld %s %s monitor_NOT_notified\n",
                     (long)t, user, role);
        else
        {
            printf("Monitor (PID %d) notificat.\n", (int)monitor_pid);
            snprintf(log_msg, sizeof(log_msg),
                     "%ld %s %s monitor_notified (PID %d)\n",
                     (long)t, user, role, (int)monitor_pid);
        }
    }

    char filepath[150];
    sprintf(filepath, "%s/logged_district", district);
    int lfd = open(filepath, O_WRONLY | O_APPEND);
    if (lfd != -1)
    {
        write(lfd, log_msg, strlen(log_msg));
        close(lfd);
    }
}


int main(int argc, char *argv[])
{
    char role[20]="", user[31]="", district[50]="", action[20]="";
    int val_aux=0;
    char *conditii[32];
    int nr_conditii=0;

    for(int i=1;i<argc;i++)
    {
        if(strcmp(argv[i],"--role")==0)
            strcpy(role,argv[++i]);
        else if(strcmp(argv[i],"--user")==0)
            strcpy(user,argv[++i]);
        else if(strcmp(argv[i],"--add")==0)
        {
            strcpy(action,"add");
            strcpy(district,argv[++i]);
        }
        else if(strcmp(argv[i],"--list")==0)
        {
            strcpy(action,"list");
            strcpy(district,argv[++i]);
        }
        else if(strcmp(argv[i],"--view")==0)
        {
            strcpy(action,"view");
            strcpy(district,argv[++i]);
            val_aux=atoi(argv[++i]);
        }
        else if(strcmp(argv[i],"--remove_report")==0)
        {
            strcpy(action,"remove");
            strcpy(district,argv[++i]);
            val_aux=atoi(argv[++i]);
        }
        else if(strcmp(argv[i],"--update_threshold")==0)
        {
            strcpy(action,"update");
            strcpy(district,argv[++i]);
            val_aux=atoi(argv[++i]);
        }
        else if(strcmp(argv[i],"--filter")==0)
        {
            strcpy(action,"filter");
            strcpy(district,argv[++i]);
            // colectam conditiile ramase pana la urmatorul flag
            while(i+1<argc && argv[i+1][0]!='-')
                conditii[nr_conditii++]=argv[++i];
        } else if(strcmp(argv[i],"--remove_district")==0)
        {
            strcpy(action,"remove_district");
            if(i+1<argc)
            {
            strcpy(district, argv[++i]);
            }
        } 
       
    }

    char cale_r[150];
    sprintf(cale_r,"%s/reports.dat",district);

    // verificam symlink-ul la fiecare rulare
    char nume_link[100];
    sprintf(nume_link,"active_reports-%s",district);
    verifica_link_dangling(nume_link);

    if(strcmp(action,"add")==0)
    {
        setaredistrict(district);
        if(verifica_acces(cale_r,role,'w'))
        {
            add(district,user);
            gestioneaza_link(district);
            if(strcmp(role,"manager")==0)
              loggeddistrict(district,"add",role,user);
             notifica_monitor(district,role,user); 
        }
    }
     else if(strcmp(action,"list")==0)
    {
        if(verifica_acces(cale_r,role,'r'))
        {
            list(district);
             if(strcmp(role,"manager")==0)
              loggeddistrict(district,"list",role,user);
        }
    }
           
    else if(strcmp(action,"view")==0)
    {
        if(verifica_acces(cale_r,role,'r'))
        {
            view_report(district,val_aux);
             if(strcmp(role,"manager")==0)
               loggeddistrict(district,"view",role,user);
        }
    }
    else if(strcmp(action,"remove")==0)
    {
        if(strcmp(role,"manager")!=0)
            printf("Eroare: Doar managerul poate sterge rapoarte!\n");
        else if(verifica_acces(cale_r,role,'w'))
        {
            sterge(district,val_aux);
            gestioneaza_link(district);
             if(strcmp(role,"manager")==0)
              loggeddistrict(district,"remove_report",role,user);
        }
    }
    else if(strcmp(action,"update")==0)
    {
        if(strcmp(role,"manager")!=0)
            printf("Eroare: Doar managerul poate actualiza pragul!\n");
        else
        {
            update_threshold(district,val_aux,role);
            loggeddistrict(district,"update_threshold",role,user);
        }
    }
    else if(strcmp(action,"filter")==0)
    {
        if(nr_conditii==0)
            printf("Eroare: --filter necesita cel putin o conditie (ex: \"severity:>=:2\")\n");
        else if(verifica_acces(cale_r,role,'r'))
        {
            filter_reports(district,nr_conditii,conditii);
             if(strcmp(role,"manager")==0)
               loggeddistrict(district,"filter",role,user);
        }
    }
    else if(strcmp(action,"remove_district")==0)
    {
    
         remove_district(role,district);

    }

    return 0;
}


