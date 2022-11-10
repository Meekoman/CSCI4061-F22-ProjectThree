/* CSCI-4061 Fall 2022
 * Group Member #1: Amir Mohamed, moha1276
 * Group Member #2: Thomas Suiter, suite014
 * Group Member #3: Shannon Wallace, walla423
 */

#include "server.h"
#define PERM 0644

//Global Variables [Values Set in main()]
int queue_len           = INVALID;                              //Global integer to indicate the length of the queue
int cache_len           = INVALID;                              //Global integer to indicate the length or # of entries in the cache        
int num_worker          = INVALID;                              //Global integer to indicate the number of worker threads
int num_dispatcher      = INVALID;                              //Global integer to indicate the number of dispatcher threads      
FILE *logfile;                                                  //Global file pointer for writing to log file in worker


/* ************************ Global Hints **********************************/

//int cache[];                                  //[Cache]           --> When using cache, how will you track which cache entry to evict from array?
int workerIndex = 0;                            //[worker()]        --> How will you track which index in the request queue to remove next?
int dispatcherIndex = 0;                        //[dispatcher()]    --> How will you know where to insert the next request received into the request queue?
int curequest= 0;                               //[multiple funct]  --> How will you update and utilize the current number of requests in the request queue?


pthread_t worker_thread[MAX_THREADS];           //[multiple funct]  --> How will you track the p_thread's that you create for workers?
pthread_t dispatcher_thread[MAX_THREADS];       //[multiple funct]  --> How will you track the p_thread's that you create for dispatchers?
int threadID[MAX_THREADS];                      //[multiple funct]  --> Might be helpful to track the ID's of your threads in a global array


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;        //What kind of locks will you need to make everything thread safe? [Hint you need multiple]
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t some_content = PTHREAD_COND_INITIALIZER;  //What kind of CVs will you need  (i.e. queue full, queue empty) [Hint you need multiple]
pthread_cond_t free_space = PTHREAD_COND_INITIALIZER;
request_t req_entries[MAX_QUEUE_LEN];                    //How will you track the requests globally between threads? How will you ensure this is thread safe?


//cache_entry_t* ?????;                                  //[Cache]  --> How will you read from, add to, etc. the cache? Likely want this to be global

/**********************************************************************************/


/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGGESTION. FEEL FREE TO MODIFY AS NEEDED
*/


/* ******************************** Cache Code  ***********************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *request){
  /* TODO (GET CACHE INDEX)
  *    Description:      return the index if the request is present in the cache otherwise return INVALID
  */
  return INVALID;
}

// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, char *memory , int memory_size){
  /* TODO (ADD CACHE)
  *    Description:      It should add the request at an index according to the cache replacement policy
  *                      Make sure to allocate/free memory when adding or replacing cache entries
  */
}

// Function to clear the memory allocated to the cache
void deleteCache(){
  /* TODO (CACHE)
  *    Description:      De-allocate/free the cache memory
  */

}

