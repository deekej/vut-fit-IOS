/**
 * File:          readersWriters_reader.c
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
 * Description:   Module containing function (process) of reader.
 *
 * More info @:   http://www.fit.vutbr.cz/study/courses/IOS/public/Lab/projekt2/
 *
 * File encoding: en_US.utf8 (United States)
 * Compiler used: gcc 4.5.2 (Ubuntu/Linaro 4.5.2-8ubuntu4)
 */


/******************************************************************************
 ***[ START OF READERSWRITERS_READER.C ]***************************************
 ******************************************************************************/


/******************************************************************************
 ~~~[ HEADER FILES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

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
 * Function (process) of reader. Reads as long as the read value is not zero.
 * Also prints its actions to output.
 */
void reader(TS_shared_mem *p_shm, TS_semaphores *p_sem, unsigned id, unsigned slpt)
{{{
  int last_writer;

  do {
    /* Lock for actions counter (output). */
    sem_lock(p_sem->counter, "reader", id);
    {
      fprintf(stdout, "%d: reader: %u: ready\n", p_shm->counter, id);
      p_shm->counter++;
    }
    sem_unlock(p_sem->counter, "reader", id);

    
    /* Locking of readers queue (writers has absolute priority). */
    sem_lock(p_sem->rdrs_front, "reader", id);
    { 

      /* Lock for reading request. */
      sem_lock(p_sem->read, "reader", id);
      {

        /* Able to read, locking 'readers_num'. */
        sem_lock(p_sem->rdrs_num, "reader", id);
        {
          p_shm->rdrs_num++;            /* Increasing number of readers. */
          
         /*
          * If the actual reader is the first reader, then it locks semaphore of
          * writer so he can't write into the shared memory while readers are
          * reading.
          */
          if (p_shm->rdrs_num == 1) {
            sem_lock(p_sem->write, "reader", id);
          }
        }
        sem_unlock(p_sem->rdrs_num, "reader", id);
      }
      sem_unlock(p_sem->read, "reader", id);
    }
    sem_unlock(p_sem->rdrs_front, "reader", id);

    
    /* Lock for actions counter (output). */
    sem_lock(p_sem->counter, "reader", id);
    {
      fprintf(stdout, "%d: reader: %u: reads a value\n", p_shm->counter, id);
      p_shm->counter++;
    }
    sem_unlock(p_sem->counter, "reader", id);

    
    last_writer = p_shm->last_writer;   /* Value reading. */

    
    /* Lock for actions counter (output). */
    sem_lock(p_sem->counter, "reader", id);
    {
      fprintf(stdout, "%d: reader: %u: read: %d\n", p_shm->counter, id,
              last_writer);
      p_shm->counter++;
    }
    sem_unlock(p_sem->counter, "reader", id);

    
    /* Lock for 'readers number'. */
    sem_lock(p_sem->rdrs_num, "reader", id);
    {
      p_shm->rdrs_num--;                /* Decreasing number of readers. */
      
      /*
       * If the actual process is the last reader, then unlocks the semaphore of
       * writer so he can start writing.
       */
      if (p_shm->rdrs_num == 0) {
        sem_unlock(p_sem->write, "reader", id);
      }
    }
    sem_unlock(p_sem->rdrs_num, "reader", id);


    /* Trying to put process to sleep from 0 to slpt milliseconds. */
    if (slpt != 0 && usleep(1000 * (rand() % slpt)) != 0) {
      fprintf(stderr, "reader: %u: ", id);
      perror("");
      exit(EXIT_FAILURE);
    }

  /* Reads until the read value is 0. */
  } while (last_writer != 0);

  semaphores_close(p_sem);

  exit(EXIT_SUCCESS);
}}}


/******************************************************************************
 ***[ END OF READERSWRITERS_READER.C ]*****************************************
 ******************************************************************************/
 
