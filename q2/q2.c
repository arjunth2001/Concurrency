#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
//gets a random number including lower and upper bounds
int getRandom(int lb, int ub)
{
    return (rand() % (ub - lb + 1)) + lb;
}
// returns success of vaccine on a student given probabilty
int isSuccess(double p){
    double rndDouble = (double)rand() / RAND_MAX;
    return rndDouble > p;
}

//return min of 3 nos
int minna(int num1, int num2 , int num3){
    if(num1 < num2 && num1 < num3)
	{
		return num1;
	}
	else if(num2 < num3)
	{
		return num2;
	}
	else
	{
		return num3;
	}
}
// the inputs
int n,m,o;


//num_students_left in simulation
int num_students_left;
//num of waiting students
int waitingStudents;
//mutex_lock_for global variables
pthread_mutex_t global_lock;

struct Pcompany
{
    int id;//id of company
    double x;//probabilty of success
    int p;//no. of vaccines in each batch
    int r;//no. of batches left
    pthread_mutex_t company_lock;//lock
    int deliverable;
};

struct Pcompany *companies;

struct VZone
{
    int id; //id of zone
    int batchleft; //no. of vaccinizes left
    int numslots;// no. of slots left
    double currp;// the curr probabilty
    int isVaccinating;// 1 if vaccinating otherwise zero;
    int vaccinatedslots;//keeps count of vaccinated slots
    pthread_mutex_t vaccination_lock; //the lock
    
};

struct VZone *zones;

struct Student
{
    int id; //stores id of student
    int status; //stores status of student
    int round; //stores the number of rounds the student participated in

};
struct Student *students;

//the company thread
void *pcompany(void *companyargs)
{
    int id = ((struct Pcompany *)companyargs)->id; //id of company
    int r,p;
    int flag;
    while (num_students_left > 0) //continue the process until all students are processed
    {
        
        r=getRandom(1,5); // get the number of batches to prepare
        p=getRandom(10,20); // get the number of vaccines
        printf("\n\033[1;34mPharmaceutical Company %d is preparing %d batches of vaccines which has success probability %lf\n\033[0m",id,r,companies[id].x);
        sleep(getRandom(2, 5)); //sleep for time to simulate manufacturing
        printf("\n\033[1;34mPharmaceutical Company %d has prepared %d batches of vaccines which has success probability %lf. Waiting for all vaccines to be used to resume production\n\033[0m",id,r,companies[id].x);
        pthread_mutex_lock(&companies[id].company_lock);//lock the company for concurrency 
        companies[id].p=p; //set the new batch 
        companies[id].r=r; 
        companies[id].deliverable=r;
        pthread_mutex_unlock(&companies[id].company_lock);//unlock
        // now busy wait for all batches to get over
        while(companies[id].deliverable){ //deliverable is the variable that keeps count of batches that are in still in use. when there are no batches left continue production
                if(num_students_left==0){
                    return NULL;
                }
        }
        printf("\n\033[1;34mAll the vaccines produced by Pharmaceutical Company %d are emptied. Resuming Production now\n\033[0m",id);
        //resume

    }
    return NULL;
}

