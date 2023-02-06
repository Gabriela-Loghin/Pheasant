#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

/* portul folosit */
#define PORT 2908

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData
{
    int idThread; // id-ul thread-ului tinut in evidenta de acest program
    int cl;       // descriptorul intors de accept
} thData;

static void *treat(void *);
void raspunde(void *);
void robot(void *);

sem_t semafor;

int numar_jucatori = -1;
int numar_initial_jucatori;
int numar_threaduri;

int main()
{
    struct sockaddr_in server; // structura folosita de server
    struct sockaddr_in from;

    int sd; // descriptorul de socket
    int pid;

    pthread_t th[100]; // Identificatorii thread-urilor care se vor crea
    int i = 1;

    /* crearea unui socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }

    /* utilizarea optiunii SO_REUSEADDR */
    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    /* pregatirea structurilor de date */
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    /* umplem structura folosita de server */

    /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;

    /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    /* utilizam un port utilizator */
    server.sin_port = htons(PORT);

    /* atasam socketul */
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    /* punem serverul sa asculte daca vin clienti sa se conecteze */
    if (listen(sd, 2) == -1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    time_t t;
    srand((unsigned)time(&t));

    /* servim in mod concurent clientii...folosind thread-uri */
    while (1)
    {
        int client;
        thData *td; // parametru functia executata de thread
        int length = sizeof(from);

        sem_init(&semafor, 0, 1);

        printf("[server]Asteptam la portul %d...\n", PORT);
        fflush(stdout);

        /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[server]Eroare la accept().\n");
            continue;
        }

        /* s-a realizat conexiunea, se astepta bufferul */
        td = (struct thData *)malloc(sizeof(struct thData));
        td->idThread = i++;
        td->cl = client;

        pthread_create(&th[i], NULL, &treat, td);
    }

    sem_destroy(&semafor);
};

static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);

    numar_threaduri++;

    char buffer[100];
    strcpy(buffer, "");

    int bucla = 1;

    while (numar_jucatori != numar_threaduri)
    {
        if (tdL.idThread == 1 && bucla > 0)
        {
            char myVar[10];

            FILE *file = fopen("pheasant.txt", "r");
            fscanf(file, "%s", myVar);

            numar_jucatori = atoi(myVar);
            numar_initial_jucatori = numar_jucatori;

            fclose(file);

            sprintf(buffer, "Numarul de jucatori este: %d", numar_jucatori);
            write(tdL.cl, buffer, strlen(buffer));

            printf("%s\n", buffer);
            fflush(stdout);

            bucla = -1;
        }
        else if (bucla > 0)
        {
            strcpy(buffer, "Asteptam ceilalti jucatori sa se conecteze");
            write(tdL.cl, buffer, strlen(buffer));

            bucla = -1;
        }
    }

    pthread_detach(pthread_self());

    if (tdL.idThread <= numar_jucatori)
    {
        raspunde((struct thData *)arg);
    }

    /* am terminat cu acest client, inchidem conexiunea */
    close((intptr_t)arg);

    return (NULL);
};

char litera[1];
char cuvant[30];
char sufix[2];

int jucator = 1;

void genereazaSufix()
{
    char copie[30];

    strcpy(copie, cuvant);

    int lungime = strlen(copie);
    lungime = lungime - 2;

    strcpy(copie, copie + lungime);

    strcpy(sufix, copie);
}

int verificaSufix()
{
    if (strcmp(sufix, "nt") == 0 || strcmp(sufix, "rt") == 0 || strcmp(sufix, "nc") == 0)
        return -2;

    char prefix[2] = "";

    for (int i = 0; i < 2; i++)
        prefix[i] = cuvant[i];

    if (strcmp(sufix, prefix) == 0)
        return 1;

    return 0;
}

