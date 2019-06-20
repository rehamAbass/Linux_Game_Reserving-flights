//////////////////////////////////////////////////////////////////////////////////
//
//						DEAR MR.RON SIVAN
///////					HW number 4 :
//						By : Reham Abass [id]
//			
//
//			Date : 10.06.2019
//			Program: Flight Ticket Purchase System 	
//			Each Flight has number(id) and number of sets.
//			Requests file: Each request for(1-6)people,has flight ID. 
//
//			Filling request: 
//			1- Check empty sets on map (matrix which rellevant to the demanded flight).
//			2- Max seats by one request is: 6 .
//			Choosing sets:
//			1. has to take time: usleep()
//			2. if asked sete are empty > so regist for him and rewrite them unempty
//			3.else:not empty > do while choose sets
//			4. if there are no empty set in that flight so can't fill the request.
//			Threads are customers they are not depending on each other.
//			Two threads can't get the same set on the same flight
//	

#include "flights.h"		// which are defined from the flights.h :
/*
							#define FLIGHT_COUNT 20
							#define ROW_COUNT 40
							#define SEATS_PER_ROW 8
							#define MAX_SEATS 6
							*/
#include<stdio.h>							//for input Output
#include<stdlib.h>							//for allocations
#include<time.h> 							// for random sleep
#include<string.h>							//for strings
#include<pthread.h>							// each request gonna work by thread
#include<fcntl.h>							//for files
#include<unistd.h>
#include<semaphore.h>
#include<sys/types.h>
#define ID_LONG  4
#define SPACE_LONG 2	
#define FLIGHT_LONG 1
#define CHAIRS_LONG 1
#define ENTER_CHAR 1
#define BUF_SIZE ID_LONG+SPACE_LONG+FLIGHT_LONG+SPACE_LONG+CHAIRS_LONG+ENTER_CHAR //11



pthread_t *threads_array;									//array has the pointers to each thread

int array_flights[FLIGHT_COUNT][ROW_COUNT * SEATS_PER_ROW];  //global array of pointers to matrixes							

int empty_chairs_array[FLIGHT_COUNT];			//global counters for each fly-numbers of occupied chairs 

int succeded_requests_counter;								//global counter on succeded requestes
sem_t s;													//global semaphore