void *vZone(void * zoneargs)
{
    int id = ((struct VZone *)zoneargs)->id; //id of vzone
    int i=0;
    int k;//slots in iteration
    int mycompany;//the company from where we got the vaccines from
    while (num_students_left > 0)  //run the thread till number of students is greater than zero
    {
        pthread_mutex_lock(&zones[id].vaccination_lock); //lock zone for concurrency
        zones[id].isVaccinating=0; // the zone is not vaccinating currently
        pthread_mutex_unlock(&zones[id].vaccination_lock);//unlock
        mycompany=-1; // the company that gave it vaccines, now no one
        while(num_students_left){ //while there are some students left
            pthread_mutex_lock(&companies[i].company_lock);//lock so that only one one zone gets access to a company
            if(companies[i].r>=1){ //if this company has a batch take it
                companies[i].r--; //we took a batch
                pthread_mutex_lock(&zones[id].vaccination_lock); //lock this zone for concurrency
                zones[id].batchleft=companies[i].p; //update this zone
                zones[id].currp=companies[i].x;
                mycompany=companies[i].id;
                pthread_mutex_unlock(&zones[id].vaccination_lock); //unlock zone, company
                pthread_mutex_unlock(&companies[i].company_lock);
                printf("\n\033[1;34mPharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probabiltity %lf\n\033[0m",mycompany,id,companies[i].x);
                sleep(1);//simulate time
                printf("\n\033[1;33mPharmaceutical Company %d has delivered vaccines to Vaccination Zone %d, resuming vaccination now\n\033[0m",mycompany,id);
                break; //we got a batch now need not search for more companies, so break and wait for slots to fill.
                
            }
            pthread_mutex_unlock(&companies[i].company_lock);
            i = (i + 1) % n;
        }
        int waiters;//the no of waiters
        slot_declaration:
        while((waiters=waitingStudents)<=0){ //wait till we have some waiting students to move onto slot declaration
            //here we busy wait for some students to arrive
            if(num_students_left==0){
                return NULL;
            }
        }
        k = (getRandom(1,minna(8,zones[id].batchleft,waiters))); // get slots that are to be allocated
        
        pthread_mutex_lock(&zones[id].vaccination_lock); // lock this zone we are going to update, concurrency matters
        zones[id].numslots=k; //update
        printf("\n\033[1;33mVaccination Zone %d is ready to vaccinate with %d slots\n\033[0m",id,k);
        zones[id].isVaccinating=0;
        printf("\n\033[1;33mVaccination Zone %d is not in Vaccination Phase currently\n\033[0m",id);
        pthread_mutex_unlock(&zones[id].vaccination_lock);//unlock, before busy waiting
        while(zones[id].numslots>0 && waitingStudents>0){
            //busy wait for maximum slots to get filled , if there are no waitingstudents then enter vaccination phase
            if(num_students_left==0){
                return NULL;
            }
        }
        pthread_mutex_lock(&zones[id].vaccination_lock);//lock for concurrency
        zones[id].vaccinatedslots=k-zones[id].numslots; //vaccinatedslots has no of students who registered for vaccination
        printf("\n\033[1;33mVaccination Zone %d entering Vaccination Phase\n\033[0m",id);
        zones[id].isVaccinating=1; //now entering vaccination phase
        pthread_mutex_unlock(&zones[id].vaccination_lock);//unlock before waiting
        while(zones[id].vaccinatedslots){
            //wait for all of em to get vaccinated
            if(num_students_left==0){
                return NULL;
            }
        }
        pthread_mutex_lock(&zones[id].vaccination_lock);//for concurrency
        if(zones[id].batchleft>0){ // there are some more vaccines left in this batch, going to allocate more slots
            pthread_mutex_unlock(&zones[id].vaccination_lock);//unlock to avoid deadlocks
            goto slot_declaration;// go back to declaring more slots      
        }
        zones[id].isVaccinating=0;// now it is not vaccinating
        zones[id].numslots=0;//set slots back to zero
        printf("\n\033[1;33mVaccination zone %d has run out of Vaccines\n\033[0m",id);
        pthread_mutex_unlock(&zones[id].vaccination_lock);
        pthread_mutex_lock(&companies[mycompany].company_lock);
        companies[mycompany].deliverable--;// reduce the batch of the company from which we took vaccines from
        pthread_mutex_unlock(&companies[mycompany].company_lock);
        
    }

    return NULL;
}


