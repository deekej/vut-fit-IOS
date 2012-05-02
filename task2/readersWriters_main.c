/**
 * File:          readersWriters_main.c
 * Version:       0.0
 * Date:          29-04-2012
 * Last update:   29-04-2012
 *
 * Course:        IOS (summer semester, 2012)
 * Project:       Project #2 (Processes synchronization problem.)
 *
 * Author:        David Kaspar (aka Dee'Kej), 1BIT
 * Faculty:       Faculty of Information Technologies,
 *                Brno University of Technologies
 * E-mail:        xkaspa34@stud.fit.vutbr.cz
 *
 * Description:   Program implementing solution to Readers-Writers sync problem.
 *                See the Wiki for more information about the problem and the
 *                link below for more information about given instructions.
 *
 * More info @:   http://www.fit.vutbr.cz/study/courses/IOS/public/Lab/projekt2/
 *
 * File encoding: en_US.utf8 (United States)
 * Compiler used: gcc 4.5.2 (Ubuntu/Linaro 4.5.2-8ubuntu4)
 */


/******************************************************************************
 ***[ START OF READERSWRITERS_MAIN.C ]*****************************************
 ******************************************************************************/


/******************************************************************************
 ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/
//{{{
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
//}}}

/******************************************************************************
 ~~~[ GLOBAL CONSTANTS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

/* Name to be used to identify shared memory and semaphores of this program. */
#define SHM_NAME "/xkaspa34"

/* Name of shared memory. */
#define SHARED_MEM        SHM_NAME "_shm"

/* Names of semaphores used. */
#define SEM_READ          SHM_NAME "_read"
#define SEM_WRITE         SHM_NAME "_write"
#define SEM_RDRS_NUM      SHM_NAME "_rdrs_num"
#define SEM_WRTRS_NUM     SHM_NAME "_wrtrs_num"
#define SEM_COUNTER       SHM_NAME "_counter"
#define SEM_WRTRS_ALIVE   SHM_NAME "_wrtrs_alive"

const int ARGS_NUM = 6;                   /* 6 arguments required. */


/******************************************************************************
 ~~~[ DATA TYPES DECLARATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/
//{{{
/* Structure for storing processed arguments of invocated program.  */
 typedef struct arguments {
  unsigned writers_num;                   /* Number of writers. */
  unsigned writers_slpt;                  /* Sleep time of writers [ms]. */

  unsigned readers_num;                   /* Number of readers. */
  unsigned readers_slpt;                  /* Sleep time of writers [ms]. */

  unsigned cycles_count;                  /* Number of cycles. */

  char *p_fname;                          /* Output filename. */
 } TS_arguments;


/* Structure to be mapped as a shared memory between processes. */
 typedef struct shared_mem {
  int counter;                            /* Actions counter. */
  int last_writer;                        /* Internal number of last writer. */

  unsigned rdrs_num;                      /* Number of readers reading. */
  unsigned wrtrs_num;                     /* Number of writers writing. */

  unsigned wrtrs_alive;                   /* Number of writers alive. */
 } TS_shared_mem;


/* Structure containing pointers to all semaphores used by program. */
typedef struct semaphores {
  sem_t *read;                /* Semaphore of reading possibility. */
  sem_t *write;               /* Semaphore of writing possibility. */
  sem_t *rdrs_num;            /* Semaphore of shared memory for rdrs_num. */
  sem_t *wrtrs_num;           /* Semaphore of shared memory for wrtrs_num. */
  sem_t *counter;             /* Semaphore of shared memory for act_count. */
  sem_t *wrtrs_alive;         /* Semaphore of shared memory for wrtrs_alive. */
} TS_semaphores;
//}}}

/******************************************************************************
 ~~~[ FUNCTIONAL PROTOTYPES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

void process_args(int argc, char *argv[], TS_arguments *p_args);
static inline void display_usage(char *prg_name);

int semaphores_open(TS_semaphores *sem);
void semaphores_close(TS_semaphores *sem);
void semaphores_unlink(void);

static inline void sem_lock(sem_t *sem, const char *proc, unsigned id);
static inline void sem_unlock(sem_t *sem, const char *proc, unsigned id);

void writer(TS_shared_mem *shm, TS_semaphores *sem, unsigned id, unsigned slpt,
            unsigned cycles);

/******************************************************************************
 ~~~[ AUXILIARY FUNCTIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

/**
 * Processing arguments of invocated program and storing them into given
 * structure.
 */
