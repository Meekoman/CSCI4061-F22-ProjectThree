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

int cacheIndex = 0;                             //[Cache]           --> When using cache, how will you track which cache entry to evict from array?
int workerIndex = 0;                            //[worker()]        --> How will you track which index in the request queue to remove next?
int dispatcherIndex = 0;                        //[dispatcher()]    --> How will you know where to insert the next request received into the request queue?
int curequest= 0;                               //[multiple funct]  --> How will you update and utilize the current number of requests in the request queue?


pthread_t worker_thread[MAX_THREADS];           //[multiple funct]  --> How will you track the p_thread's that you create for workers?
pthread_t dispatcher_thread[MAX_THREADS];       //[multiple funct]  --> How will you track the p_thread's that you create for dispatchers?
int worker_threadID[MAX_THREADS];                      //[multiple funct]  --> Might be helpful to track the ID's of your threads in a global array
int dispatcher_threadID[MAX_THREADS];                      //[multiple funct]  --> Might be helpful to track the ID's of your threads in a global array


pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;        //What kind of locks will you need to make everything thread safe? [Hint you need multiple]
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_full = PTHREAD_COND_INITIALIZER;  //What kind of CVs will you need  (i.e. queue full, queue empty) [Hint you need multiple]
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;
request_t req_entries[MAX_QUEUE_LEN];                    //How will you track the requests globally between threads? How will you ensure this is thread safe?


cache_entry_t* cache;                                  //[Cache]  --> How will you read from, add to, etc. the cache? Likely want this to be global

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

  for(int i = 0; i < cache_len; i++) {
    if((cache[i].request != NULL) && (strcmp(cache[i].request, request) == 0)){
      return i;
    }
  }
    return INVALID;
}

// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, char *memory , int memory_size){
  /* TODO (ADD CACHE)
  *    Description:      It should add the request at an index according to the cache replacement policy
  *                      Make sure to allocate/free memory when adding or replacing cache entries
  */

  cache[cacheIndex].len = memory_size;

  if(cache[cacheIndex].request != NULL) {
    free(cache[cacheIndex].request);
    cache[cacheIndex].request = NULL;
  }

  if((cache[cacheIndex].request = malloc(strlen(mybuf + 1))) == NULL) {
    perror("Allocation has failed\n");
    return;
  }
  strcpy(cache[cacheIndex].request, mybuf);

  if(cache[cacheIndex].content != NULL){
    free(cache[cacheIndex].content);
    cache[cacheIndex].content = NULL;
  }

  if((cache[cacheIndex].content = malloc(memory_size)) == NULL) {
    perror("Allocation has failed\n");
    return;
  }
  memcpy(cache[cacheIndex].content, memory, memory_size);

  cacheIndex++;
  cacheIndex %= cache_len;

  return;
}

// Function to clear the memory allocated to the cache
void deleteCache(){
  /* TODO (CACHE)
  *    Description:      De-allocate/free the cache memory
  */

  for(int i = 0; i < cache_len; i++) {
    free(cache[i].request);
    free(cache[i].content);
  }
  free(cache);
}

