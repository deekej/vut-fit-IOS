/**
 * File:          readersWriters.h
 * Version:       0.7
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
 * Description:   This is header file for readersWriters program, containing
 *                some data types declarations, inline functions and functional
 *                prototypes of functions.
 *
 * More info @:   http://www.fit.vutbr.cz/study/courses/IOS/public/Lab/projekt2/
 *
 * File encoding: en_US.utf8 (United States)
 * Compiler used: gcc 4.5.2 (Ubuntu/Linaro 4.5.2-8ubuntu4)
 */


/******************************************************************************
 ***[ START OF READERSWRITERS.H ]**********************************************************
 ******************************************************************************/

/* Safety mechanism against multi-including of this header file. */
#ifndef READERSWRITERS_H
#define READERSWRITERS_H


/******************************************************************************
 ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>


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
#define SEM_RDRS_FRONT    SHM_NAME "_rdrs_front"
#define SEM_RDRS_NUM      SHM_NAME "_rdrs_num"
#define SEM_WRTRS_NUM     SHM_NAME "_wrtrs_num"
#define SEM_COUNTER       SHM_NAME "_counter"
#define SEM_WRTRS_ALIVE   SHM_NAME "_wrtrs_alive"


/******************************************************************************
 ~~~[ DATA TYPES DECLARATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

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
  sem_t *rdrs_front;          /* Semaphore front of readers. */
  sem_t *rdrs_num;            /* Semaphore of shared memory for rdrs_num. */
  sem_t *wrtrs_num;           /* Semaphore of shared memory for wrtrs_num. */
  sem_t *counter;             /* Semaphore of shared memory for act_count. */
  sem_t *wrtrs_alive;         /* Semaphore of shared memory for wrtrs_alive. */
} TS_semaphores;


/******************************************************************************
 ~~~[ GLOBAL FUNCTIONAL PROTOTYPES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

static inline void sem_lock(sem_t *sem, const char *proc, unsigned id);
static inline void sem_unlock(sem_t *sem, const char *proc, unsigned id);

static inline int semaphores_open(TS_semaphores *sem);
static inline void semaphores_close(TS_semaphores *sem);
static inline void semaphores_unlink(void);

void reader(TS_shared_mem *shm, TS_semaphores *sem, unsigned id, unsigned slpt);
void writer(TS_shared_mem *shm, TS_semaphores *sem, unsigned id, unsigned slpt,
            unsigned cycles);


/******************************************************************************
 ~~~[ GLOBAL INLINE FUNCTIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

/**
 * Tries to open all semaphores of given TS_semaphores structure. Returns
 * EXIT_FAILURE and errno is set, otherwise EXIT_SUCCESS is returned.
 */
static inline int semaphores_open(TS_semaphores *sem)
{{{

  /* Opening and initializing semaphores in given semaphores structure. */
  sem->read = sem_open(SEM_READ, O_CREAT, 0600, 1);
  sem->write = sem_open(SEM_WRITE, O_CREAT, 0600, 1);
  sem->rdrs_front = sem_open(SEM_RDRS_FRONT, O_CREAT, 0600, 1);
  sem->rdrs_num = sem_open(SEM_RDRS_NUM, O_CREAT, 0600, 1);
  sem->wrtrs_num = sem_open(SEM_WRTRS_NUM, O_CREAT, 0600, 1);
  sem->counter = sem_open(SEM_COUNTER, O_CREAT, 0600, 1);
  sem->wrtrs_alive = sem_open(SEM_WRTRS_ALIVE, O_CREAT, 0600, 1);

  /* Test of successful opening of semaphores. */
  if (sem->read == SEM_FAILED || sem->write == SEM_FAILED ||
      sem->rdrs_front == SEM_FAILED || sem->rdrs_num == SEM_FAILED ||
      sem->wrtrs_num == SEM_FAILED || sem->counter == SEM_FAILED ||
      sem->wrtrs_alive == SEM_FAILED) {
    
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
static inline void semaphores_close(TS_semaphores *sem)
{{{
  sem_close(sem->read);
  sem_close(sem->write);
  sem_close(sem->rdrs_front);
  sem_close(sem->rdrs_num);
  sem_close(sem->wrtrs_num);
  sem_close(sem->counter);
  sem_close(sem->wrtrs_alive);

  return;
}}}


/**
 * Tries to unlink all semaphores of this program.
 */
static inline void semaphores_unlink(void)
{{{
  sem_unlink(SEM_READ);
  sem_unlink(SEM_WRITE);
  sem_unlink(SEM_RDRS_FRONT);
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

    sem_close(sem);
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

    sem_close(sem);
    exit(EXIT_FAILURE);
  }
  
  return;
}}}


/******************************************************************************
 ***[ END OF READERSWRITERS.H ]************************************************
 ******************************************************************************/

#endif
 
