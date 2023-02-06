#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

int main(int argc, char *argv[])
{
    int sd;                    // descriptorul de socket
    struct sockaddr_in server; // structura folosita pentru conectare  // bufferul trimis
    char buf[10];

    /* exista toate argumentele in linia de comanda? */
    if (argc != 3)
    {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    /* stabilim portul */
    port = atoi(argv[2]);

    /* cream socketul */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Eroare la socket().\n");
        return errno;
    }

    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
    /* familia socket-ului */
    server.sin_family = AF_INET;
    /* adresa IP a serverului */
    server.sin_addr.s_addr = inet_addr(argv[1]);
    /* portul de conectare */
    server.sin_port = htons(port);

    /* ne conectam la server */
    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[Client] Eroare la connect().\n");
        return errno;
    }

    char buffer[100];
    bzero(buffer, 100);

    char cuvant[30];
    bzero(cuvant, 30);

    char sufix[2];
    bzero(sufix, 2);

    int rc;

    while (1)
    {
        if ((rc = read(sd, buffer, sizeof(buffer))) <= 0)
        {
            perror("[Client] Eroare la read().\n");
        }

        buffer[rc] = '\0';

        fflush(stdout);

        printf("%s\n", buffer);

        fflush(stdout);

        if (strcmp(buffer, "Sunteti primul jucator") == 0)
        {
            strcpy(buffer, "");

            printf("Alege o litera: \n");

            char litera[1] = "";

            if (read(0, litera, sizeof(int)) <= 0)
            {
                perror("[Client] Eroare la read().\n");
            }

            litera[1] = '\0';

            if (write(sd, litera, sizeof(litera)) <= 0)
            {
                perror("[Client] Eroare la write().\n");
            }

            printf("Citeste primul cuvant: \n");

            int continua = -1;

            while (continua < 0)
            {
                bzero(cuvant, 30);

                rc = read(0, cuvant, sizeof(cuvant));
                cuvant[rc] = '\0';

                int lungime = strlen(cuvant);
                cuvant[lungime - 1] = '\0';

                char copie[30];

                strcpy(copie, cuvant);

                lungime = strlen(copie);
                lungime = lungime - 2;

                strcpy(copie, copie + lungime);

                if (litera[0] != cuvant[0])
                {
                    printf("Cuvantul nu incepe cu litera %s. Citeste alt cuvant: \n", litera);
                }
                else if (strcmp(copie, "nd") == 0 || strcmp(copie, "nt") == 0 || strcmp(copie, "rt") == 0 || strcmp(copie, "rc") == 0 || strcmp(copie, "nc") == 0 || strcmp(copie, "pt") == 0 || strcmp(copie, "ct") == 0 || strcmp(copie, "ft") == 0)
                {
                    printf("Nu puteti inchide cu primul cuvant! Citeste alt cuvant: \n");
                }
                else
                {
                    if (write(sd, cuvant, sizeof(cuvant)) <= 0)
                    {
                        perror("[Client] Eroare la write().\n");
                    }

                    if (read(sd, &continua, sizeof(int)) <= 0)
                    {
                        perror("[Client] Eroare la read().\n");
                    }

                    if (continua == -1)
                        printf("Cuvantul nu se afla in dictionar. Citeste alt cuvant: \n");
                }
            }

            cuvant[0] = '\0';
        }
        else if (strcmp(buffer, "Acum este randul tau") == 0)
        {
            bzero(sufix, 2);

            if (read(sd, sufix, sizeof(sufix)) <= 0)
            {
                perror("[Client] Eroare la read().\n");
            }

            printf("Cuvantul tau incepe cu: %s\n", sufix);

            printf("Citeste cuvantul: \n");

            int continua = 0;

            while (continua == 0)
            {
                bzero(cuvant, 30);
                read(0, cuvant, sizeof(cuvant));

                int lungime = strlen(cuvant);
                cuvant[lungime - 1] = '\0';

                if (write(sd, cuvant, sizeof(cuvant)) <= 0)
                {
                    perror("[Client] Eroare la write().\n");
                }

                cuvant[0] = '\0';

                if (read(sd, &continua, sizeof(int)) <= 0)
                {
                    perror("[Client] Eroare la read().\n");
                }

                if (continua == 0)
                {
                    printf("Cuvantul nu incepe cu %s. Citeste alt cuvant: \n", sufix);
                }
                else if (continua == -1)
                {
                    printf("Cuvantul nu se afla in dictionar. Ai pierdut!\n");

                    close(sd);

                    return 0;
                }
                else if (continua == -2)
                {
                    printf("Ati castigat aceasta runda!\n");
                    fflush(stdout);

                    bzero(buffer, 100);
                }
            }

            sufix[0] = '\0';
            cuvant[0] = '\0';
            buffer[0] = '\0';
        }
        else if (strcmp(buffer, "Ai pierdut!") == 0)
        {
            if (read(sd, buffer, sizeof(buffer)) <= 0)
            {
                perror("[Client] Eroare la read().\n");
            }

            printf("%s\n", buffer);

            close(sd);

            return 0;
        }
        else if (strcmp(buffer, "Felicitari!") == 0)
        {
            printf("Ai castigat jocul!\n");

            close(sd);

            return 0;
        }

        bzero(buffer, 100);
    }

    /* inchidem conexiunea, am terminat */
    close(sd);
}
