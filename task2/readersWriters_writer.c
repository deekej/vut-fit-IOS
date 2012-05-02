/**
 * File:          readersWriters_writer.c
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
 * Description:   Module containing function (process) of writer.
 *
 * More info @:   http://www.fit.vutbr.cz/study/courses/IOS/public/Lab/projekt2/
 *
 * File encoding: en_US.utf8 (United States)
 * Compiler used: gcc 4.5.2 (Ubuntu/Linaro 4.5.2-8ubuntu4)
 */


/******************************************************************************
 ***[ START OF READERSWRITERS_WRITER.C ]***************************************
 ******************************************************************************/


/******************************************************************************
 ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#include "readersWriters.h"


/******************************************************************************
 ~~~[ PRIMARY FUNCTION ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

/**
 * Function (process) of writer. Writes the n-times into shared memory, the n is
 * specified by given argument cycles. Also prints its actions to output.
 */
void writer(TS_shared_mem *p_shm, TS_semaphores *p_sem, unsigned id,
            unsigned slpt, unsigned cycles)
{{{
  /* Lock for 'writers alive'. */
  sem_lock(p_sem->wrtrs_alive, "writer", id);
  {
    /* 
     * Increasing value, so the main can determine how many writers are still 
     * alive.
     */
    p_shm->wrtrs_alive++;
  }
  sem_unlock(p_sem->wrtrs_alive, "writer", id);


  srand((unsigned int) time(NULL));         /* Getting new seed for rand(). */

  
  /* Repeats for number specified in cycles. */
  for (unsigned i = 0; i < cycles; i++) {

    /* Lock for actions counter (output). */
    sem_lock(p_sem->counter, "writer", id);
    {
      fprintf(stdout, "%d: writer: %u: new value\n", p_shm->counter, id);
      p_shm->counter++;
    }
    sem_unlock(p_sem->counter, "writer", id);


    /* Trying to put process to sleep from 0 to slpt milliseconds. */
    if (slpt != 0 && usleep(1000 * (rand() % slpt)) != 0) {
      fprintf(stderr, "writer: %u: ", id);
      perror("");
      exit(EXIT_FAILURE);
    }


    /* Lock for actions counter (output). */
    sem_lock(p_sem->counter, "writer", id);
    {
      fprintf(stdout, "%d: writer: %u: ready\n", p_shm->counter, id);
      p_shm->counter++;
    }
    sem_unlock(p_sem->counter, "writer", id);

    
    /* Lock for number of actual writers writing. */
    sem_lock(p_sem->wrtrs_num, "writer", id);
    {
      p_shm->wrtrs_num++;
      
      /*
       * First writer also locks 'read' semaphore, so the new coming readers
       * can't access the shared memory for the time of writers performing
       * actions.
       */
      if (p_shm->wrtrs_num == 1) {
        sem_lock(p_sem->read, "writer", id);
      }
    }
    sem_unlock(p_sem->wrtrs_num, "writer", id);


    /* 
     * Lock for writer's semaphore, allowing only one writer at the time to edit
     * shared memory.
     */
    sem_lock(p_sem->write, "writer", id);
    {
      /* Lock for actions counter (output). */
      sem_lock(p_sem->counter, "writer", id);
      {
        fprintf(stdout, "%d: writer: %u: writes a value\n", p_shm->counter, id);
        p_shm->counter++;
      }
      sem_unlock(p_sem->counter, "writer", id);

      
      p_shm->last_writer = id;            /* Writer is writing his value. */

      
      /* Lock for actions counter (output). */
      sem_lock(p_sem->counter, "writer", id);
      {
        fprintf(stdout, "%d: writer: %u: written\n", p_shm->counter, id);
        p_shm->counter++;
      }
      sem_unlock(p_sem->counter, "writer", id);
    }
    sem_unlock(p_sem->write, "writer", id);

  
    /* Lock for number of actual writers writing. */
    sem_lock(p_sem->wrtrs_num, "writer", id);
    {
      p_shm->wrtrs_num--;
      
      /*
       * If the actual writer is the last one, then he opens semaphore for
       * readers.
       */
      if (p_shm->wrtrs_num == 0) {
        sem_unlock(p_sem->read, "writer", id);
      }
    }
    sem_unlock(p_sem->wrtrs_num, "writer", id);
  }

  
  /* Lock for 'writers alive'. */
  sem_lock(p_sem->wrtrs_alive, "writer", id);
  { 
    /* Writer is terminating, decreasing value. */
    p_shm->wrtrs_alive--;
  }
  sem_unlock(p_sem->wrtrs_alive, "writer", id);

  semaphores_close(p_sem);

  exit(EXIT_SUCCESS);
}}}


/******************************************************************************
 ***[ END OF READERSWRITERS_WRITER.C ]*****************************************
 ******************************************************************************/
 