// Function to initialize the cache
void initCache(){
  /* TODO (CACHE)
  *    Description:      Allocate and initialize an array of cache entries of length cache size
  */
  cache = malloc(cache_len * sizeof(cache_entry_t));

  if(cache == NULL) {
    perror("Allocating cache has failed\n");
    return;
  }

  for(int i = 0; i < cache_len; i++) {
    cache[i].len == INVALID;
    cache[i].content = NULL;
    cache[i].request = NULL;
  }
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char* getContentType(char *mybuf) {
  /* TODO (Get Content Type)
  *    Description:      Should return the content type based on the file type in the request
  *                      (See Section 5 in Project description for more files)
  *    Hint:             Need to check the end of the string passed in to check for .html, .jpg, .gif, etc.
  */

  char* type;
  char* extension;
  int j = 0;
  for(j = 0; j < strlen(mybuf); j++){
    if (mybuf[j] == '.') {
      // type = &mybuf[j];
      extension = mybuf + j;
      break;
    }
    else {
      continue;
    }
  }

  

  if(strcmp(extension, ".html") == 0 || strcmp(extension, ".htm") == 0) {
    type = "text/html\0";
  }
  else if(strcmp(extension, ".jpg") == 0) {
    type = "image/jpeg\0";
  }
  else if(strcmp(extension, ".gif") == 0) {
    type = "image/gif\0";
  }
  else {
    type = "text/plain\0";
  }
    return type;
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
  struct stat file;
  int ret = stat(mybuf, &file);
  if(ret != 0) {
    perror("stat has failed\n");
    return INVALID;
  }

  int fileSize = file.st_size;

  if((*memory = malloc(fileSize)) == NULL) {
    perror("Allocating content into a memory location has failed\n");
    return INVALID;
  }

  int contRead = fread(*memory, fileSize, 1, fp);
  if(contRead != 1) {
    perror("Reading contents has failed\n");
    return INVALID;
  }

  if(fclose(fp)) {
    fprintf(stderr, "ERROR: Fail to close the file.\n");
    exit(1);
  }
    return fileSize;
}

// function to print out contents of cache for debugging purposes
void cachePrintDebug() {
  printf("i | content?     | request \n");
  printf("____________________________\n");
  for (int i = 0; i < cache_len; i++){
    printf("%d", i);
    if (cache[i].content == NULL)
      printf(" | content NULL | ");
    else
      printf(" | content pres | ");
    printf("%s \n", cache[i].request);
  }
}
/**********************************************************************************/

// Function to receive the path request from the client and add to the queue
void * dispatch(void *arg) {

  /********************* DO NOT REMOVE SECTION - TOP     *********************/


  /* TODO (B.I)
  *    Description:      Get the id as an input argument from arg, set it to ID (tid)
  */
  int index = -1;
  index = * (int*)arg;
  request_t file;

  printf("Dispatch entered, arg = %d \n", index);

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
    while((file.fd = accept_connection()) < 0);


    /* TODO (B.III)
    *    Description:      Get request from the client
    *    Utility Function: int get_request(int fd, char *filename); //utils.h => Line 41
    */
    char fileName[BUFF_SIZE];
    if((get_request(file.fd, fileName)) != 0){
      printf("ERROR: Failed to get request.\n");
      continue;
    }
    fprintf(stderr, "fileName: %s\n", fileName);

    fprintf(stderr, "Dispatcher Received Request: fd[%d] request[%s]\n", file.fd, fileName);
    /* TODO (B.IV)
    *    Description:      Add the request into the queue
    */


    //(1) Copy the filename from get_request into allocated memory to put on request queue
    file.request = malloc(strlen(fileName) + 1);
    if (file.request == NULL) {
      perror("file request allocation has failed \n");
    } 
    strncpy(file.request, fileName, BUFF_SIZE);
      

    //(2) Request thread safe access to the request queue
    if (pthread_mutex_lock(&queue_lock) != 0 ){
      perror("locking has failed\n");
    }
    
    //(3) Check for a full queue... wait for an empty one which is signaled from queue_not_full
    while (curequest >= MAX_QUEUE_LEN)
      pthread_cond_wait(&queue_not_full, &queue_lock);


    //(4) Insert the request into the queue
    req_entries[curequest] = file;
    
    //(5) Update the queue index in a circular fashion
    curequest++;

    //(6) Release the lock on the request queue and signal that the queue is not empty anymore
    pthread_cond_signal(&queue_not_empty);
    if(pthread_mutex_unlock(&queue_lock) != 0) {
      perror("unlocking has failed\n");
    }
  
 }

  return NULL;
}

