#define _POSIX_C_SOURCE 199309L //required for clock
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>
#define int long long int
#define threshhold 5  //change threshold - according to question 5

//argument to threads
struct arg{
     int l; //left index
     int r; //right index
     int* arr;//array to sort
};
//gets shared memory
int * shareMem(size_t size){
     key_t mem_key = IPC_PRIVATE; //It is the numeric key to be assigned to the returned shared memory segment
     int shm_id = shmget(mem_key, size, IPC_CREAT | 0666); //creating and granting read and write access
     return (int*)shmat(shm_id, NULL, 0); //shmat() returns the address of the attached shared memory, NULL, the system chooses a suitable (unused) page- aligned address to attach the segment.
     //0 is read-only
}

//normal Selection sort
void selectionSort( int A[], int l, int r ){
    int min,temp;
    int len=r-l+1;
	for(int i=0;i<len;i++)
	{
	    min=l+i;
		for(int j=i+1;j<len;j++)
		{
			if(A[l+j]<A[min])
			{
					min=l+j;
			}
		}
		temp=A[min];
		A[min]=A[l+i];
		A[l+i]=temp;
	}
}
//normal merge sort merge
void merge(int A[],int l, int mid, int r){
    int n1= mid-l+1;
    int n2=r-mid;
    int L[n1];
    int R[n2];
    for(int i=0; i< n1;i++){
        L[i]=A[l+i];

    }
    for(int i=0; i< n2;i++){
        R[i]=A[mid+1+i];

    }
    int i=0,j=0,k=l;
    while (i < n1 && j < n2) 
    { 
        if (L[i] <= R[j])  
        { 
            A[k] = L[i]; 
            i++; 
        } 
        else 
        { 
            A[k] = R[j]; 
            j++; 
        } 
        k++; 
    } 
  
    while (i < n1)  
    { 
        A[k] = L[i]; 
        i++; 
        k++; 
    } 
    while (j < n2) 
    { 
        A[k] = R[j]; 
        j++; 
        k++; 
    } 
}

//the normal mergesort function - with selection sort when array size becomes less than or equal to threshold
void normalMergeSort(int A[], int l, int r){
    if(r-l > threshhold){
        int mid= (r+l)/2;
        normalMergeSort(A,l,mid);
        normalMergeSort(A,mid+1,r);
        merge(A,l,mid,r);
    }
    else{
        selectionSort(A,l,r);
    }
}

// threaded - merge sort
void *threadedMergeSort(void* a){

    struct arg *args = (struct arg*) a; //arguments passed as structure
    int l = args->l; //left index
    int r = args->r; //right index
    int *A = args->arr; //array to sort
    if(r-l > threshhold){ // if array size greater than threshold
        int mid= (r+l)/2;
        struct arg leftargs, rightargs; //args to right thread and left thread
        pthread_t ltid,rtid; //left thread, right thread
        leftargs.arr=A;
        leftargs.l=l;
        leftargs.r=mid;
        rightargs.l=(mid+1);
        rightargs.arr=A;
        rightargs.r=r;
        pthread_create(&ltid,NULL,threadedMergeSort,&leftargs); //sort left side in left thread
        pthread_create(&rtid,NULL,threadedMergeSort,&rightargs);//sort right side in right thread
        pthread_join(rtid,NULL);//join them, that is wait for them to complete
        pthread_join(ltid,NULL);
        merge(A,l,mid,r);
        pthread_exit(0); //exit this thread

        
    }
    else{
        selectionSort(A,l,r);
        pthread_exit(0);//exit this thread
    }
}
void processMergeSort(int A[],int l, int r){

    if(r-l > threshhold){  //if array size greater than threshold do mergesort 
        int mid= (r+l)/2;
        
        
        merge(A,l,mid,r);
        pid_t left,right; //process id of left and right process
        left=fork(); //fork a left child
        if(left==-1){  //error handling

            perror("Fork:");
            _exit(-1);

        }
        else if(left==0){ 
            //this is left child, this process will sort left side
            processMergeSort(A,l,mid);
            _exit(0); //exit after processing
        }
        else{
            //this is parent,
            right=fork(); // make a right child
            if(right==-1){
                perror("Fork right:");
                _exit(-1);

            }
            else if(right==0){
                //this is right child, process right side
                processMergeSort(A,mid+1,r);
                _exit(0); //exit after doing it
            }
        }
        //this is parent, wait for both of them to complete...
        signed status;
        waitpid(left,&status,0);
        waitpid(right,&status,0);
        //merge array after both of them complete
        merge(A,l,mid,r);
    }
    else{ //else do selection sort
        
        selectionSort(A,l,r);
        
    }

}

void runSort(int n){

    struct timespec ts; //for timing 
    int *normalSortArray = shareMem(sizeof(int)*(n+1)); //array to be used for normal sort
    int *multiSortArray = shareMem(sizeof(int)*(n+1)); //array to be used for merge sort by processes
    int *threadSortArray = shareMem(sizeof(int)*(n+1)); // array to be used for threaded merge sort
    //inputs
    for(int i=0;i<n;i++){
        
        scanf("%lld", &normalSortArray[i]);
    }
    //copying
    for(int i=0;i<n;i++){
        
        multiSortArray[i]=normalSortArray[i];
        threadSortArray[i]=normalSortArray[i];

    }
    printf("Running Concurrent MergeSort for n = %lld\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double st = ts.tv_nsec/(1e9)+ts.tv_sec; //initialising start time
    processMergeSort(multiSortArray,0,n-1); //do process merge
    for(int i=0; i<n; i++){
        printf("%lld ",multiSortArray[i]);
    }
    printf("\n");
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    long double en = ts.tv_nsec/(1e9)+ts.tv_sec;// get end time
    printf("time = %Lf\n", en - st); //endtime - start time : time taken
    long double t1 = en-st; //store for comparison

    pthread_t tid; //create a thread
    struct arg a; //args to thread
    a.l = 0;
    a.r = n-1;
    a.arr = threadSortArray;
    printf("Running Threaded MergeSort for n = %lld\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts); 
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    pthread_create(&tid, NULL,threadedMergeSort, &a);
    pthread_join(tid, NULL);
    for(int i=0; i<n; i++){
        printf("%lld ",a.arr[i]);
    }
    printf("\n");
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    long double t2 = en-st; //store time as before

    printf("Running Normal MergeSort for n = %lld\n", n);
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

    normalMergeSort(normalSortArray,0,n-1);
    for(int i=0; i<n; i++){
        printf("%lld ",normalSortArray[i]);
    }
    printf("\n");
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("time = %Lf\n", en - st);
    long double t3 = en - st; //store time as bfore

    printf("Normal Merge Sort ran:\n\t[ %Lf ] times faster than Concurrent Merge Sort\n\t[ %Lf ] times faster than Threaded Merge Sort\n\n\n", t1/t3, t2/t3);
    shmdt(threadSortArray);
    shmdt(normalSortArray);
    shmdt(multiSortArray);
    return;

}

signed main(){
     
    int n;
    scanf("%lld", &n);
    runSort(n);
    return 0;

}