// Function to initialize the cache
void initCache(){
  /* TODO (CACHE)
  *    Description:      Allocate and initialize an array of cache entries of length cache size
  */
  
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char* getContentType(char *mybuf) {
  /* TODO (Get Content Type)
  *    Description:      Should return the content type based on the file type in the request
  *                      (See Section 5 in Project description for more details)
  *    Hint:             Need to check the end of the string passed in to check for .html, .jpg, .gif, etc.
  */


   //TODO remove this line and return the actual content type
  return NULL;
}

// Function to open and read the file from the disk into the memory. Add necessary arguments as needed
// Hint: caller must malloc the memory space
int readFromDisk(int fd, char *mybuf, void **memory) {
  //    Description: Try and open requested file, return INVALID if you cannot meaning error


  FILE *fp;
  if((fp = fopen(mybuf, "r")) == NULL){
     fprintf (stderr, "ERROR: Fail to open the file.\n");
    return INVALID;
  }

   fprintf (stderr,"The requested file path is: %s\n", mybuf);
  
  /* TODO 
  *    Description:      Find the size of the file you need to read, read all of the contents into a memory location and return the file size
  *    Hint:             Using fstat or fseek could be helpful here
  *                      What do we do with files after we open them?
  */



  //TODO remove this line and follow directions above
  return INVALID;
}

/**********************************************************************************/

// Function to receive the path request from the client and add to the queue
void * dispatch(void *arg) {

  /********************* DO NOT REMOVE SECTION - TOP     *********************/


  /* TODO (B.I)
  *    Description:      Get the id as an input argument from arg, set it to ID (tid)
  */
  long tid = (long)arg;
  char *fileName[1024];
  request_t file;

  while (1) {
    /* TODO (FOR INTERMEDIATE SUBMISSION)
    *    Description:      Receive a single request and print the conents of that request
    *                      The TODO's below are for the full submission, you do not have to use a 
    *                      buffer to receive a single request 
    *    Hint:             Helpful Functions: int accept_connection(void) | int get_request(int fd, char *filename
    *                      Recommend using the request_t structure from server.h to store the request. (Refer section 15 on the project write up)
    */


    /* TODO (B.II)
    *    Description:      Accept client connection
    *    Utility Function: int accept_connection(void) //utils.h => Line 24
    */
    file.fd = accept_connection();
    if (file.fd < 0) {
      perror("Invalid file\n");
      exit(0);
    }


    /* TODO (B.III)
    *    Description:      Get request from the client
    *    Utility Function: int get_request(int fd, char *filename); //utils.h => Line 41
    */
    file.request = *fileName;
    get_request(file.fd, file.request); 
    if ( file.request != 0) {
      perror("Invalid Request\n");
      // TODO: Skip return_request or return_error for connection
    }


    //fprintf(stderr, "Dispatcher Received Request: fd[%d] request[%s]\n", tempreq.fd, tempreq.request);
    /* TODO (B.IV)
    *    Description:      Add the request into the queue
    */

        //(1) Copy the filename from get_request into allocated memory to put on request queue
        

        //(2) Request thread safe access to the request queue
        pthread_mutex_lock(&lock);
        
        //(3) Check for a full queue... wait for an empty one which is signaled from req_queue_notfull

        //(4) Insert the request into the queue
        
        //(5) Update the queue index in a circular fashion

        //(6) Release the lock on the request queue and signal that the queue is not empty anymore
        pthread_mutex_unlock(&lock);

 }

  return NULL;
}

/**********************************************************************************/
// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) {
  /********************* DO NOT REMOVE SECTION - BOTTOM      *********************/


  #pragma GCC diagnostic ignored "-Wunused-variable"      //TODO --> Remove these before submission and fix warnings
  #pragma GCC diagnostic push                             //TODO --> Remove these before submission and fix warnings


  // Helpful/Suggested Declarations
  int num_request = 0;                                    //Integer for tracking each request for printing into the log
  bool cache_hit  = false;                                //Boolean flag for tracking cache hits or misses if doing 
  int filesize    = 0;                                    //Integer for holding the file size returned from readFromDisk or the cache
  void *memory    = NULL;                                 //memory pointer where contents being requested are read and stored
  int fd          = INVALID;                              //Integer to hold the file descriptor of incoming request
  char mybuf[BUFF_SIZE];                                  //String to hold the file path from the request

  #pragma GCC diagnostic pop                              //TODO --> Remove these before submission and fix warnings



  /* TODO (C.I)
  *    Description:      Get the id as an input argument from arg, set it to ID
  */


  while (1) {
    /* TODO (C.II)
    *    Description:      Get the request from the queue and do as follows
    */
          //(1) Request thread safe access to the request queue by getting the req_queue_mutex lock

          //(2) While the request queue is empty conditionally wait for the request queue lock once the not empty signal is raised

          //(3) Now that you have the lock AND the queue is not empty, read from the request queue

          //(4) Update the request queue remove index in a circular fashion

          //(5) Check for a path with only a "/" if that is the case add index.html to it

          //(6) Fire the request queue not full signal to indicate the queue has a slot opened up and release the request queue lock

    /* TODO (C.III)
    *    Description:      Get the data from the disk or the cache 
    *    Local Function:   int readFromDisk(//necessary arguments//);
    *                      int getCacheIndex(char *request);  
    *                      void addIntoCache(char *mybuf, char *memory , int memory_size);  
    */


    /* TODO (C.IV)
    *    Description:      Log the request into the file and terminal
    *    Utility Function: LogPrettyPrint(FILE* to_write, int threadId, int requestNumber, int file_descriptor, char* request_str, int num_bytes_or_error, bool cache_hit);
    *    Hint:             Call LogPrettyPrint with to_write = NULL which will print to the terminal
    *                      You will need to lock and unlock the logfile to write to it in a thread safe manor
    */


    /* TODO (C.V)
    *    Description:      Get the content type and return the result or error
    *    Utility Function: (1) int return_result(int fd, char *content_type, char *buf, int numbytes); //look in utils.h 
    *                      (2) int return_error(int fd, char *buf); //look in utils.h 
    */
  }




  return NULL;

}

/**********************************************************************************/