int verificaCuvant()
{
    FILE *file = fopen("dictionar.txt", "r");

    char dictionar[2000];
    fscanf(file, "%s", dictionar);

    char *pointer = strtok(dictionar, ",");
    while (pointer)
    {
        if (strcmp(cuvant, pointer) == 0)
        {
            fclose(file);
            return 1;
        }

        pointer = strtok(NULL, ",\n");
    }

    return -1;

    fclose(file);
}

int jucatorAnterior;
int inceput = 1;
int jucatorEliminat;

int seJoaca[10];

int STOP = 1;

void raspunde(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);

    char buffer[500];

    fflush(stdout);

    if (tdL.idThread != 1)
    {
        sprintf(buffer, "Sunteti jucatorul numarul: %d", tdL.idThread);
        write(tdL.cl, buffer, 30);

        strcpy(buffer, "");
    }

    printf("[Thread %d] Sunt aici.\n", tdL.idThread);

    while (STOP)
    {
        if (inceput == 1 && jucator == tdL.idThread)
        {
            if (numar_jucatori == 1)
            {
                strcpy(buffer, "Felicitari!");
                write(tdL.cl, buffer, 12);

                break;
            }

            sprintf(buffer, "Sunteti primul jucator");
            write(tdL.cl, buffer, 23);

            strcpy(buffer, "");

            if (read(tdL.cl, litera, sizeof(litera)) <= 0)
            {
                perror("[Client] Eroare la read().\n");
            }

            printf("[Thread %d] Litera este: %s\n", tdL.idThread, litera);

            inceput = 0;

            cuvant[0] = '\0';

            int bucla3 = 0;

            while (bucla3 <= 0)
            {
                if (read(tdL.cl, cuvant, sizeof(cuvant)) <= 0)
                {
                    perror("[Client] Eroare la read().\n");
                }

                printf("[Thread %d] Cuvantul citit a fost: %s\n", tdL.idThread, cuvant);
                fflush(stdout);

                bucla3 = verificaCuvant();

                write(tdL.cl, &bucla3, sizeof(int));
            }

            sufix[0] = '\0';

            genereazaSufix();

            printf("Sufix este: %s\n", sufix);

            if (numar_jucatori > 1)
            {
                jucator++;

                if (jucator > numar_initial_jucatori)
                    jucator = 1;

                while (seJoaca[jucator] == -1)
                {
                    jucator++;

                    if (jucator > numar_initial_jucatori)
                        jucator = 1;
                }
            }

            printf("[Thread %d] Urmatorul jucator este: %d\n", tdL.idThread, jucator);

            sprintf(buffer, "Este randul jucatorului %d.\nCuvantul lui incepe cu %s", jucator, sufix);
            printf("%s\n", buffer);

            write(tdL.cl, buffer, strlen(buffer));

            sem_wait(&semafor);
        }
        else if (jucator == tdL.idThread)
        {

            if (numar_jucatori == 1)
            {
                strcpy(buffer, "Felicitari!");
                write(tdL.cl, buffer, 12);

                break;
            }

            write(tdL.cl, "Acum este randul tau", 21);

            printf("[Thread %d] Cuvantul tau incepe cu: %s\n", tdL.idThread, sufix);
            write(tdL.cl, sufix, strlen(sufix));

            fflush(stdout);

            int bucla3 = 0;

            while (bucla3 == 0)
            {
                if (read(tdL.cl, cuvant, sizeof(cuvant)) <= 0)
                {
                    perror("[Client] Eroare la read().\n");
                }

                printf("[Thread %d] Cuvantul citit a fost: %s\n", tdL.idThread, cuvant);
                fflush(stdout);

                bucla3 = verificaSufix();

                if (bucla3 == 0)
                {
                    write(tdL.cl, &bucla3, sizeof(int));
                }
                else
                {
                    int nr1 = verificaCuvant();

                    char copieSufix[2];
                    strcpy(copieSufix, sufix);

                    genereazaSufix();

                    int nr2 = verificaSufix();
                    printf("nr2 este %d\n", nr2);

                    if (nr2 == -2 && nr1 == 1)
                    {
                        write(tdL.cl, &nr2, sizeof(int));

                        sprintf(buffer, "Jucatorul %d a castigat runda. Cuvantul a fost %s", tdL.idThread, cuvant);
                        printf("[Thread %d] %s\n", tdL.idThread, buffer);

                        jucatorAnterior = jucator;

                        jucator++;

                        if (jucator > numar_initial_jucatori)
                            jucator = 1;

                        while (seJoaca[jucator] == -1)
                        {
                            jucator++;

                            if (jucator > numar_initial_jucatori)
                                jucator = 1;

                            if (numar_jucatori == 1)
                                break;
                        }

                        jucatorEliminat = jucator;

                        numar_jucatori = numar_jucatori - 1;
                        seJoaca[jucator] = -1;

                        sprintf(buffer, "Jucatorul %d a fost eliminat.", jucator);
                        printf("[Thread %d] %s\n", tdL.idThread, buffer);

                        jucator = jucatorAnterior;

                        inceput = 1;
                    }
                    else if (nr2 == -2 && nr1 == -1)
                    {
                        numar_jucatori = numar_jucatori - 1;
                        seJoaca[tdL.idThread] = -1;

                        jucator++;

                        if (jucator > numar_initial_jucatori)
                            jucator = 1;

                        while (seJoaca[jucator] == -1)
                        {
                            jucator++;

                            if (jucator > numar_initial_jucatori)
                                jucator = 1;

                            if (numar_jucatori == 1)
                            {
                                break;
                            }
                        }

                        inceput = 1;

                        printf("[Thread %d] Urmatorul jucator este: %d\n", tdL.idThread, jucator);

                        write(tdL.cl, &nr1, sizeof(int));
                    }
                    else if (nr1 == -1)
                    {
                        write(tdL.cl, &nr1, sizeof(int));
                        strcpy(sufix, copieSufix);

                        numar_jucatori = numar_jucatori - 1;
                        seJoaca[tdL.idThread] = -1;

                        jucator++;

                        if (jucator > numar_initial_jucatori)
                            jucator = 1;

                        while (seJoaca[jucator] == -1)
                        {
                            jucator++;

                            if (jucator > numar_initial_jucatori)
                                jucator = 1;

                            if (numar_jucatori == 1)
                            {
                                break;
                            }
                        }

                        inceput = 1;
                    }
                    else
                    {
                        write(tdL.cl, &nr1, sizeof(int));

                        jucator++;

                        if (jucator > numar_initial_jucatori)
                            jucator = 1;

                        while (seJoaca[jucator] == -1)
                        {
                            jucator++;

                            if (jucator > numar_initial_jucatori)
                                jucator = 1;
                        }

                        sprintf(buffer, "Este randul jucatorului %d.\nCuvantul lui incepe cu %s", jucator, sufix);
                        write(tdL.cl, buffer, strlen(buffer));
                    }
                }

                printf("[Thread %d] Urmatorul jucator este: %d\n", tdL.idThread, jucator);

                sprintf(buffer, "Este randul jucatorului %d.\nCuvantul lui incepe cu %s", jucator, sufix);
                write(tdL.cl, buffer, strlen(buffer));

                sem_wait(&semafor);
            }
        }
        else
        {
            if (tdL.idThread == jucatorEliminat)
            {
                write(tdL.cl, "Ai pierdut!", 12);

                sprintf(buffer, "Jucatorul %d a castigat runda. Cuvantul a fost %s", jucator, cuvant);
                write(tdL.cl, buffer, strlen(buffer));

                if (numar_jucatori == 1)
                    STOP = 0;

                break;
            }

            for (int i = 1; i < numar_jucatori; i++)
            {
                if (i != tdL.idThread)
                    sem_post(&semafor);
            }
        }
    }
}