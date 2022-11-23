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
int worker_threadID[MAX_THREADS];               //[multiple funct]  --> Might be helpful to track the ID's of your threads in a global array
int dispatcher_threadID[MAX_THREADS];           //[multiple funct]  --> Might be helpful to track the ID's of your threads in a global array


pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;         
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_not_full = PTHREAD_COND_INITIALIZER;       
pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;
request_t req_entries[MAX_QUEUE_LEN];                           
cache_entry_t* cache;                                          

/**********************************************************************************/


/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGGESTION. FEEL FREE TO MODIFY AS NEEDED
*/


/* ******************************** Cache Code  ***********************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *request){

  for(int i = 0; i < cache_len; i++) {
    if((cache[i].request != NULL) && (strcmp(cache[i].request, request) == 0)){
      return i;
    }
  }
  
  return INVALID;
}

// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, char *memory , int memory_size){

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
}

// Function to clear the memory allocated to the cache
void deleteCache(){

  for(int i = 0; i < cache_len; i++) {
    free(cache[i].request);
    free(cache[i].content);
  }

  free(cache);
}

// Function to initialize the cache
void initCache(){
  cache = malloc(cache_len * sizeof(cache_entry_t));

  if(cache == NULL) {
    perror("Allocating cache has failed\n");
    return;
  }

  for(int i = 0; i < cache_len; i++) {
    cache[i].len = 0;
    cache[i].content = NULL;
    cache[i].request = NULL;
  }
}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
char* getContentType(char *mybuf) {

  char* type;
  char* extension;
  int j = 0;
  for(j = 0; j < strlen(mybuf); j++){
    if (mybuf[j] == '.') {
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

// Function to open and read the file from the disk into the memory. 
int readFromDisk(int fd, char *mybuf, void **memory) {

  int fp;
  if((fp = open(mybuf + 1, O_RDONLY)) == -1){
    fprintf (stderr, "ERROR: Fail to open the file.\n");
    return INVALID;
  }

  struct stat file;
  int ret = fstat(fp, &file);
  if(ret != 0) {
    perror("stat has failed\n");
    return INVALID;
  }

  int fileSize = file.st_size;

  if((*memory = malloc(fileSize)) == NULL) {
    perror("Allocating content into a memory location has failed\n");
    return INVALID;
  }

  int contRead = read(fp, *memory, fileSize);
  if(contRead == -1) {
    perror("Reading contents has failed\n");
    return INVALID;
  }

  if(close(fp)) {
    perror("ERROR: Fail to close the file.\n");
    exit(1);
  }

  return fileSize;
}

// function to print out contents of cache for debugging purposes
// not thread-safe, do not call without cache lock. 
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
// function to print out the contents of the queue for debugging purposes
// not thread-safe, do not call without queue lock. 
void reqQueuePrintDebug() {
  printf ("----request queue-----\n");
  printf ("curequest:  %d \n", curequest);
  for (int i = 0; i < queue_len; i++){
    printf("%d | %d |", i, req_entries[i].fd);
    printf(" %s \n", req_entries[i].request);
  }
}
/**********************************************************************************/

// Function to receive the path request from the client and add to the queue
void * dispatch(void *arg) {

  /********************* DO NOT REMOVE SECTION - TOP     *********************/

  int index = -1;
  index = * (int*)arg;
  fprintf(stderr, "Dispatcher                     [%3d] Started\n", index);

  while (1) {

    int fd = accept_connection();
    if(fd < 0){
      return NULL;
    }

    char fileName[BUFF_SIZE];
    if((get_request(fd, fileName)) != 0){
      perror("ERROR: Failed to get request.\n");
      continue;
    }

    char *req = (char *)malloc(strlen(fileName) + 1);
    if (req == NULL) {
      perror("file request allocation has failed \n");
    } 
    strcpy(req, fileName);
      
    if (pthread_mutex_lock(&queue_lock) < 0 ){
      perror("locking has failed\n");
    }
    

    while (curequest == queue_len)
      pthread_cond_wait(&queue_not_full, &queue_lock);

    req_entries[dispatcherIndex].fd = fd;
    req_entries[dispatcherIndex].request = req;
   
    curequest++;
    dispatcherIndex = (dispatcherIndex + 1) % queue_len;

    if(pthread_cond_signal(&queue_not_empty) < 0) {
      perror("Failed to signal\n");
    }
    if(pthread_mutex_unlock(&queue_lock) < 0) {
      perror("Unlocking has failed\n");
    }
 }

  return NULL;
}

