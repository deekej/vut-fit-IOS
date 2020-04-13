#!/bin/bash

################################################################################
# File:           runtests.sh
# Version:        1.0
# Date:           18-03-2012
# Last update:    11-04-2012
#
# Course:         IOS (summer semester, 2012)
# Project:        Project #1
#
# Author:         David Kaspar (aka Dee'Kej), 1BIT
# Faculty:        Faculty of Information Technology,
#                 Brno University of Technology
# E-mail:         xkaspa34@stud.fit.vutbr.cz
#
# Description:    Script working with set of regress tests. Can be used for
#                 test driven development or just testing of behaviour of 
#                 some program. For more info see the link below.
#
# More info @:
#   http://www.fit.vutbr.cz/study/courses/IOS/public/Lab/projekt1/projekt1.html
#
# File encoding:  7-bit ASCII
# EOL marker:     LF ('\n' == 0x0A) 
################################################################################


################################################################################
### START OF THE SCRIPT
################################################################################

export LC_ALL=C


################################################################################
### "GLOBAL" VARIABLES
################################################################################

EXIT_STATUS=0         # Global exit status of the script.

# Global variables for indication of action to be performed:
OPT_VALID='false'
OPT_TEST='false'
OPT_REPORT='false'
OPT_SYNC='false'
OPT_CLEAR='false'

TEST_DIR=             # Directory which the script should work with.
REGEX=                # Extended regular expression specifying the directory.

SCRIPT=$0             # Assigning script name to global variable 'cause function
                      # in ksh doesn't see the $0 variable


################################################################################
### FUNCTIONS DEFINITIONS
################################################################################

# ########################################################################
# Helpful function for detecting files of given type. (Files to be usually
# tested are: block, character, pipeline, socket.)
# ##########################
function validate_unexpected
{
  for file_type in $(find . -type "$1" | grep -E -e "$REGEX" | sort); do
    echo "$SCRIPT: warning: unexpected behaviour may occur because of:" >&2
    echo "$SCRIPT: $2 file has been found: $file_type" >&2
    EXIT_STATUS=1
  done
}


# #####################################################################
# Helpful function that validates content of current working directory.
# ###################
function validate_dir
{
  # All other directories of actual directory.
  typeset other_dirs=$(find . -maxdepth 1 -type d)

  # All regular files of actual directory.
  typeset dir_files=$(find . -maxdepth 1 -type f)
  

  # Test for mixed files with directories:
  if [[ "$other_dirs" != "." && -n "$dir_files" ]]; then
    echo "$SCRIPT: warning: directories and other files are mixed in: $dir" >&2
    EXIT_STATUS=1
  elif [[ "$other_dirs" == "." ]]; then

    # No other directories here, testing of 'cmd-given' file:
    if [[ ! -e cmd-given ]]; then
      echo "$SCRIPT: warning: 'cmd-given' file does not exist in: $dir" >&2
      EXIT_STATUS=1
    else
      if [[ ! -r cmd-given ]]; then
        echo "$SCRIPT: warning: 'cmd-given' file is not marked as readable in: $dir" >&2
        EXIT_STATUS=1
      fi

      if [[ ! -x cmd-given ]]; then
        echo "$SCRIPT: warning: 'cmd-given' file is not marked as executable in: $dir" >&2
        EXIT_STATUS=1
      fi
    fi
  fi


  # Testing every file in actual directory:
  for file in $dir_files; do
    
    # Stripping to filename only, then testing:
    case "$(basename "$file")" in
      stdin-given)  test -r "$file" || {
                      echo "$SCRIPT: warning: 'stdin-given' file is not marked as readable in: $dir" >&2;
                      EXIT_STATUS=1;
                    } ;;
      
      stdout-delta | stdout-expected | stdout-captured |\
      stderr-delta | stderr-expected | stderr-captured |\
      status-delta) test -w "$file" || {
                      echo "$SCRIPT: warning: '$(basename "$file")' file is not marked as writeable in: $dir" >&2;
                      EXIT_STATUS=1;
                    } ;;

      status-expected |\
      status-captured)  test -w "$file" || {
                          echo "$SCRIPT: warning: '$(basename "$file")' file is not marked as writeable in: $dir" >&2;
                          EXIT_STATUS=1;
                        }
                        
                        # Tests the content of the file for an integer number followed by \n character:
                        if [[ ! -z "$(grep -E -v -e ^[[:digit:]]+$ "$file")" ]]; then
                          echo "$SCRIPT: warning: '$(basename "$file")' file does not contain valid integer number in: $dir" >&2
                          EXIT_STATUS=1
                        fi

                        if [[ $(wc -l <"$file") -gt 1 ]]; then
                          echo "$SCRIPT: warning: '$(basename "$file")' file contains extra line(s) in: $dir" >&2
                          EXIT_STATUS=1
                        fi;;
      
            # 'cmd-given' file has been already tested above.
            cmd-given)
                        ;;
                    
                    # Not "allowed" file:
                    *)  echo "$SCRIPT: warning: '$(basename "$file")' stray file has been found in: $dir" >&2
                        EXIT_STATUS=1
                        ;;
    esac

  done
}