//the student gets vaccinated here
void waitforVZone(int id){
    int i=0;
    pthread_mutex_lock(&global_lock);//lock
    printf("\n\033[0;36mStudent %d is waiting to be allocated a slot on a vaccination zone\n\033[0m", id);
    waitingStudents++; //A new student is waiting so add him
    pthread_mutex_unlock(&global_lock);//unlock
    
    while (1)
    {
        pthread_mutex_lock(&zones[i].vaccination_lock); // lock so number of slots do not get accessed by many students at the same time.
        if (zones[i].numslots >= 1 && !zones[i].isVaccinating) // allow a student to get a slot when the zone is not in vaccination phase and there are slots left
        {   pthread_mutex_lock(&global_lock);
            waitingStudents--;// A student is assigned a slot, so reduce waiting students
            pthread_mutex_unlock(&global_lock);
            zones[i].numslots--;//This student took a slot
            printf("\n\033[1;36mStudent %d has been assigned a slot on the vaccination zone %d. Waiting to be vaccinated\n\033[0m", id, i);
            pthread_mutex_unlock(&zones[i].vaccination_lock);//unlocks the lock so that multiple students can get vaccinated at the same time at the same vaccination zone
            
            while(!zones[i].isVaccinating){
                //busy wait to start vaccination
            }
            sleep(1); //simulate time to vaccinate
            printf("\n\033[1;36mStudent %d on Vaccination Zone %d has been Vaccinated which has success Probabilty %lf\n\033[0m", id, i,zones[i].currp);
            pthread_mutex_lock(&zones[i].vaccination_lock);//lock zone for concurrency
            zones[i].batchleft--;//This student will get vaccinated here so reduce size to simulate simultaneous vaccination on signal.
            zones[i].vaccinatedslots--;//reduce vaccinatedslots
            pthread_mutex_unlock(&zones[i].vaccination_lock);//unlock
            int status= isSuccess(zones[i].currp);
            if(!status){
                sleep(1); //simulate time for testing
                printf("\n\033[1;32mStudent %d  has been tested positive for antibodies\n\033[0m",id);
                students[id].status=1; // he is free to go
                return;
            }
            else{
                sleep(1); //simulate time for testing
                printf("\n\033[0;31mStudent %d  has been tested negative for antibodies\n\033[0m",id);
                students[id].status=0; //he has to start all again
                students[id].round++;//increment rounds
                return;

            }
        }
        pthread_mutex_unlock(&zones[i].vaccination_lock);// unclock if the if statement didn't get execute
        i = (i + 1) % m; // go through all zones again
    }

}
// Student Thread
void *student(void *studentargs)
{
    sleep(getRandom(1, 5)); //Sleep for a random time between 1-5
    int id = ((struct Student *)studentargs)->id;
    while(students[id].status!=1&&students[id].round<=3){  //while the student has not participated in three rounds and he has not been tested antibody positive participate in vaccination
        printf("\n\033[0;36mStudent %d has arrived for %d round of Vaccination\n\033[0m",id,students[id].round);
        waitforVZone(id); //wait for zone allocation
    }
    if(students[id].status!=1){ //if he exited the loop after getting rounds greater than 3 sedlyf for him.
        printf("\n\033[0;36mStudent %d is leaving for home. The college gave up on him. Sedlyf\n\033[0m", id);
    }
    pthread_mutex_lock(&global_lock);//concurrency
    num_students_left--;//he left reduce number of students left
    pthread_mutex_unlock(&global_lock);
    return NULL;
}


int main(){
    //inputs
    printf("\nInput the number of pharmaceutical companies, the number of vaccination zones, the number of students\n");
    scanf("%d %d %d", &n, &m, &o);
    if(m<=0 || n<=0 || o < 0){
        printf("\nInvalid Inputs: Try again.\n");
        return 0;
    }
    //initialisations
    companies = malloc(n* sizeof(struct Pcompany));
    zones = malloc(m * sizeof(struct VZone));
    students = malloc(o * sizeof(struct Student));

    num_students_left = o;
    waitingStudents=0;
    pthread_mutex_init(&global_lock, NULL);

    pthread_t companythreads[n];
    double p[n+1];
    for (int i = 0; i < n; i++)
    {
        scanf("%lf",&p[i]);
        if(p[i]<0 || p[i]>1){
            printf("\nInvalid probability: Try again.\n");
            return 0;
        }
    }
    
    printf("\n\033[1;35mBeginning simulation\n\033[0m");
    for (int i = 0; i < n; i++)
    {
        pthread_mutex_init(&companies[i].company_lock, NULL);
    }

    for (int i = 0; i < n; i++)
    {
        
        companies[i].id = i;
        companies[i].x=p[i];
        companies[i].p=0;
        companies[i].r=0;
        pthread_create(&companythreads[i], NULL, pcompany, &companies[i]);
    }
    

    pthread_t vaccinationthreads[m];
    for (int i = 0; i < m; i++)
    {

        pthread_mutex_init(&zones[i].vaccination_lock, NULL);

    }

    for (int i = 0; i < m; i++)
    {
        zones[i].id = i;
        zones[i].batchleft=0;
        zones[i].currp=0;
        zones[i].isVaccinating=0;
        zones[i].numslots=0;
        pthread_create(&vaccinationthreads[i], NULL,vZone, &zones[i]);

    }


    pthread_t student_threads[o];

    for (int i = 0; i < o; i++)
    {
        students[i].id = i;
        students[i].status=0;
        students[i].round=1;
        pthread_create(&student_threads[i], NULL, student, &students[i]);
    }


    for (int i = 0; i < m; i++)
    {
        pthread_join(vaccinationthreads[i], NULL);
    }

    for (int i = 0; i < n; i++)
    {
        pthread_join(companythreads[i], NULL);
    }

    for (int i = 0; i < o; i++)
    {
        pthread_join(student_threads[i], NULL);
    }
    for (int i = 0; i < m; i++)
    {
        pthread_mutex_destroy(&zones[i].vaccination_lock);
    }

    for (int i = 0; i < n; i++)
    {
        pthread_mutex_destroy(&companies[i].company_lock);
    }

    pthread_mutex_destroy(&global_lock);

    printf("\n\033[1;35mSimulation ended\n\033[0m");
    return 0;
}