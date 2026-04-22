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




void add(char*district, char*username)
{
    report r; // facem o "cutie" goala pentru raportul nostru
    char filepath[150];

    //completam datele care se pun automat
    r.ID=time(NULL); //timp curent ca ID unic
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
    fgets(r.description, 101, stdin); //citim toata propozitia
    r.description[strcspn(r.description, "\n")] = 0; // taiem enter-ul de la final

    // salvam "cutia" in fisierul binar
    sprintf(filepath, "%s/reports.dat", district);

    // deschidem fisierul doar pentru scriere si adaugare la final
    int fd = open(filepath, O_WRONLY | O_APPEND);
    if (fd != -1) {
        // write ia tot struct-ul si il scrie
        write(fd, &r, sizeof(report));
        close(fd);
        printf("Raportul a fost salvat cu succes in %s!\n", filepath);
    } else {
        printf("Eroare: Nu am gasit fisierul reports.dat!\n");
    }
}


void scrieinjurnal(const char* district, const char* action, const char* role, const char* user)
{
    char filepath[150];
    char log_entry[256];
    //calea catre fisier
    sprintf(filepath, "%s/logged_district", district);
    //deschid fisierul doar pentru scriere si adaugare la final (O_APPEND)
    int fd = open(filepath, O_WRONLY | O_APPEND);
    if (fd!=-1) {
        //timpul curent
        time_t t=time(NULL);
        sprintf(log_entry, "%ld\n%s\n%s %s\n", t, user, role, action);

        write(fd, log_entry, strlen(log_entry));
        close(fd);
    } else
    {
        printf("Eroare la deschiderea jurnalului pentru districtul %s\n", district);
    }
}


void permisiuni(mode_t mode,char *str) {
    //plec de la ideea ca nu are nicio permisiune
    strcpy(str, "---------");
    //verific permisiunile pentru OWNER (User)
    if (mode & S_IRUSR) str[0] = 'r'; // read
    if (mode & S_IWUSR) str[1] = 'w'; // write
    if (mode & S_IXUSR) str[2] = 'x'; // eXecute
    //verific permisiunile pentru GROUP
    if (mode & S_IRGRP) str[3] = 'r';
    if (mode & S_IWGRP) str[4] = 'w';
    if (mode & S_IXGRP) str[5] = 'x';
    //verific permisiunile pentru OTHERS
    if (mode & S_IROTH) str[6] = 'r';
    if (mode & S_IWOTH) str[7] = 'w';
    if (mode & S_IXOTH) str[8] = 'x';
}

int verifica_acces(const char* cale, const char* rol, char mod_cerut)
{
    struct stat st;
    if (stat(cale, &st) == -1) {
        perror("Eroare stat");
        return 0;
    }
    mode_t mode = st.st_mode;
    if (strcmp(rol, "manager") == 0) {
        // Managerii sunt owneri [cite: 29]
        if (mod_cerut=='r' && (mode & S_IRUSR)) return 1;
        if (mod_cerut=='w' && (mode & S_IWUSR)) return 1;
    } else if (strcmp(rol, "inspector") == 0) {
        // Inspectorii sunt în grup [cite: 29]
        if (mod_cerut=='r' && (mode & S_IRGRP)) return 1;
        if (mod_cerut=='w' && (mode & S_IWGRP)) return 1;
    }

    printf("Acces refuzat pentru rolul %s (lipsa permisiuni %c)\n", rol, mod_cerut);
    return 0;
}


void list(char*district)
{
    char cale[150];
    //lipesc numele districtului de fisier downtown/reports.dat
    sprintf(cale,"%s/reports.dat",district);

    //aici tinem datele despre fisier (marime, permisiuni etc)
    struct stat date_fisier;
    //extragem datele
    if(stat(cale,&date_fisier)==-1){
        printf("Nu gasesc fisierul %s\n",cale);
        return;
    }

    // Facem un sir gol si chemam functia noastra sa il umple cu litere rwx
    char perm[10];
    permisiuni(date_fisier.st_mode,perm);

    printf("Fisier reports.dat\n");
    printf("Permisiuni: %s\n",perm);
    printf("Marime: %ld bytes\n",date_fisier.st_size);
    printf("Modificat: %s",ctime(&date_fisier.st_mtime));
    //deschid fisierul doar sa citesc din el

    int fd=open(cale,O_RDONLY);
    if(fd==-1){
        printf("Eroare la deschidere!\n");
        return;
    }

    report r; //cutia in care bagam cate un raport pe rand
    int gasite=0; //numaram cate rapoarte avem

    //citim din fisier fix marimea unei cutii
    while(read(fd,&r,sizeof(report))==sizeof(report)){
        printf("ID: %d | Cat: %s | Grav: %d | Insp: %s\n",r.ID,r.category,r.severitylevel,r.inspectorname);
        printf("Descriere: %s\n",r.description);
        printf("GPS: %.4f, %.4f\n",r.latitude,r.longitude);
        printf("Data: %s\n",ctime(&r.timestamp));
        gasite++;
    }

    //daca gasite a ramas 0, inseamna ca fisierul era gol
    if(gasite==0){
        printf("E gol in districtul %s.\n",district);
    }
    close(fd);
}