int main(int argc, char **argv) {

  /********************* DO NOT REMOVE SECTION - TOP     *********************/
  // Error check on number of arguments
  if(argc != 7){
    printf("usage: %s port path num_dispatcher num_workers queue_length cache_size\n", argv[0]);
    return -1;
  }


  int port            = -1;
  char path[PATH_MAX] = "no path set\0";
  num_dispatcher      = -1;                               //global variable
  num_worker          = -1;                               //global variable
  queue_len           = -1;                               //global variable
  cache_len           = -1;                               //global variable


  /********************* DO NOT REMOVE SECTION - BOTTOM  *********************/
  /* TODO (A.I)
  *    Description:      Get the input args --> (1) port (2) path (3) num_dispatcher (4) num_workers  (5) queue_length (6) cache_size
  */
  port = atoi(argv[1]);
  strcpy(path, argv[2]);
  num_dispatcher = atoi(argv[3]);
  num_worker = atoi(argv[4]);
  queue_len  = atoi(argv[5]);
  cache_len  = atoi(argv[6]);

  /* TODO (A.II)
  *    Description:     Perform error checks on the input arguments
  *    Hints:           (1) port: {Should be >= MIN_PORT and <= MAX_PORT} | (2) path: {Consider checking if path exists (or will be caught later)}
  *                     (3) num_dispatcher: {Should be >= 1 and <= MAX_THREADS} | (4) num_workers: {Should be >= 1 and <= MAX_THREADS}
  *                     (5) queue_length: {Should be >= 1 and <= MAX_QUEUE_LEN} | (6) cache_size: {Should be >= 1 and <= MAX_CE}
  */
  if (port < MIN_PORT || port > MAX_PORT) {
    fprintf(stderr, "Port %d is invalid\n", port);
    return -1;
  }
  else if (opendir(path) == NULL) {
    perror("Directory does not exist\n");
    return -1;
  }
  else if (num_dispatcher < 1 || num_dispatcher > MAX_THREADS) {
    perror("Invalid dispatcher number\n");
    return -1;
  }
  else if (num_worker < 1 || num_worker > MAX_THREADS) {
    perror("Invalid worker number\n");
    return -1;
  } 
  else if (queue_len < 1 || queue_len > MAX_QUEUE_LEN) {
    perror("Invalid queue length\n");
    return -1;
  } 
  else if (cache_len < 1 || cache_len > MAX_CE) {
    perror("Invalid cache length\n");
    return -1;
  } 


  /********************* DO NOT REMOVE SECTION - TOP    *********************/
  printf("Arguments Verified:\n\
    Port:           [%d]\n\
    Path:           [%s]\n\
    num_dispatcher: [%d]\n\
    num_workers:    [%d]\n\
    queue_length:   [%d]\n\
    cache_size:     [%d]\n\n", port, path, num_dispatcher, num_worker, queue_len, cache_len);
  /********************* DO NOT REMOVE SECTION - BOTTOM  *********************/


  /* TODO (A.III)
  *    Description:      Open log file
  *    Hint:             Use Global "File* logfile", use "web_server_log" as the name, what open flags do you want?
  */
  FILE *logfile;
	logfile = fopen(LOG_FILE_NAME, "w+"); 

	if (logfile == NULL) {
    perror("Error opening server log\n");
 		exit(-1);
  }


  /* TODO (A.IV)
  *    Description:      Change the current working directory to server root directory
  *    Hint:             Check for error!
  */
  chdir(path);


  /* TODO (A.V)
  *    Description:      Initialize cache  
  *    Local Function:   void    initCache();
  */
  initCache();


  /* TODO (A.VI)
  *    Description:      Start the server
  *    Utility Function: void init(int port); //look in utils.h 
  */
  init(port);


  /* TODO (A.VII)
  *    Description:      Create dispatcher and worker threads 
  *    Hints:            Use pthread_create, you will want to store pthread's globally
  *                      You will want to initialize some kind of global array to pass in thread ID's
  *                      How should you track this p_thread so you can terminate it later? [global]
  */
  // Create worker thread pool
  for(int i = 0; i < num_worker; i++) {
    if(pthread_create(&(worker_thread[i]), NULL, worker, NULL))
      printf("Thread %d failed to create\n", i);

    //Storing thread ID
    threadID[i] = pthread_self();
  }

  // Create dispatch thread pool
  for (int i = 0; i < num_dispatcher; i++) {
    if(pthread_create(&(dispatcher_thread[i]), NULL, dispatch, NULL)) 
      printf("Thread %d failed to create\n", i);

    //Storing thread ID
    threadID[num_worker + i] = pthread_self();
  }


  // Wait for each of the threads to complete their work
  // Threads (if created) will not exit (see while loop), but this keeps main from exiting
  int i;
  for(i = 0; i < num_worker; i++){
    fprintf(stderr, "JOINING WORKER %d \n",i);
    if((pthread_join(worker_thread[i], NULL)) != 0){
      printf("ERROR : Fail to join worker thread %d.\n", i);
    }
  }
  for(i = 0; i < num_dispatcher; i++){
    fprintf(stderr, "JOINING DISPATCHER %d \n",i);
    if((pthread_join(dispatcher_thread[i], NULL)) != 0){
      printf("ERROR : Fail to join dispatcher thread %d.\n", i);
    }
  }
  fprintf(stderr, "SERVER DONE \n");  // will never be reached in SOLUTION
}

