#include "multi-lookup.h"
#include "util.h"

#define MAX_BUFFER 20 // this is the maximum number of items that can be held in the array
#define MAX_IP INET6_ADDRSTRLEN
#define MAX_NAME 1025




int main(int argc, char **argv){ // TODO: initialize all mutexes, allocate memory for arrays, build shared struct
    int req; // num of requester threads
    int res; // num of resolver threads
    shared *input;
    pthread_t *threads;
    struct timeval start;
    struct timeval end;
    gettimeofday(&start,NULL);
    //void* ret; // used for return value of threads
//    char *reqlog; // all this probably not needed
//    char *reslog;
//
//    reqlog = malloc(MAX_NAME);
//    reslog = malloc(MAX_NAME);
//
    if(argc < 6) // this includes name of program in number of arguments
    {
        printf("Usage: multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ...]");
        exit(1);
    }
    
    if(sscanf(argv[1], "%d", &req) != 1 || sscanf(argv[2], "%d", &res) != 1)
    {
        fprintf(stderr,("Problem reading numbers"));
        exit(1);
    }
    //printf("Begining to initialize shared\n");
    input = malloc(sizeof(shared));
    //initialize mutexes within shared 'input' structure
    pthread_mutex_init(&(input->r), NULL);
    pthread_mutex_init(&(input->s), NULL);
    pthread_mutex_init(&(input->c), NULL);
    pthread_mutex_init(&(input->resolver_log_mutex), NULL);
    pthread_mutex_init(&(input->requester_log_mutex), NULL);
    
    //initialize condition variables
    pthread_cond_init(&(input->x),NULL);
    pthread_cond_init(&(input->y),NULL);
    
    // allocate memory for arrays within shared input, and variables related to arrays
    input->buffer = malloc((MAX_BUFFER)*sizeof(void*));
    input->counter = 0;
    input->numfiles = argc - 5;
    input->files = malloc(input->numfiles*sizeof(void*)); // allocate array of pointers. these will point to filewrapper structs
    //initialize all file structs in files array, and open all input files
    for(int i = 0; i < input->numfiles; i++)
    {
        input->files[i] = malloc(sizeof(filewrapper));
        input->files[i]->numthreads = 0;
        input->files[i]->done = false;
	//input->files[i]->bad = false;
	//input->files[i]->fm = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_init(&(input->files[i]->fm), NULL);
        input->files[i]->file = fopen(argv[i+5],"r");
	if(input->files[i]->file == NULL)
	{
		fprintf(stderr, "Bad input file name: %s\n", argv[i+5]);
		input->files[i]->done = true;
		//input->files[i]->bad = true;
	}
    }
    //initialize other variables
    input->reqdone = false;
    
    //initialize results file, serviced.txt
    input->results = fopen("results.txt","w");
    input->serviced = fopen("serviced.txt", "w");
    input->resolver_log = fopen(argv[4],"r+");
    input->requester_log = fopen(argv[3],"r+");

    if(input->resolver_log == NULL || input->requester_log == NULL)
    {
	fprintf(stderr, "Bad output file path provided\n");
	exit(1);
    }
    
    //printf("Begining to initialize threads\n");
    //create resolver and requester threads threads
    threads = malloc((req+res)*sizeof(pthread_t));
    for(int i = 0; i < req; i++)
    {
        pthread_create(&threads[i],NULL,requester,(void *)input);
    }
    for(int i = 0; i < res; i++)
    {
        pthread_create(&threads[i+req],NULL,resolver,(void *)input);
    }
    //printf("Main is waiting for threads\n");
    for(int i = 0; i < req+res; i++)
    {
        //pthread_join(threads[i],&ret);
	pthread_join(threads[i],NULL);
    }
    //printf("Threads finished\n");
    free(threads);
    cleanup(input);
    gettimeofday(&end,NULL);
    printf("%lf\n", (((double)end.tv_sec + (double)end.tv_usec / 1000000)
	  - ((double)start.tv_sec+ (double)start.tv_usec/1000000)));
    return 0;
}


//resolver thread code. gets pointer to name from buffer, stores it, then does dnslookup and stores info into results file.
void* resolver(void *input2){
    shared *input;
    char *hostname;
    char *ip;
    int result;
    input = (shared *)input2;
    pthread_mutex_lock(&(input->resolver_log_mutex));
    fprintf(input->resolver_log,"Resolver %lu thread starting\n",pthread_self());
    pthread_mutex_unlock(&(input->resolver_log_mutex));
    while(!(input->reqdone) || input->counter > 0)
    {
	pthread_mutex_lock(&(input->resolver_log_mutex));
	fprintf(input->resolver_log,"Resolver %lu beginning to read from buffer\n",pthread_self());
	pthread_mutex_unlock(&(input->resolver_log_mutex));
	ip = malloc(MAX_IP);
	pthread_mutex_lock(&(input->c));
        while(input->counter == 0)
            {
		pthread_cond_wait(&(input->x),&(input->c));
		if((input->reqdone) && input->counter == 0)
		{
			pthread_mutex_unlock(&(input->c));
			pthread_exit(NULL);
		}
	    }
        //do all resolver stuff, get hostname from buffer, then use dnslookup
        
        
        hostname = input->buffer[(input->counter)-1];
        input->counter -= 1;

	
        pthread_mutex_unlock(&(input->c));

	if(input->counter < MAX_BUFFER)
            pthread_cond_signal(&(input->y)); //moved this to be outside the counter mutex

	pthread_mutex_lock(&(input->resolver_log_mutex));
	fprintf(input->resolver_log,"Resolver %lu has retrieved data from buffer:%s\n",pthread_self(),hostname);
	pthread_mutex_unlock(&(input->resolver_log_mutex));
	//printf("%d\n",input->counter);
        result = dnslookup(hostname,ip,MAX_IP);
	if(result != 0)
	{
		*ip = 0;
		fprintf(stderr,"Error with Hostname: %s\n",hostname);
	}
	pthread_mutex_lock(&(input->resolver_log_mutex));
	fprintf(input->resolver_log,"Resolver %lu found ip:%s\n",pthread_self(), ip);
	pthread_mutex_unlock(&(input->resolver_log_mutex));
        pthread_mutex_lock(&(input->r));
        fprintf(input->results,"%s,%s\n",hostname,ip);
        pthread_mutex_unlock(&(input->r));
        free(hostname); //  free space from hostname since no longer needed. Space was allocated by requester thread
        free(ip);
        //end resolver stuff
	//signal used to be placed here
        pthread_mutex_lock(&(input->resolver_log_mutex));
	fprintf(input->resolver_log,"Resolver %lu placed data into file\n",pthread_self());
	pthread_mutex_unlock(&(input->resolver_log_mutex));
    }
    pthread_mutex_lock(&(input->resolver_log_mutex));
    fprintf(input->resolver_log,"Resolver %lu exiting\n",pthread_self());
    pthread_mutex_unlock(&(input->resolver_log_mutex));
    pthread_exit(NULL);
}


