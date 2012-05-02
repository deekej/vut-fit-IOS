/**
 * File:          readersWriters_main.c
 * Version:       0.9
 * Date:          29-04-2012
 * Last update:   02-05-2012
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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>

#include "readersWriters.h"


/******************************************************************************
 ~~~[ GLOBAL CONSTANTS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

const int ARGS_NUM = 6;                   /* 6 arguments required. */


/******************************************************************************
 ~~~[ DATA TYPES DECLARATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

/* Structure for storing processed arguments of invocated program.  */
 typedef struct arguments {
  unsigned wrtrs_num;                     /* Number of writers. */
  unsigned wrtrs_slpt;                    /* Sleep time of writers [ms]. */

  unsigned rdrs_num;                      /* Number of readers. */
  unsigned rdrs_slpt;                     /* Sleep time of writers [ms]. */

  unsigned cycles;                        /* Number of cycles. */

  char *p_fname;                          /* Output filename. */
 } TS_arguments;


/******************************************************************************
 ~~~[ FUNCTIONAL PROTOTYPES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

void process_args(int argc, char *argv[], TS_arguments *p_args);
static inline void display_usage(char *prg_name);


void create_children(TS_shared_mem *p_shm, TS_semaphores *p_sem,
                     TS_arguments *p_args, int shm_fd, const char *prg_name);

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
        p_args->wrtrs_num = (unsigned) result;
        break;

      case 2 :
        p_args->rdrs_num = (unsigned) result;
        break;

      case 3 :
        p_args->cycles = (unsigned) result;
        break;

      case 4 :
        p_args->wrtrs_slpt = (unsigned) result;
        break;

      case 5 :
        p_args->rdrs_slpt = (unsigned) result;
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
  
  /* Test if at least 1 writer and 1 reader was entered. */
  if (p_args->wrtrs_num < 1 || p_args->rdrs_num < 1) {
    fprintf(stderr, "%s: at least 1 writer and 1 reader is required\n",
            argv[0]);
    display_usage(argv[0]);
    exit(EXIT_FAILURE);
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


/******************************************************************************
 ~~~[ PRIMARY FUNCTIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

/**
 * Function for creating all of children processes (forking).
 */
void create_children(TS_shared_mem *p_shm, TS_semaphores *p_sem,
                     TS_arguments *p_args, int shm_fd, const char *prg_name)
{{{
  pid_t pid;                    /* PID of last forked process. */
  pid_t pgid = 0;               /* PGID of children processes. */


  /* Creating specified number of writers. */
  for (unsigned i = 1; i <= p_args->wrtrs_num; i++) {
    pid = fork();

    if (pid == 0) {
      /* Children process, acting as a writer. */
      writer(p_shm, p_sem, i, p_args->wrtrs_slpt, p_args->cycles);
    }
    else if (pid < 0) {
      /* Error occurred, terminating all children and sweeping. */
      fprintf(stderr, "%s: ", prg_name);
      perror("");
      fprintf(stderr, "%s: terminating all children processes\n", prg_name);

      killpg(pgid, SIGTERM);

      /* Closing and unlinking semaphores. */
      semaphores_close(p_sem);
      semaphores_unlink();

      close(shm_fd);                        /* Closing file descriptor. */
      shm_unlink(SHARED_MEM);               /* Unlinking shared memory. */

      exit(EXIT_FAILURE);
    }
    else if (i == 1) {
      /* First forked process is the group leader. */
      setpgid(pid, 0);
      pgid = pid;
    }
    else {
      setpgid(pid, pgid);                   /* Adding process to group. */
    }
  }

  
  /* Creating specified number of readers. */
  for (unsigned i = 1; i <= p_args->rdrs_num; i++) {
    pid = fork();

    if (pid == 0) {
      /* Children process, acting as a reader. */
      reader(p_shm, p_sem, i, p_args->rdrs_slpt);
    }
    else if (pid < 0) {
      /* Error occurred, terminating all children and sweeping. */
      fprintf(stderr, "%s: ", prg_name);
      perror("");
      fprintf(stderr, "%s: terminating all children processes\n", prg_name);

      killpg(pgid, SIGTERM);

      /* Closing and unlinking semaphores. */
      semaphores_close(p_sem);
      semaphores_unlink();

      close(shm_fd);                        /* Closing file descriptor. */
      shm_unlink(SHARED_MEM);               /* Unlinking shared memory. */

      exit(EXIT_FAILURE);
    }
    else {
      setpgid(pid, pgid);                   /* Adding process to group. */
    }
  }  

  return;
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


  /*
   * Bigger part taking care of creating and opening shared memory for
   * inter-process communication.
   */
  int shm_fd;                         /* File descriptor for shared memory. */
  TS_shared_mem *p_shm;               /* Pointer to shared memory structure. */
 
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
  else if ((p_shm = mmap(NULL, sizeof(TS_shared_mem), PROT_READ | PROT_WRITE,
                       MAP_SHARED, shm_fd, 0)) == MAP_FAILED) {
    fprintf(stderr, "%s: ", argv[0]);
    perror("");

    close(shm_fd);
    shm_unlink(SHARED_MEM);

    return EXIT_FAILURE;
  }


  /* Shared memory initialization. */
  p_shm->counter = 1;
  p_shm->last_writer = -1;

  p_shm->rdrs_num = 0;
  p_shm->wrtrs_num = 0;
  p_shm->wrtrs_alive = 0;


  /*
   * Creating and opening semaphores for synchronizing processes.
   */
  TS_semaphores sem;

  if (semaphores_open(&sem) == EXIT_FAILURE) {
    fprintf(stderr, "%s: ", argv[0]);
    perror("");

    close(shm_fd);
    shm_unlink(SHARED_MEM);

    return EXIT_FAILURE;
  }


  /* Creating requested number of children. */
  create_children(p_shm, &sem, &args, shm_fd, argv[0]);

  /*
   * Waiting until all writers are finished, then writing 0 to shared memory and
   * waiting for readers to finish.
   */
  pid_t pid;

  do {
    pid = wait(NULL);

    sem_lock(sem.wrtrs_alive, "main", 0);

    if (p_shm->wrtrs_alive == 0) {
      sem_lock(sem.read, "main", 0);
      sem_lock(sem.write, "main", 0);

      p_shm->last_writer = 0;

      sem_unlock(sem.write, "main", 0);
      sem_unlock(sem.read, "main", 0);
    }

    sem_unlock(sem.wrtrs_alive, "main", 0);

  } while (pid != -1);


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
 