void process_args(int argc, char *argv[], TS_arguments *p_args)
{{{
  /* Too few arguments? */
  if (argc < ARGS_NUM + 1) {
    fprintf(stderr, "%s: too few arguments\n", argv[0]);
    display_usage(argv[0]);
    exit(EXIT_FAILURE);
  }
  /* Too many arguments? */
  else if (argc > ARGS_NUM + 1) {
    fprintf(stderr, "%s: too many arguments\n", argv[0]);
    display_usage(argv[0]);
    exit(EXIT_FAILURE);
  }


  long long int result;                       /* Result of strtoll(). */
  char *endptr;                               /* End pointer for strtoll(). */

  int i;                                      /* Iteration variable. */

  /*
   * Processing first ARGS_NUM -1 arguments (without the name of invocated
   * program.)
   */
  for (i = 1; i < ARGS_NUM; i++) {

    result = strtoll(argv[i], &endptr, 10);
    
    /* Invalid character in number encountered? */
    if (*endptr != '\0') {
      fprintf(stderr, "%s: %s: invalid number\n", argv[0], argv[i]);
      display_usage(argv[0]);
      exit(EXIT_FAILURE);
    }
    /* Result value of long long range? */
    else if (errno == ERANGE) {
      fprintf(stderr, "%s: %s: result value is too large\n", argv[0], argv[i]);
      display_usage(argv[0]);
      exit(EXIT_FAILURE);
    }
    /* Negative value? */
    else if (result < 0) {
      fprintf(stderr, "%s: %s: negative values are not supported\n", argv[0],
              argv[i]);
      display_usage(argv[0]);
      exit(EXIT_FAILURE);
    }
    /* Value too big to be stored in unsigned int? */
    else if ((unsigned long long) result > ULONG_MAX) {
      fprintf(stderr, "%s: %s: value too large to be stored in data type\n",
              argv[0], argv[i]);
      display_usage(argv[0]);
      exit(EXIT_FAILURE);
    }

    /* Assigning into appropriate member of given structure. */
    switch (i) {
      case 1 :
        p_args->writers_num = (unsigned) result;
        break;

      case 2 :
        p_args->readers_num = (unsigned) result;
        break;

      case 3 :
        p_args->cycles_count = (unsigned) result;
        break;

      case 4 :
        p_args->writers_slpt = (unsigned) result;
        break;

      case 5 :
        p_args->readers_slpt = (unsigned) result;
        break;

      default :
        break;
    }
  }
  
  /* Processing last argument. */
  if (strcmp(argv[i], "-") == 0) {
    p_args->p_fname = NULL;
  }
  else {
    p_args->p_fname = argv[i];
  }

  return;
}}}


/**
 * Displays 'small help' in case incorrect arguments were used in time of
 * program's invocation.
 */
static inline void display_usage(char *prg_name)
{{{
  fprintf(stderr, "Usage: %s W R C SW SR OUT\n"
                  "    W - number of writers to simulate\n"
                  "    R - number of readers to simulate\n"
                  "    C - number of live cycles of readers and writers\n"
                  "   SW - simulation time scale of writers [ms]\n"
                  "   SR - simulation time scale of readers [ms]\n"
                  "  OUT - name of output file, use hyphen (-) for stdout\n"
                  "It is mandatory to supply all options.\n"
                  "\n"
                  "For more info visit webpage (2012):\n"
                  "http://www.fit.vutbr.cz/study/courses/IOS/public/Lab/"
                  "projekt2/projekt2.html\n"
                  "\n"
                  "Written by David Kaspar aka Dee'Kej "
                  "(xkaspa34@stud.fit.vutbr.cz).\n", prg_name);
  return;
}}}


/**
 * Tries to open all semaphores of given TS_semaphores structure. Returns
 * EXIT_FAILURE and errno is set, otherwise EXIT_SUCCESS is returned.
 */
