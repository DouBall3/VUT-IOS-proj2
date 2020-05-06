#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>

typedef struct {
    int entered, checked, building, judge;
} THall;


THall *hall;
int hallFD;

int counterFD;
int* counter;

int imcountFD;
int* imcount;

sem_t *mutex;
sem_t *count;
sem_t *noJudge;
sem_t *confirmed;
sem_t *allSignedIn;
sem_t *imc;
sem_t *isJudge;

void start(int ID)
{

    printf("%d: IMM %d : starts\n", *counter, ID);

}

void imen(int ID){
    printf("%d: IMM %d: enters: %d: %d: %d\n", *counter, ID, hall->entered, hall->checked, hall->building);
}

void imwc(int ID){
    printf("%d: IMM %d: wants certificate: %d: %d: %d\n", *counter, ID, hall->entered, hall->checked, hall->building);
}

void imgc(int ID){
    printf("%d: IMM %d: got certificate: %d: %d: %d\n", *counter, ID, hall->entered, hall->checked, hall->building);
}

void imle(int ID){
    printf("%d: IMM %d: leaves: %d: %d: %d\n", *counter, ID, hall->entered, hall->checked, hall->building);
}

void jven(){
    printf("%d: JUDGE: wants to enter\n", *counter);
}
void jent(){
    hall->judge = 1;
    printf("%d: JUDGE: enters: %d: %d: %d\n", *counter, hall->entered, hall->checked, hall->building);
}
void jwait(){
    printf("%d: JUDGE: waits for imm: %d: %d: %d\n",*counter, hall->entered, hall->checked, hall->building);
}
void jstart(){
    printf("%d: JUDGE: starts confirmation: %d: %d: %d\n",*counter, hall->entered, hall->checked, hall->building);
}
void jend(){
    printf("%d: JUDGE: ends confirmation: %d: %d: %d\n",*counter, hall->entered, hall->checked, hall->building);
}
void jleaves(){
    hall->judge = 0;
    printf("%d: JUDGE: leaves: %d: %d: %d\n",*counter, hall->entered, hall->checked, hall->building);
}
void jfinish(){
    printf("%d: JUDGE: finishes\n",*counter);
}
void check(int ID){
    printf("%d: IMM %d: checks: %d: %d: %d\n", *counter, ID, hall->entered, hall->checked, hall->building);
}

void createImigrants(int, int, int);
void createJudge(int, int);
static void imigrant(int,int);
static void judge(int, int);