/**********************************************************************************/
// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) {
  /********************* DO NOT REMOVE SECTION - BOTTOM      *********************/


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
  int index = -1;
  index = * (int*)arg;
  printf("Worker entered, arg = %d \n", index);
  
  while (1) {
    /* TODO (C.II)
    *    Description:      Get the request from the queue and do as follows
    */
    //(1) Request thread safe access to the request queue by getting the req_queue_mutex lock
  
    if (pthread_mutex_lock(&queue_lock) != 0 ){
      perror("locking has failed\n");
    }

    //(2) While the request queue is empty conditionally wait for the request queue lock once the not empty signal is raised
    if (curequest <= 0) {
      pthread_cond_wait(&queue_not_empty, &queue_lock);
    }

    //(3) Now that you have the lock AND the queue is not empty, read from the request queue
    curequest--; // curequest always points to open slot above latest filled one. decrementing it will make it point to the full one. 
    fd = req_entries[curequest].fd;
    strncpy(mybuf, req_entries[curequest].request, BUFF_SIZE);
    num_request = curequest;
    
    //(4) Update the request queue remove index in a circular fashion
    free(req_entries[curequest].request); // this was malloc'd in the dispatch. need to free now that it's being used.  

    //(5) Check for a path with only a "/" if that is the case add index.html to it
    if ((strcmp(mybuf, "/")) == 0) {
      strcat(mybuf, "index.html");
    }

    //(6) Fire the request queue not full signal to indicate the queue has a slot opened up and release the request queue lock
    pthread_cond_signal(&queue_not_full);
    pthread_mutex_unlock(&queue_lock);


    /* TODO (C.III)
    *    Description:      Get the data from the disk or the cache 
    *    Local Function:   int readFromDisk(int fd, char *mybuf, void **memory);
    *                      int getCacheIndex(char *request);  
    *                      void addIntoCache(char *mybuf, char *memory , int memory_size);  
    */    
    // at this point requested file is saved in mybuf
    // probably need to chdir to 


    // 1) Look up the request in the cache.
    // 2a) If the request is in the cache (Cache HIT), 
    //       -get the result from the cache,
    //       -return result to the user. 
    // 2b) If the request is not in the cache (Cache MISS), 
    //       -get the result from disk as usual, 
    //       -put the entry in the cache,
    //       -return result to the user.

    // make filename absolute, not relative 
    char absFilepath[1024];
    getcwd(absFilepath, 1024);
    strcat(absFilepath, mybuf);

    pthread_mutex_lock(&cache_lock);
    int cache_index = getCacheIndex(absFilepath);
    num_request ++;
    printf("mybuf: %s \n", mybuf);
    printf("cache index: %d \n", cache_index);
    cachePrintDebug();
    //If request in not in cache, get from disk
    if (cache_index == INVALID) {
      cache_hit = false;
      int requestfd = open(absFilepath, O_RDONLY);
      struct stat file;
      fstat(requestfd, &file);
      filesize = file.st_size;

      memory = malloc(filesize);
      if (memory == NULL) {
        perror("memory failed to allocate \n");
      }

      if ((readFromDisk(requestfd, absFilepath, &memory) == INVALID)) {
        perror("failed to read from disk\n");
        return NULL;
      }
      addIntoCache(absFilepath, memory, filesize);
      cache_index = getCacheIndex(mybuf);
    }
    //Request is in cache, place in buffer
    else {
      cache_hit = true;
      filesize = cache[cache_index].len;
      memory = malloc(filesize);

      //file contents into allocated memory
      memcpy(memory, cache[cache_index].content, filesize);

    }

    pthread_mutex_unlock(&cache_lock);

   
    /* TODO (C.IV)
    *    Description:      Log the request into the file and terminal
    *    Utility Function: LogPrettyPrint(FILE* to_write, int threadId, int requestNumber, int file_descriptor, char* request_str, int num_bytes_or_error, bool cache_hit);
    *    Hint:             Call LogPrettyPrint with to_write = NULL which will print to the terminal
    *                      You will need fileNameto lock and unlock the logfile to write to it in a thread safe manor
    */
    pthread_mutex_lock(&log_lock);
    int currentThreadID = worker_threadID[index];

    // To file
    LogPrettyPrint(logfile, currentThreadID, num_request, fd, mybuf, filesize, cache_hit);
    // To terminal
    LogPrettyPrint(NULL, currentThreadID, num_request, fd, mybuf, filesize, cache_hit);

    pthread_mutex_unlock(&log_lock);


    /* TODO (C.V)
    *    Description:      Get the content type and return the result or error
    *    Utility Function: (1) int return_result(int fd, char *content_type, char *buf, int numbytes); //look in utils.h 
    *                      (2) int return_error(int fd, char *buf); //look in utils.h 
    */
    char *typereturn;
    typereturn = getContentType(mybuf);

    if (return_result(fd, typereturn, memory, filesize) != 0) {
      return_error(fd, mybuf);
    }

    free(memory);
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
  *    Description:     Perform error checName: /image/jpg/29.jpgks on the input arguments
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
	logfile = fopen(LOG_FILE_NAME, "a"); 

	if (logfile == NULL) {

    perror("Error opening server log\n");
 		exit(-1);
  }


  /* TODO (A.IV)
  *    Description:      Change the current working directory to server root directory
  *    Hint:             Check for error!
  */
  if (chdir(path) != 0) {
    fprintf(stderr, "Error changing directoreis\n");
  }

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
    worker_threadID[i] = i; 
    if(pthread_create(&(worker_thread[i]), NULL, worker, (void *) &worker_threadID[i] )){
      printf("Thread %d failed to create\n", i);
      continue;
    }
  }

  // Create dispatch thread pool
  for (int i = 0; i < num_dispatcher; i++) {
    dispatcher_threadID[i] = i;
    if(pthread_create(&(dispatcher_thread[i]), NULL, dispatch, (void *) &dispatcher_threadID[i] )) {
      printf("Thread %d failed to create\n", i);
      continue;
    }
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
  fclose(logfile);
}