/**********************************************************************************/
// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) {
  /********************* DO NOT REMOVE SECTION - BOTTOM      *********************/

  int num_request = 0;                                    //Integer for tracking each request for printing into the log
  bool cache_hit  = false;                                //Boolean flag for tracking cache hits or misses if doing 
  int filesize    = 0;                                    //Integer for holding the file size returned from readFromDisk or the cache
  void *memory    = NULL;                                 //memory pointer where contents being requested are read and stored
  int fd          = INVALID;                              //Integer to hold the file descriptor of incoming request
  char mybuf[BUFF_SIZE];                                  //String to hold the file path from the request

  int index = -1;
  index = * (int*)arg;
  fprintf(stderr,"Worker                         [%3d] Started \n", index);
  while (1) {

    if (pthread_mutex_lock(&queue_lock) < 0 )
      perror("Locking has failed\n");

    while (curequest == 0) 
      pthread_cond_wait(&queue_not_empty, &queue_lock);

    fd = req_entries[workerIndex].fd;
    strcpy(mybuf, req_entries[workerIndex].request);
    // this was malloc'd in the dispatch. need to free now that it's being used.
    free(req_entries[workerIndex].request); 

    curequest--; 
    workerIndex = (workerIndex + 1) % queue_len;
    num_request++;

    if ((strcmp(mybuf, "/")) == 0) {
      strcpy(mybuf, "/index.html");
    }

    if (pthread_cond_signal(&queue_not_full) < 0) {
      perror("Failed to signal\n");
    }

    if (pthread_mutex_unlock(&queue_lock) < 0) {
      perror("Unlocking has failed\n");
    }


    if (pthread_mutex_lock(&cache_lock)  < 0) {
      perror("Failed to lock cache\n");
    }

    int cache_index = getCacheIndex(mybuf);

    if (cache_index == INVALID) {
      cache_hit = false;
      filesize = readFromDisk(fd, mybuf, &memory);

      if(filesize == -1){
        if(pthread_mutex_unlock(&cache_lock) < 0) {
          perror("Failed to unlock cache\n");
        }
      }

      addIntoCache(mybuf, memory, filesize);
      cache_index = getCacheIndex(mybuf);
    }
    else {
      cache_hit = true;
      filesize = cache[cache_index].len;
      if ((memory = malloc(filesize)) == NULL) {
        perror("Worker failed to allocate memory in cache \n");
      }

      memcpy(memory, cache[cache_index].content, filesize);
    }

    if (pthread_mutex_unlock(&cache_lock) < 0)
      perror("Failed to unlock cache\n");


    if (pthread_mutex_lock(&log_lock) < 0)
      perror("Locking failed\n");

    int currentThreadID = worker_threadID[index];

    LogPrettyPrint(logfile, currentThreadID, num_request, fd, mybuf, filesize, cache_hit);
    LogPrettyPrint(NULL, currentThreadID, num_request, fd, mybuf, filesize, cache_hit);

    if (pthread_mutex_unlock(&log_lock) < 0)
      perror("Unlocking failed\n");


    char *typereturn;
    typereturn = getContentType(mybuf);

    if (return_result(fd, typereturn, memory, filesize) != 0)
      return_error(fd, mybuf);

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
 
  port = atoi(argv[1]);
  strcpy(path, argv[2]);
  num_dispatcher = atoi(argv[3]);
  num_worker = atoi(argv[4]);
  queue_len  = atoi(argv[5]);
  cache_len  = atoi(argv[6]);

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


	logfile = fopen(LOG_FILE_NAME, "a"); 

	if (logfile == NULL) {

    perror("Error opening server log\n");
 		exit(-1);
  }

  if (chdir(path) != 0) {
    fprintf(stderr, "Error changing directories\n");
  }

  initCache();
  init(port);


  for(int i = 0; i < num_worker; i++) {
    worker_threadID[i] = i; 
    if(pthread_create(&(worker_thread[i]), NULL, worker, (void *) &worker_threadID[i] )){
      printf("Thread %d failed to create\n", i);
      continue;
    }
  }

  for (int i = 0; i < num_dispatcher; i++) {
    dispatcher_threadID[i] = i;
    if(pthread_create(&(dispatcher_thread[i]), NULL, dispatch, (void *) &dispatcher_threadID[i] )) {
      fprintf(stderr,"Thread %d failed to create\n", i);
      continue;
    }
  }

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
  deleteCache();
  fclose(logfile);
}