//requester will allocate space for a hostname, place the address to it in buffer, then the resolver will take that address, use it, then free the space
void* requester( void *input2){
    shared *input;
    int num; // number of files this thread has serviced
    int f; // file being worked on. f is the index of the file within the array of files in 'input'
    char *name; // name currently being worked on
    input = (shared *)input2;
    pthread_mutex_lock(&(input->requester_log_mutex));
    fprintf(input->requester_log,"Requester %lu thread starting\n",pthread_self());
    pthread_mutex_unlock(&(input->requester_log_mutex));
    num = 0;
    
    while(!(input->reqdone))
    {
        f = select_file(input);
	pthread_mutex_lock(&(input->requester_log_mutex));
	fprintf(input->requester_log,"Requester %lu has chosen a file\n",pthread_self());
	pthread_mutex_unlock(&(input->requester_log_mutex));
        num++;
        while(!(input->files[f]->done))
        {
            name = malloc(MAX_NAME);
            if(fscanf(input->files[f]->file, "%s", name) == EOF)
            {
		pthread_mutex_lock(&(input->requester_log_mutex));
		fprintf(input->requester_log,"Requester %lu reached EOF\n",pthread_self());
		pthread_mutex_unlock(&(input->requester_log_mutex));
                free(name);
                pthread_mutex_lock(&(input->files[f]->fm));
                input->files[f]->done = true;
                pthread_mutex_unlock(&(input->files[f]->fm));
            }
            else {
		pthread_mutex_lock(&(input->requester_log_mutex));
		fprintf(input->requester_log,"Requester %lu read data and is beginning process of placing it in buffer\n",pthread_self());
		pthread_mutex_unlock(&(input->requester_log_mutex));
		pthread_mutex_lock(&(input->c));
                while(input->counter > MAX_BUFFER-1)
                    pthread_cond_wait(&(input->y),&(input->c));
                
                input->counter += 1;
                input->buffer[(input->counter)-1] = name;

		
                pthread_mutex_unlock(&(input->c));

		if(input->counter > 0) // moved this to inside counter mutex
                    pthread_cond_signal(&(input->x));
                //signal used to be here
                //printf("Requester has placed data in buffer\n");
            }
        }
	req_finished(input);
    }
    pthread_mutex_lock(&(input->s));
    fprintf(input->serviced, "Thread %lu serviced %d files\n",pthread_self(),num);
    pthread_mutex_unlock(&(input->s));

    pthread_mutex_lock(&(input->requester_log_mutex));
    fprintf(input->requester_log,"Thread %lu exiting\n", pthread_self());
    pthread_mutex_unlock(&(input->requester_log_mutex));

    pthread_exit(NULL);
}

bool req_finished(shared *input){
    bool finished;
    finished = true;
    for(int i = 0; i < input->numfiles; i++)
    {
        if(!(input->files[i]->done))
            finished = false;
    }
    if(finished)
        input->reqdone = true;
    return finished;
}

//find file that is not done and has the least number of threads working on it
int select_file(shared *input){
    int lowest = 0;
    for(int i = 0; i < input->numfiles; i++)
    {
        if(input->files[i]->numthreads < input->files[lowest]->numthreads && !(input->files[i]->done))
            lowest = i;
    }
    pthread_mutex_lock(&(input->files[lowest]->fm));
    input->files[lowest]->numthreads++;
    pthread_mutex_unlock(&(input->files[lowest]->fm));
    return lowest;
}
//does the cleanup of freeing the memory of shared variables and closing all files
int cleanup(shared *input){
    //printf("Beginning cleanup\n");
    //close all files
    fclose(input->results);
    fclose(input->serviced);
    fclose(input->resolver_log);
    fclose(input->requester_log);
    for(int i = 0; i < input->numfiles; i++)
    {
	if(input->files[i]->file != NULL)
        	fclose(input->files[i]->file);
    }
    //free all memory
    for(int i = 0; i < input->numfiles; i++)
    {
        free(input->files[i]);
    }
    free(input->files);
    free(input->buffer);
    free(input);
    return 0;
}