# ########################################################################################
# Wrapper function for complete validation of the directory tree determined by the $REGEX.
# ####################
function validate_tree
{
  typeset IFS_backup=$IFS           # Backup of actual IFS for later restoration.
  IFS=$'\n'                         # New IFS for line operations below.

  
  # Testing for unexpected files:
  validate_unexpected "b" "block (buffered) special"
  validate_unexpected "c" "character (unbuffered) special"
  validate_unexpected "p" "named pipe (FIFO)"
  validate_unexpected "s" "socket"

  
  # Testing for not allowed symbolic and multi hard links:
  for sym_link in $(find . -type l | grep -E -e "$REGEX" | sort); do
    echo "$SCRIPT: warning: symbolic link has been found: $sym_link" >&2
    EXIT_STATUS=1
  done

  for hard_link in $(find . -type f -links +1 | grep -E -e "$REGEX" | sort); do
    echo "$SCRIPT: warning: more than 1 hard link of file has been found for: $hard_link" >&2
    EXIT_STATUS=1
  done


  #typeset default_dir="$(pwd)"      # Backup of actual working directory for return below.


  # Validating every directory matching the regular expression:
  for dir in $(find . -type d | grep -E -e "$REGEX" | sort); do

    # Changing to new directory of the tree for validation.
    cd "$dir" 2>/dev/null || {
      echo "$SCRIPT: warning: skipping over not accessible directory: $dir" >&2;
      EXIT_STATUS=1;
      continue;
    }

    validate_dir
    
    # Return from the directory so the testing can continue.
    cd "$OLDPWD" 2>/dev/null || {
      echo "$SCRIPT: error: failed to return to directory: $OLDPWD" >&2
      exit 2;
    }

  done

  IFS=$IFS_backup                   # Restoring IFS.
}


################################################################################

# ##############################################################################################
# Processes the results of tests and prints them to stdout or stderr, depending on 1st argument,
# in actual directory. 2nd argument represent the path to actual directory's cmd-given file.
# ######################
function display_results
{
  typeset actual_path=$(dirname "$2")     # Stripping the 'cmd-given' from path.
  typeset result="OK"                     # Initializing the result.

  
  # Getting the results of tests:
  for file in {stdout,stderr,status}; do
    diff -up "$file-expected" "$file-captured" >"$file-delta" 2>/dev/null
    
    # Testing return value of diff, if any file was missing:
    if [[ $? -gt 1 ]]; then
      echo "$SCRIPT: warning: '$file-expected' and '$file-captured' lines difference could not be obtain in: $actual_path" >&2
      result="FAILED"
      EXIT_STATUS=1
    # Not empty X-delta file means FAILED test:
    elif [[ -s "$file-delta" ]]; then
      result="FAILED"
      EXIT_STATUS=1
    fi
  done

  
  # Test for colored output to stderr or stdout:
  if [[ "$1" == "run_tests" && -t 2 ]] || [[ "$1" == "report_results" && -t 1 ]]; then
    if [[ "$result" == "OK" ]]; then
      result="\033[1;32mOK\033[0m"        # Setting to light green.
    else
      result="\033[1;31mFAILED\033[0m"    # Setting to light red.
    fi
  fi

  
  # Output destination depends on calling function (1st argument of actual function):
  if [[ "$1" == "run_tests" ]]; then
    echo -e "${actual_path#./}: $result" >&2
  else
    echo -e "${actual_path#./}: $result"
  fi
}