int main(int argc, char* argv[]) {
    srand(time(NULL));
    int PI; //počet imigrantu co musím vyrobit
    int IG; //max doba čekání při výrobě imigranta
    int JG; //max doma čekání při výrobě soudce
    int IT; //max doma vyzvednuti certifikatu
    int JT; //max doma trvani rozhodnuti soudcem
    char *dump;
    if(argc == 6)
    {
        PI = strtoul(argv[1], &dump, 10);
        IG = strtoul(argv[2], &dump, 10);
        JG = strtoul(argv[3], &dump, 10);
        IT = strtoul(argv[4], &dump, 10);
        JT = strtoul(argv[5], &dump, 10);

        if(PI < 0)
        {
            fprintf(stderr, "%s", "Chybny argument PI\n");
            return 1;
        }

        if((IG < 0) || (IG > 2000))
        {
            fprintf(stderr, "%s", "Chybny argument IG\n");
            return 1;
        }

        if((JG < 0) || (JG > 2000))
        {
            fprintf(stderr, "%s", "Chybny argument JG\n");
            return 1;
        }

        if((IT < 0) || (IT > 2000))
        {
            fprintf(stderr, "%s", "Chybny argument IT\n");
            return 1;
        }

        if((JT < 0) || (JT > 2000))
        {
            fprintf(stderr, "%s", "Chybny argument JT\n");
            return 1;
        }
    }
    else
    {
        printf("Chybny pocet argumentu.\n");
        printf("Zadejte prosim ve formatu:\n");
        printf("./proj2 PI IG JG IT JT\n");
        return 1;
    }

    FILE *outFile = fopen("proj2.out", "w");
    if(outFile == NULL)
    {
        fprintf(stderr, "Problem s pristupem do souboru");
        return 1;
    }
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    hallFD = shm_open("/xdohna45_hall", O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(hallFD, sizeof(THall));
    hall = (THall*)mmap(NULL, sizeof(THall), PROT_READ | PROT_WRITE, MAP_SHARED, hallFD,0);
    close(hallFD);

    counterFD = shm_open("/xdohna45_counter", O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(counterFD, sizeof(int));
    counter = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, counterFD, 0);
    close(counterFD);

    imcountFD = shm_open("/xdohna45_imcount", O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(imcountFD, sizeof(int));
    imcount = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, imcountFD, 0);
    close(imcountFD);

    *imcount = PI;
    *counter = 0;


    hall->checked=0;
    hall->entered=0;
    hall->building=0;
    hall->judge=0;

    mutex = sem_open("/xdohna45_mutex", O_CREAT, 0666,1);
    noJudge = sem_open("xdohna45_noJudge", O_CREAT, 0666,1);
    confirmed = sem_open("/xdohna45_confirmed", O_CREAT, 0666,0);
    allSignedIn = sem_open("/xdohna45_allSignedIn", O_CREAT, 0666, 0);
    count = sem_open("/xdohna45_count", O_CREAT, 0666,1);
    imc = sem_open("/xdohna45_imc", O_CREAT, 0666, 1);
    pid_t parrentPid;
    pid_t pid = fork();

    if(pid == 0)
    {
        createImigrants(PI, IG, IT);
    }
    else
    {
        parrentPid = pid;
        createJudge(JG, JT);
    }
    waitpid(parrentPid, NULL, 0);
    sem_close(mutex);
    sem_close(noJudge);
    sem_close(confirmed);
    sem_close(allSignedIn);
    sem_close(count);
    sem_close(imc);

    sem_unlink("/xdohna45_mutex");
    sem_unlink("/xdohna45_noJudge");
    sem_unlink("/xdohna45_confirmed");
    sem_unlink("/xdohna45_allSignedIn");
    sem_unlink("/xdohna45_count");
    sem_unlink("/xdohna45_imc");

    shm_unlink("/xdohna45_hall");
    shm_unlink("/xdohna45_counter");
    shm_unlink("/xdohna45_imcount");

}

void createImigrants(int PI, int IG, int IT){
    pid_t pPid;
    for(int i = 0; i < PI; i++){
        usleep((random()%(IG+1))*1000);
        pid_t  pid = fork();
        if(pid == 0)
        {
            imigrant(i+1,IT);
            exit(0);
        }
        else
        {
            pPid = pid;
        }
    }
    waitpid(pPid, NULL, 0);
    exit(0);
}

void createJudge(int JG, int JT){
    pid_t jPid;
    pid_t pid = fork();
    if(pid == 0)
    {
        judge(JG, JT);
        exit(0);
    } else{
        jPid = pid;
    }
    waitpid(jPid, NULL, 0);
}

static void imigrant(int ID, int IT){
    sem_wait(count);
    (*counter)++;
    sem_wait(mutex);
    start(ID);
    sem_post(mutex);
    sem_post(count);

    sem_wait(noJudge);
    sem_wait(count);
    (*counter)++;
    sem_wait(mutex);
    hall->entered++;
    imen(ID);
    sem_post(mutex);
    sem_post(count);
    sem_post(noJudge);

    sem_wait(count);
    (*counter)++;
    sem_wait(mutex);
    hall->checked++;
    check(ID);
    sem_post(count);



   if(hall->judge == 1 && hall->entered == hall->checked) sem_post(allSignedIn);
    else sem_post(mutex);

    sem_wait(confirmed);

    sem_wait(count);
    (*counter)++;
    sem_wait(mutex);
    imwc(ID);
    sem_post(mutex);
    sem_post(count);

    usleep((rand()%(IT+1))*1000);

    sem_wait(count);
    (*counter)++;
    sem_wait(mutex);
    imgc(ID);
    sem_post(mutex);
    sem_post(count);

    sem_wait(noJudge);
    sem_wait(mutex);
    sem_wait(imc);
    (*imcount)--;
    sem_post(imc);
    hall->building--;
    sem_wait(count);
    (*counter)++;
    imle(ID);
    sem_post(count);
    sem_post(mutex);
    sem_post(noJudge);
    exit(0);
}

static void judge(int JG, int JT){
    while(1) {
        usleep((random() % (JG + 1)) * 1000);
        sem_wait(count);
        (*counter)++;
        sem_wait(mutex);
        jven();
        sem_post(mutex);
        sem_post(count);

        sem_wait(noJudge);

        sem_wait(mutex);

        sem_wait(count);
        (*counter)++;
        jent();
        sem_post(count);
        sem_post(mutex);
        

            if (hall->entered > hall->checked) {
                sem_wait(count);
                (*counter)++;
                jwait();
                sem_post(count);

                sem_post(mutex);
                sem_wait(allSignedIn);
            }

            sem_wait(count);
            (*counter)++;
            jstart();
            sem_post(count);

            usleep((rand() % (JT + 1)) * 1000);

            sem_wait(count);
            (*counter)++;
            hall->building += hall->checked;
            hall->entered = 0;
            hall->checked = 0;
            jend();
            for (int i = 0; i < hall->building; i++) sem_post(confirmed);
            sem_post(count);

        sem_post(mutex);

        sem_wait(mutex);
        sem_wait(count);
        (*counter)++;
        jleaves();
        sem_post(count);
        sem_post(mutex);
        sem_post(noJudge);

        sem_wait(mutex);
        sem_wait(imc);
        if ((*imcount) <= 0) {
            sem_wait(count);
            (*counter)++;
            sem_post(count);
            jfinish();

            exit(0);
        }
        sem_post(mutex);
        sem_post(imc);
    }

}