int semaphores_open(TS_semaphores *sem)
{{{
  /* Opening and initializing semaphores in given semaphores structure. */
  sem->read = sem_open(SEM_READ, O_CREAT, 0600, 1);
  sem->write = sem_open(SEM_WRITE, O_CREAT, 0600, 1);
  sem->rdrs_num = sem_open(SEM_RDRS_NUM, O_CREAT, 0600, 1);
  sem->wrtrs_num = sem_open(SEM_WRTRS_NUM, O_CREAT, 0600, 1);
  sem->counter = sem_open(SEM_COUNTER, O_CREAT, 0600, 1);
  sem->wrtrs_alive = sem_open(SEM_WRTRS_ALIVE, O_CREAT, 0600, 1);

  /* Test of successful opening of semaphores. */
  if (sem->write == SEM_FAILED || sem->wrtrs_num == SEM_FAILED ||
      sem->read == SEM_FAILED || sem->rdrs_num == SEM_FAILED ||
      sem->counter == SEM_FAILED || sem->wrtrs_alive == SEM_FAILED) {
    
    /* Backup of errno because functions below can change it. */
    int errno_backup = errno;

    /* Closing and unlinking semaphores because of opening failure. */
    semaphores_close(sem);
    semaphores_unlink();

    errno = errno_backup;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}}}


/**
 * Tries to close every semaphore of given TS_semaphores structure.
 */
void semaphores_close(TS_semaphores *sem)
{{{
  sem_close(sem->read);
  sem_close(sem->write);
  sem_close(sem->rdrs_num);
  sem_close(sem->wrtrs_num);
  sem_close(sem->counter);
  sem_close(sem->wrtrs_alive);

  return;
}}}


/**
 * Tries to unlink all semaphores of this program.
 */
void semaphores_unlink(void)
{{{
  sem_unlink(SEM_READ);
  sem_unlink(SEM_WRITE);
  sem_unlink(SEM_RDRS_NUM);
  sem_unlink(SEM_WRTRS_NUM);
  sem_unlink(SEM_COUNTER);
  sem_unlink(SEM_WRTRS_ALIVE);

  return;
}}}


/**
 * Wrapper function for semaphore locking. Terminates the process in case
 * semaphore lock cannot be achieved. Proc is the name of process who is
 * performing the lock, id is his identifier.
 */
static inline void sem_lock(sem_t *sem, const char *proc, unsigned id)
{{{
  if (sem_wait(sem) != 0) {
    fprintf(stderr, "%s: %u: ", proc, id);
    perror("");
    exit(EXIT_FAILURE);
  }
  
  return;
}}}


/**
 * Wrapper function for semaphore unlocking. Terminates the process in case
 * semaphore unlock cannot be achieved. Proc is the name of process who is
 * performing the unlock, id is his identifier.
 */
static inline void sem_unlock(sem_t *sem, const char *proc, unsigned id)
{{{
  if (sem_post(sem) != 0) {
    fprintf(stderr, "%s: %u: ", proc, id);
    perror("");
    exit(EXIT_FAILURE);
  }
  
  return;
}}}


/******************************************************************************
 ~~~[ PRIMARY FUNCTIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

void writer(TS_shared_mem *shm, TS_semaphores *sem, unsigned id, unsigned slpt,
            unsigned cycles)
{{{
  /* Lock for 'writers alive'. */
  sem_lock(sem->wrtrs_alive, "writer", id);
  {
    /* 
     * Increasing value, so the main can determine how many writers are still 
     * alive.
     */
    shm->wrtrs_alive++;
  }
  sem_unlock(sem->wrtrs_alive, "writer", id);


  srand((unsigned int) time(NULL));

  
  /* Repeats for number specified in cycles. */
  for (unsigned i = 0; i < cycles; i++) {

    /* Lock for actions counter (output). */
    sem_lock(sem->counter, "writer", id);
    {
      fprintf(stdout, "%d: writer: %u: new value\n", shm->counter, id);
      shm->counter++;
    }
    sem_unlock(sem->counter, "writer", id);


    /* Trying to put process to sleep from 0 to slpt milliseconds. */
    if (slpt != 0 && usleep(1000 * (rand() % slpt)) != 0) {
      fprintf(stderr, "writer: %u: ", id);
      perror("");
      exit(EXIT_FAILURE);
    }


    /* Lock for actions counter (output). */
    sem_lock(sem->counter, "writer", id);
    {
      fprintf(stdout, "%d: writer: %u: ready\n", shm->counter, id);
      shm->counter++;
    }
    sem_unlock(sem->counter, "writer", id);

    
    /* Lock for number of actual writers writing. */
    sem_lock(sem->wrtrs_num, "writer", id);
    {
      shm->wrtrs_num++;
      
      /*
       * First writer also locks 'read' semaphore, so the new coming readers
       * can't access the shared memory for the time of writers performing
       * actions.
       */
      if (shm->wrtrs_num == 1) {
        sem_lock(sem->read, "writer", id);
      }
    }
    sem_unlock(sem->wrtrs_num, "writer", id);


    /* 
     * Lock for writer's semaphore, allowing only one writer at the time to edit
     * shared memory.
     */
    sem_lock(sem->write, "writer", id);
    {
      /* Lock for actions counter (output). */
      sem_lock(sem->counter, "writer", id);
      {
        fprintf(stdout, "%d: writer: %u: writes a value\n", shm->counter, id);
        shm->counter++;
      }
      sem_unlock(sem->counter, "writer", id);
      
      shm->last_writer = id;              /* Writer is writing his value. */
      
      /* Lock for actions counter (output). */
      sem_lock(sem->counter, "writer", id);
      {
        fprintf(stdout, "%d: writer: %u: written\n", shm->counter, id);
        shm->counter++;
      }
      sem_unlock(sem->counter, "writer", id);
    }
    sem_unlock(sem->write, "writer", id);

  
    /* Lock for number of actual writers writing. */
    sem_lock(sem->wrtrs_num, "writer", id);
    {
      shm->wrtrs_num--;
      
      /*
       * If the actual writer is the last one, then he opens semaphore for
       * readers.
       */
      if (shm->wrtrs_num == 0) {
        sem_unlock(sem->read, "writer", id);
      }
    }
    sem_unlock(sem->wrtrs_num, "writer", id);
  }

  
  /* Lock for 'writers alive'. */
  sem_lock(sem->wrtrs_alive, "writer", id);
  { 
    /* Writer is terminating, decreasing value. */
    shm->wrtrs_alive--;
  }
  sem_unlock(sem->wrtrs_alive, "writer", id);

  exit(EXIT_SUCCESS);
}}}