void view(char*district,int id_cautat)
{
    char cale[150];
    sprintf(cale,"%s/reports.dat",district);

    //deschid fisierul doar sa citim
    int fd=open(cale,O_RDONLY);
    if(fd==-1){
        printf("Eroare la deschidere fisier %s!\n",cale);
        return;
    }
    report r;
    int gasit=0;

    while(read(fd,&r,sizeof(report))==sizeof(report)){
        // daca ID-ul cutiei e cel pe care il cautam
        if(r.ID==id_cautat){
            printf("Raport Gasit\n");
            printf("ID: %d | Cat: %s | Grav: %d | Insp: %s\n",r.ID,r.category,r.severitylevel,r.inspectorname);
            printf("Descriere: %s\n",r.description);
            printf("GPS: %.4f, %.4f\n",r.latitude,r.longitude);
            printf("Data: %s\n",ctime(&r.timestamp));
            gasit=1;
            break; //am gasit raportul, nu mai are rost sa citim restul fisierului
        }
    }

    if(gasit==0){
        printf("Nu am gasit raportul cu ID-ul %d in districtul %s.\n",id_cautat,district);
    }

    close(fd);
}

void sterge(char*district,int id_cautat)
{
    char cale[150];
    sprintf(cale,"%s/reports.dat",district);
    //deschidem fisierul pt citire si scriere (RDWR)
    int fd=open(cale,O_RDWR);
    if(fd==-1){
        printf("Eroare la deschidere fisier %s!\n",cale);
        return;
    }

    report r;
    int index_gasit=-1; //aici salvam pozitia (al catelea raport este)
    int pozitie_curenta=0;
    //cautam raportul sa vedem unde e
    while(read(fd,&r,sizeof(report))==sizeof(report)){
        if(r.ID==id_cautat){
            index_gasit=pozitie_curenta;
            break;
        }
        pozitie_curenta++;
    }

    if(index_gasit==-1){
        printf("Nu am gasit raportul %d de sters.\n",id_cautat);
        close(fd);
        return;
    }

    //mutam toate rapoartele de dupa el, cu o pozitie la stanga
    // calculam de la ce byte citim si la ce byte scriem
    off_t citeste_de_la=(index_gasit+1)*sizeof(report);
    off_t scrie_la=index_gasit*sizeof(report);

    while(1){
        //mergem la pozitia de citire si luam cutia
        lseek(fd,citeste_de_la,SEEK_SET);
        if(read(fd,&r,sizeof(report))!=sizeof(report)){
            break; //am ajuns la capatul fisierului, ne oprim
        }
        //mergem un pas mai in spate si o scriem, acoperind gaura
        lseek(fd,scrie_la,SEEK_SET);
        write(fd,&r,sizeof(report));

        //trec mai departe
        citeste_de_la+=sizeof(report);
        scrie_la+=sizeof(report);
    }

    //taiem ultima cutie (care a ramas duplicat la final dupa mutare)
    struct stat date_fisier;
    fstat(fd,&date_fisier); //fstat face acelasi lucru ca stat, dar ii dai direct fd-ul
    ftruncate(fd,date_fisier.st_size-sizeof(report)); // Taiem fisierul

    printf("Raportul %d a fost sters cu succes!\n",id_cautat);
    close(fd);
}

void creeaza_link(char* district) {
    char tinta[150];
    char nume_link[100];
    sprintf(tinta, "%s/reports.dat", district);
    sprintf(nume_link, "active_reports-%s", district);

    //stergem link-ul vechi daca exista
    unlink(nume_link);
    //cream link-ul nou
    if(symlink(tinta, nume_link) == 0) {
        printf("Link creat: %s -> %s\n", nume_link, tinta);
    }
}
void gestioneaza_link(char* district) {
    char tinta[150];
    char nume_link[100];

    //tinta este fisierul din folder: ex: downtown/reports.dat
    sprintf(tinta, "%s/reports.dat", district);
    // Numele link-ului va fi: active_reports-downtown
    sprintf(nume_link, "active_reports-%s", district);

    //stergem link-ul vechi daca exista deja (unlink)
    unlink(nume_link);

    //cream link-ul simbolic (symlink)
    if (symlink(tinta, nume_link) == 0) {
        printf("Link simbolic creat: %s -> %s\n", nume_link, tinta);
    }
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
             setaredistrict(district);
             gestioneaza_link(district);
            add(district, user);
            scrieinjurnal(district,action,role,user);
            i++;
        }
        else if (strcmp(argv[i],"--list")==0 && i+1<argc) {
            strcpy(action,"list");
            strcpy(district,argv[i+1]);
            gestioneaza_link(district);
            list(district);
            scrieinjurnal(district,action,role,user);
            i++;
        }
        else if(strcmp(argv[i],"--view")==0 && i+2<argc){
            strcpy(action,"view");
            strcpy(district,argv[i+1]);
            int idcautat=atoi(argv[i+2]); // transformam ID-ul din text in numar
            view(district,idcautat);
            scrieinjurnal(district,action,role,user);
            i+=2; //aici sarim cu 2 pentru ca am citit atat districtul, cat si ID-ul
        }
        else if(strcmp(argv[i],"--remove_report")==0 && i+2<argc){
            strcpy(action,"--remove_report");
            strcpy(district,argv[i+1]);
            int idcautat=atoi(argv[i+2]);
            //doar managerul are voie
            if(strcmp(role,"manager")!=0){
                printf("Eroare:Doar managerii pot sterge!\n");
            } else {
                sterge(district,idcautat);
                scrieinjurnal(district,action,role,user);
            }
            i+=2;
        }

    }



    return 0;
}