# ##############################################################################################
# Runs every 'cmd-given' file filtered by REGEX and captures it stdout, stderr and return value
# to appropriate files. Then runs function display_results to get the differences and displaying
# them.
# ################
function run_tests
{
  typeset IFS_backup=$IFS           # Backup of actual IFS for later restoration.
  IFS=$'\n'                         # New IFS for line operations below.

  #typeset default_dir="$(pwd)"      # Backup of actual working directory for return below.
  typeset file_input                # Input of 'cmd-given' file.


  for file_path in $(find . -type f -name cmd-given | grep -E -e "$REGEX" | sort); do
    
    # Change of actual directory to where the 'cmd-given' file is located:
    cd "$(dirname "$file_path")" 2>/dev/null || {
      echo "$SCRIPT: warning: skipping over not accessible directory: $(dirname "$file_path")" >&2;
      EXIT_STATUS=1;
      continue;
    }
    
    # Use stdin-given if it exists for input, otherwise use the /dev/null
    if [[ -f stdin-given ]]; then
      file_input="stdin-given"
    else
      file_input="/dev/null"
    fi
    
    # Run the 'cmd-given' file and capture its results:
    {
      ./cmd-given <"$file_input" >stdout-captured 2>stderr-captured;
      echo "$?" >'status-captured';
    }
    
    # Calling the display_results function for use within this function:
    display_results "run_tests" "$file_path"
    
    # Return to default directory so the cycle can continue:
    cd "$OLDPWD" 2>/dev/null || {
      echo "$SCRIPT: error: failed to return to directory: $OLDPWD" >&2;
      exit 2;
    }

  done


  IFS=$IFS_backup                   # Restoring IFS.
}


# ###########################################################################################
# For every location of 'cmd-given' file filtered by REGEX displays the results of the tests.
# #####################
function report_results
{
  typeset IFS_backup=$IFS           # Backup of actual IFS for later restoration.
  IFS=$'\n'                         # New IFS for line operations below.

  #typeset default_dir="$(pwd)"      # Backup of actual working directory for return below.


  for file_path in $(find . -type f -name cmd-given | grep -E -e "$REGEX" | sort); do

    # Change of actual directory to where the 'cmd-given' file is located:
    cd "$(dirname "$file_path")" 2>/dev/null || {
      echo "$SCRIPT: warning: skipping over not accessible directory: $(dirname "$file_path")" >&2;
      EXIT_STATUS=1;
      continue;
    }
    
    # Calling the display_results function for use within this function:
    display_results "report_results" "$file_path"
    
    # Return to default directory so the cycle can continue:
    cd "$OLDPWD" 2>/dev/null || {
      echo "$SCRIPT: error: failed to return to directory: $OLDPWD" >&2;
      exit 2;
    }

  done


  IFS=$IFS_backup                   # Restoring IFS.
}


################################################################################

# #############################################################################################
# Every {stdout,stderr,status}-captured file from the given tree (filtered by REGEX) is renamed
# to {stdout,stderr,status}-expected. Existing files are overwritten. In other words, this
# function synchronizes actual results as wanted results for further testing.
# ###################
function sync_results
{
  typeset IFS_backup=$IFS           # Backup of actual IFS for later restoration.
  IFS=$'\n'                         # New IFS for line operations below.

  #typeset default_dir="$(pwd)"      # Backup of actual working directory for return below.


  # Going through every filtered directory:
  for dir in $(find . -type d | grep -E -e "$REGEX" | sort); do
    
    # Change of actual directory to next directory for processing:
    cd "$dir" 2>/dev/null || {
      echo "$SCRIPT: warning: skipping over not accessible directory: $dir" >&2;
      EXIT_STATUS=1;
      continue;
    }
    
    for file in {stdout,stderr,status}; do
      if [[ ! -f "$file-captured" ]]; then
        continue                                  # File does not exists, continue to next.
      else
        mv -f "$file-captured" "$file-expected"   # File exists, proceeds with renaming.
      fi

      # Successful renaming?
      if [[ $? -gt 0 ]]; then
        echo "$SCRIPT: warning: synchronizing of results has failed in: $dir" >&2
        EXIT_STATUS=1;
      fi
    done

    # Return to default directory so the cycle can continue:
    cd "$OLDPWD" 2>/dev/null || {
      echo "$SCRIPT: error: failed to return to directory: $OLDPWD" >&2
      exit 2;
    }

  done


  IFS=$IFS_backup                   # Restoring IFS.
}