/******************************************************************************
 ~~~[ MAIN FUNCTION ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

int main(int argc, char *argv[])
{{{
  TS_arguments args;                      /* Structure for storing arguments. */

  process_args(argc, argv, &args);        /* Arguments processing. */

  /* Rerouting output stream if output filename was specified. */
  if (args.p_fname != NULL && freopen(args.p_fname, "w", stdout) == NULL) {

    /* Printing error message upon failure and exiting.. */
    fprintf(stderr,"%s: %s: ", argv[0], args.p_fname);
    perror("");

    return EXIT_FAILURE;
  }

  /* Terminating buffering so the output is immediately written. */
  setbuf(stdout, NULL);
  
              


  int shm_fd;                         /* File descriptor for shared memory. */
  TS_shared_mem *shm;                 /* Pointer to shared memory structure. */
 
  /* Try to create and open shared memory. */ 
  if ((shm_fd = shm_open(SHARED_MEM, O_CREAT | O_RDWR, 0600)) < 0) {
    fprintf(stderr, "%s: ", argv[0]);
    perror("");

    return EXIT_FAILURE;
  }
  /* Truncating (extending) memory to size of TS_shared_mem structure. */
  else if (ftruncate(shm_fd, sizeof(TS_shared_mem)) != 0) {
    fprintf(stderr, "%s: ", argv[0]);
    perror("");

    close(shm_fd);
    shm_unlink(SHARED_MEM);

    return EXIT_FAILURE;
  }
  /* Mapping shared memory into this process's virtual address space. */
  else if ((shm = mmap(NULL, sizeof(TS_shared_mem), PROT_READ | PROT_WRITE,
                       MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
    fprintf(stderr, "%s: ", argv[0]);
    perror("");

    close(shm_fd);
    shm_unlink(SHARED_MEM);

    return EXIT_FAILURE;
  }

  TS_semaphores sem;

  if (semaphores_open(&sem) == EXIT_FAILURE) {
    fprintf(stderr, "%s: ", argv[0]);
    perror("");

    close(shm_fd);
    shm_unlink(SHARED_MEM);

    return EXIT_FAILURE;
  }

  /* Shared memory initialization. */
  shm->counter = 1;
  shm->last_writer = -1;

  shm->rdrs_num = 0;
  shm->wrtrs_num = 0;
  shm->wrtrs_alive = 0;




  // TODO: Children creating.

  // TODO: Waiting until children are gone.


  /* Closing and unlinking semaphores. */
  semaphores_close(&sem);
  semaphores_unlink();

  close(shm_fd);                            /* Closing file descriptor. */
  shm_unlink(SHARED_MEM);                   /* Unlinking shared memory. */

  return EXIT_SUCCESS;
}}}


/******************************************************************************
 ***[ END OF READERSWRITERS_MAIN.C ]*******************************************
 ******************************************************************************/
 
