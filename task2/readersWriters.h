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

void reader(TS_shared_mem *shm, TS_semaphores *sem, unsigned id, unsigned slpt);
void writer(TS_shared_mem *shm, TS_semaphores *sem, unsigned id, unsigned slpt,
            unsigned cycles);


/******************************************************************************
 ~~~[ GLOBAL INLINE FUNCTIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

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
 ***[ END OF READERSWRITERS.H ]************************************************
 ******************************************************************************/

#endif
 