################################################################################

# #####################################################################
# Removes every {stdout,stderr,status}-{captured,delta} from given tree
# (filtered by REGEX).
# ##################
function clear_files
{
  typeset IFS_backup=$IFS           # Backup of actual IFS for later restoration.
  IFS=$'\n'                         # New IFS for line operations below.

  #typeset default_dir="$(pwd)"      # Backup of actual working directory for return below.


  # Going through every filtered directory:
  for dir in $(find . -type d | grep -E -e "$REGEX" | sort); do

    # Change of actual directory to next directory for processing:
    cd "$dir" 2>/dev/null || {
      echo "$SCRIPT: warning: skipping over not accessible directory: $dir" >&2;
      EXIT_STATUS=1;
      continue;
    }

    for file in {stdout,stderr,status}-{captured,delta}; do
      rm -f "$file"
      
      # Successful removing of the file?
      if [[ $? -gt 0 ]]; then
        echo "$SCRIPT: warning: '$file' could not be removed in: $dir" >&2
        EXIT_STATUS=1
      fi
    done
    
    # Return to default directory so the cycle can continue:
    cd "$OLDPWD" 2>/dev/null || {
      echo "$SCRIPT: error: failed to return to directory: $OLDPWD" >&2
      exit 2;
    }

  done


  IFS=$IFS_backup                   # Restoring IFS.
}


################################################################################

# ##########################
# Displays the help message.
# ###################
function display_help
{
  printf "Usage: $SCRIPT [-vtrsc] TEST_DIR [REGEX]

    -v  validate tree
    -t  run tests
    -r  report results
    -s  synchronize expected results
    -c  clear generated files

    It is mandatory to supply at least one option.\n"
}


# ###########################################################
# Processes the parameters and arguments of invoked scripted.
# ##################
function get_options
{
  # Local variable to determine if at least 1 parameter was used.
  typeset opt_used='false'

  # Optional parameters processing.
  while getopts :hvtrsc opt; do
    case $opt in
      v) opt_used='true'; OPT_VALID='true';;
      t) opt_used='true'; OPT_TEST='true';;
      r) opt_used='true'; OPT_REPORT='true';;
      s) opt_used='true'; OPT_SYNC='true';;
      c) opt_used='true'; OPT_CLEAR='true';;
      h) display_help; exit 0;;
      ?) display_help >&2; exit 2;;
    esac
  done

  # At least one option is required.
  if [[ "$opt_used" == 'false' ]]; then
    display_help >&2
    exit 2
  fi

  # Dropping the processed parameters, so the rest of arguments can be processed.
  ((OPTIND--))
  shift $OPTIND


  # Testing for required argument of testing directory. There can be one optional
  # argument representing extended regular expression to be applied upon the
  # directory tree. No other arguments are allowed!
  if [[ -d "$1" && -z "$3" ]]; then
    TEST_DIR=$1
    REGEX=$2
  else
    display_help >&2
    exit 2
  fi
}


################################################################################
### EXECUTION PART (MAIN)
################################################################################

# Processing the scripts arguments:
get_options "$@"

# Changing requested working directory, exits upon failure:
cd "$TEST_DIR" 2>/dev/null || {
  display_help >&2;
  exit 2;
}

# Calling requested procedures at required sequence:
if [[ "$OPT_VALID" == 'true' ]]; then
  validate_tree
fi

if [[ "$OPT_TEST" == 'true' ]]; then
  run_tests
fi

if [[ "$OPT_REPORT" == 'true' ]]; then
  report_results
fi

if [[ "$OPT_SYNC" == 'true' ]]; then
  sync_results
fi

if [[ "$OPT_CLEAR" == 'true' ]]; then
  clear_files
fi

exit $EXIT_STATUS

################################################################################
### END OF THE SCRIPT
################################################################################
 