int number_of_requests;										//global variable gonna be iniated once and only one time.
char **Array_of_buffers;									//each thread on one request, which needs one different buffer to own
int *i_array;												//because of the threads needs, the local variable "i" may change so using the same pointer for all threads are bad , so I use array of indexes, i_array[count of the threads=number_of_requests]which cannt be ready in compiliation time , only in running time,can allocate it.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void allocate_array_of_threads(){
		//Using global pthread **threads_array;	
		//Using the global : number_of_requests	
	int i;	
	threads_array=(pthread_t*)malloc(number_of_requests * sizeof(pthread_t));
	if(threads_array ==NULL){printf("\nCan't allocate pointers to threads in threads array\n");exit(0);}
	//printf("\n Sucesseded to allocate array of threads \n");
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////2
void show_map(){
	int flight,i,j,index;
	for(flight=0 ; flight < FLIGHT_COUNT; flight++){
		
		printf("\n\nflight number %d looks:",flight);

		for(i=0 ; i< ROW_COUNT ; i++ ){
			printf("\n\t");
			for(j=0; j< SEATS_PER_ROW; j++){
				index= (i*SEATS_PER_ROW) + j;
				printf(" %4d,",array_flights[flight][index]);								
			}
			
		}//end flight
	}//end all flights
printf("\n\n");
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////3
void iniate_empty_chairs_array() {				//this function gonna be called just once. in the beginning of the program
	int j;  							//they all are empty for all the flies
	for (j = 0; j< FLIGHT_COUNT; j++) {
		empty_chairs_array[j] = (ROW_COUNT * SEATS_PER_ROW);			//iniate global array
	}
	//printf("\niniate empty chairs =%d\n",(ROW_COUNT * SEATS_PER_ROW));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////4
void iniate_array_flights() {	//no parameters, gonna iniate the global array
	int i, flight;
	for (flight = 0; flight < FLIGHT_COUNT; flight++) { //iniate
		for (i = 0; i < ROW_COUNT * SEATS_PER_ROW; i++)
			array_flights[flight][i] = 0;	
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////5
void* convert(int* id,int *flight,int* chairs,char* str){
	printf("\nstr is:%s",str);
	int x;
	char str2[4];

	strncpy(str2,str,4);
	x= atoi(str2); 
	*id= x;
	
	strncpy(str2,&str[6],2);
	x= atoi(str2);
	*flight= x;

	strncpy(str2,&str[9],2);
	x= atoi(str2); 
	*chairs= x;
	//printf("\nIn convert: id=%d,flight =%d,chairs= %d \n",*id,(*flight)+1,x);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////6

int number_of_lines_in_file(int file){			//I GONNA USE IT SO MUCH , IN LOOPS AND ALLOCATIONS.
	int lines=0;
	char c;
	while(read(file,&c,1) == 1 ){//if he succede to read one byte as giving	
		if(c =='\n') lines++;
	}
lines--;	//because it counts also the last line(by \n) which is empty
	printf("\n number of lines in file= %d\n",lines);
	return lines;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void allocate_buffers(){ 					//SO EACH THREAD GONNA HAS HIS SPECIAL BUFFER TO WORK WITH.
	int i;
	Array_of_buffers=(char**)malloc(number_of_requests*sizeof(char*));
	for(i=0 ; i< number_of_requests; i++){
		Array_of_buffers[i]=(char*)malloc(BUF_SIZE*sizeof(char));
		if(Array_of_buffers[i] == NULL){ printf("\n faild to allocate buffer\n");exit(-1);}
	}
	//printf("\nSuccess to allocate buffers\n");
}////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void free_buffers(){						//WE MUST FREE ALL THE POINTERS WE HAVE ALLOCATED 
	int i;
	for(i=0 ; i< number_of_requests; i++)
		free(Array_of_buffers[i]);
free(Array_of_buffers);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////7
void join_ALL_threads(){
	int i ;
	for(i=0; i< number_of_requests ;i++){
		pthread_join(threads_array[i],NULL);// threads_array has pointer to the thread
		}
}

////////////////////////////////////////////////////////////////
// it gonna return a pointer to the struct of the last request READY!(being converted as well)

							// THE FUNCTION OF THE THREAD 
void* occupy(void * i) {  

	sem_wait(&s); //in begining to lock from others, by reducing the semaphore to -1.
	int line= *(int*)i;	
	printf("\nsemaphore opened to thread(request)number: %d\n",line);
	printf("\n\nIN OCCUPY :\n");	
	int t=(rand()%31 + 10);					// 10 .. 40
	usleep(t*5000); 
	int flight,chairs,id ;

	convert(&id,&flight,&chairs,Array_of_buffers[line]);

	printf("\nTO OCCUPY :id=%d,flight =%d,chairs= %d \n",id,flight,chairs);
	int empty = empty_chairs_array[flight];
	int held= (ROW_COUNT * SEATS_PER_ROW)-empty;
	printf("\nThis flight there are %d helded chairs\n",held);
	
	if(empty ==0 || empty < chairs){ 
		printf("\nEmpty chairs =%d< required chairs(%d)-\t FAILD",empty,chairs);

		}else if(array_flights[flight][held] == 0 && index >0 ){
				succeded_requests_counter++;
				while(chairs > 0){
						empty_chairs_array[flight]--;
						chairs--;
						array_flights[flight][held+chairs] = id;
						
					}
				printf("\nsucceded to occupy! id %d flight %d \n",id,flight);
		}

sem_post(&s);  //release the semaphore , it got 1, in end to release the flight to other threads
printf("\nsemaphore for thread(request)number %d closed\n",line);		
}
//////////////////////////////////////////////////////////
int main(int argc, char *argv[]) 				// argv[1] GONNA BE THE REQUESTS.TXT file
{
	struct timeval begining_time, end_time;
	gettimeofday(&begining_time, NULL);	// what is the time in the begining ?
	double elapsedTime;
	int flight,i,j,k,count,index,res_open,res_read,res_thread,requests_counter = 0;
	int *temp;
	char * requests_file = argv[1];
	sem_init(&s,0,1);	// iniate the semaphore with 1 so we can go in the function - occupy for the first time.

	res_open = open(requests_file, O_RDONLY, 0644);
	if (res_open == -1) {printf("\ncan't open file\n");exit(0);}

	number_of_requests= number_of_lines_in_file(res_open);

	close(res_open);	
	res_open = open(requests_file, O_RDONLY, 0644);
	if (res_open == -1) {printf("\ncan't open file\n");exit(0);}

	iniate_array_flights();
	allocate_array_of_threads();
	succeded_requests_counter = 0;	//iniate global variable
	iniate_empty_chairs_array();	//number of 0 chairs are occupied -in the begining- they all are empty for all the flies
	allocate_buffers();					
	i_array=(int*)malloc(number_of_requests * sizeof(int));
	if(i_array ==NULL){printf("\nCan't allocate i_array\n");exit(0);}
	
	for(i=0; i< number_of_requests ;i++)// the primary loop which pass on all the requests in the file :	
	{	
			res_read = read(res_open,Array_of_buffers[i], BUF_SIZE);  // buffer for each line(request)
			if (res_read == -1) { printf("\ncan't read file\n");exit(0); }
	}

	for(i=0; i< number_of_requests ;i++)// the primary loop which pass on all the requests in the file :	
	{
			i_array[i]=i;
			res_thread= pthread_create(&threads_array[i], NULL,occupy,(void*)&i_array[i]); 
			if (res_thread != 0) {printf("\nCan't create thread \n"); exit(0);}
	}
	
	sem_destroy(&s);		//DESTROY THE SEMAPHORE AFTER USE , THNK U SEMAPHORE U WAS DID A GREAT JOB 
	close(res_open);		//MUST CLOSE  THE FILE WHICH WAS OPENED BY ME : REQUESTS.TXT FILE
	join_ALL_threads();		//WAITING FOR ALL THE JOBS I HAVE HAD, UNTIL THEY END THEIR WORK 

	///////////////////		LET'S FREE ALL THE ALLOCATED MEMO BY ME:

	free(threads_array);
	free_buffers();
	free(i_array);


		////////////// 	LET'S PRINT RESULTS:

	printf("\nAnswers :\n");
	printf("\nqusion a.which requests faild?\nanswer a.number of faild Requests :%d\n",(number_of_requests- succeded_requests_counter));
	printf("\n\nqusion b.which seats still empty ?\nanswer b:");
	for(j=0 ; j< FLIGHT_COUNT ; j++){
		printf("\n\n\nflight number %d :  %d empty seats.\n",j,empty_chairs_array[j]);
			if(empty_chairs_array[j] !=0){
				printf("\t\tWhich ARE:\n");
				for(i=0 ; i< ROW_COUNT ; i++ ){
					printf("\n\t\tin Row number %d :",i);
					count=0;
					for(k=0; k< SEATS_PER_ROW; k++){
						index= (i*SEATS_PER_ROW) + k;
						if(array_flights[j][index] == 0 ){ 	//CONDITION IF SEAT IS EMPTY
							printf(" (%2d,%2d),",i,k);}
						else count++;
					}//end for on columns
					if(count == SEATS_PER_ROW)printf("no empty seat here.");
				}//end of rows
			}//end of condition
		}//end for on flights
	
	show_map();											//SHOW ME THE MAP OF ALL THE FLIGHTS AFTER ALL THE REQUESTS!
	
	printf("\nQusion c :Which seats have been marked for more than one man?\nAnswer c :There is NO seat like that! we assure that by using semaphore.\n");
	printf("\nQusion d : How long time the program took?\n");
														// what is the time in the END ?
	gettimeofday(&end_time, NULL);
														//substracte  TIME , IN ORDER TO KNOW how long the process was 
	elapsedTime = (end_time.tv_sec - begining_time.tv_sec)*10000.0;
	printf("\nAnswer d : elapsedTime = %f\n", elapsedTime);

	return 0;											//THANK U SIR.
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////1
