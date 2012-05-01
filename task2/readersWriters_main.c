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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


/******************************************************************************
 ~~~[ GLOBAL CONSTANTS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

#define SHM_NAME "/xkaspa34_shm"

const int ARGS_NUM = 6;                   /* 6 arguments required. */


/******************************************************************************
 ~~~[ DATA TYPES DECLARATIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

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
  int act_count;                          /* Actions counter. */
  int last_writer;                        /* Internal number of last writer. */

  unsigned wrtrs_num;                     /* Number of writers writing. */
  unsigned rdrs_num;                      /* Number of readers reading. */

  unsigned wrtrs_alive;                   /* Number of writers alive. */
 } TS_shared_mem;


/******************************************************************************
 ~~~[ FUNCTIONAL PROTOTYPES ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/

void process_args(int argc, char *argv[], TS_arguments *p_args);
static inline void display_usage(char *prg_name);


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


/******************************************************************************
 ~~~[ PRIMARY FUNCTIONS ]~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 ******************************************************************************/


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

  int shm_fd;                         /* File descriptor for shared memory. */
 
  /* Try to create and open shared memory. */ 
  if ((shm_fd = shm_open(SHM_NAME, O_RDWR|O_CREAT, 0600)) < 0) {
    fprintf(stderr, "%s: ", argv[0]);
    perror("");
    return EXIT_FAILURE;
  }

  /* Truncating (extending) memory to size of TS_shared_mem structure. */
  if (ftruncate(shm_fd, sizeof(TS_shared_mem)) != 0) {
    fprintf(stderr, "%s: ", argv[0]);
    perror("");
    return EXIT_FAILURE;
  }

  /* Mapping shared memory into this process's virtual address space. */
  TS_shared_mem *shm = mmap(NULL, sizeof(TS_shared_mem), PROT_READ | PROT_WRITE,
                            MAP_SHARED, shm_fd, 0);

  /* Test of successful mapping. */
  if (shm == MAP_FAILED) {
    fprintf(stderr, "%s: ", argv[0]);
    perror("");
    return EXIT_FAILURE;
  }
  
  // TODO: Children creating.

  // TODO: Waiting until children are gone.

  close(shm_fd);                      /* Closing file descriptor. */
  shm_unlink(SHM_NAME);               /* Unlinking shared memory. */

  return EXIT_SUCCESS;
}}}


/******************************************************************************
 ***[ END OF READERSWRITERS_MAIN.C ]*******************************************
 ******************************************************************************/
 
