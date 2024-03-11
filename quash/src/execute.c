/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#define _GNU_SOURCE
#include "execute.h"
#include "command.h"
#include "signal.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "quash.h"
#include "deque.h"

#define WRITE_END 1
#define READ_END 0

// define process queue
IMPLEMENT_DEQUE_STRUCT(process_queue, pid_t);
IMPLEMENT_DEQUE(process_queue, pid_t);
process_queue p_q;

// job structure
struct Job
{
  int jobID;
  char *cmd;
  process_queue p_q;
  pid_t pid;
} Job;

// creating job queue
int num_job = 1;
IMPLEMENT_DEQUE_STRUCT(job_queue, struct Job);
IMPLEMENT_DEQUE(job_queue, struct Job);
job_queue j_q;

int pipes[3];

bool on = 0;

// Remove this and all expansion calls to it
/**
 * @brief Note calls to any function that requires implementation
 */
// #define IMPLEMENT_ME()                                                  
  // fprintf(stderr, "IMPLEMENT ME: %s(line %d): %s()\n", __FILE__, __LINE__, __FUNCTION__)

/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  // TODO: Get the current working directory. This will fix the prompt path.
  // HINT: This should be pretty simple
  // IMPLEMENT_ME();

  // Change this to true if necessary
  *should_free = true;

  char* w_d = getcwd(NULL, 1024);
  return w_d;
}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  // TODO: Lookup environment variables. This is required for parser to be able
  // to interpret variables from the command line and display the prompt
  // correctly
  // HINT: This should be pretty simple
  // IMPLEMENT_ME();

  // TODO: Remove warning silencers
  //(void) env_var; // Silence unused variable warning

  return getenv(env_var);
}

// Check the status of background jobs
void check_jobs_bg_status() {
  // TODO: Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.
  // IMPLEMENT_ME();

  

  // TODO: Once jobs are implemented, uncomment and fill the following line
  // print_job_bg_complete(job_id, pid, cmd)

  if (num_job == 0)
  {
    printf("No Jobs found...leaving...");
    return;
  }

  int num_job2 = length_job_queue(&j_q);

  // iterate over the number of jobs present
  // if jobs actually exist
  for (int i = 0; i < num_job2; i++)
  {
    struct Job current_job = pop_front_job_queue(&j_q);

    int pid_num = length_process_queue(&current_job.p_q);
    pid_t queue_front = peek_front_process_queue(&current_job.p_q);

    for (int j = 0; j < pid_num; j++)
    {
      pid_t current_pid = pop_front_process_queue(&current_job.p_q);
      int p_status;

      if (waitpid(current_pid, &p_status, 1) == 0)
        push_back_process_queue(&current_job.p_q, current_pid);
    }

    if (is_empty_process_queue(&current_job.p_q))
    {
      print_job_bg_complete(current_job.jobID, queue_front, current_job.cmd);
    }

    else
    {
      push_back_job_queue(&j_q, current_job);
    }
  }
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
 * Functions to process commands
 ***************************************************************************/
// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  // Execute a program with a list of arguments. The `args` array is a NULL
  // terminated (last string is always NULL) list of strings. The first element
  // in the array is the executable
  char* exec = cmd.args[0];
  char** args = cmd.args;

  // TODO: Remove warning silencers
  //(void) exec; // Silence unused variable warning
  //(void) args; // Silence unused variable warning

  // TODO: Implement run generic
  //IMPLEMENT_ME();
  execvp(exec, args);

  perror("ERROR: Failed to execute program");
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;

  // TODO: Remove warning silencers
  // (void) str; // Silence unused variable warning

  // TODO: Implement echo
  // IMPLEMENT_ME();
  for (int j = 0; str[j] != 0; j++)
  {
    printf("%s ",str[j]);
  }
  printf("\n");
  // Flush the buffer before returning
  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;

  // TODO: Remove warning silencers
  // (void) env_var; // Silence unused variable warning
  // (void) val;     // Silence unused variable warning

  // TODO: Implement export.
  // HINT: This should be quite simple.
  // IMPLEMENT_ME();
  setenv(env_var, val, 1);
}

// Changes the current working directory
void run_cd(CDCommand cmd) {
  // Get the directory name
  const char* dir = cmd.dir;

  // Check if the directory is valid
  if (dir == NULL) {
    perror("ERROR: Failed to resolve path");
    return;
  }

  // TODO: Change directory

  // TODO: Update the PWD environment variable to be the new current working
  // directory and optionally update OLD_PWD environment variable to be the old
  // working directory.
  // IMPLEMENT_ME();

  // change to the old directory
  // if (cmd.dir == -1)
  // {
  //   perror("ERROR: ");
  //   return;
  // }

  // if (setenv("PWD", cmd.dir, 1) == -1)
  // {
  //   perror("ERROR: ");
  //   return;
  // }

  // if (setenv("OLDPWD", lookup_env("PWD"), 1) == -1)
  // {
  //   perror("ERROR: ");
  //   return;
  // }

  bool should_free = false;
  char* og_directory = get_current_directory(&should_free);

  if (chdir(dir) != 0) {
    perror("Error changing directory");
    // Handle the error, maybe exit or return an error code
  }
  char* new_directory = get_current_directory(&should_free);
  setenv("OLD_PWD", og_directory, 1);
  setenv("PWD", new_directory, 1);

  free(og_directory);
  free(new_directory);

}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  // TODO: Remove warning silencers
  // (void) signal; // Silence unused variable warning
  // (void) job_id; // Silence unused variable warning

  // TODO: Kill all processes associated with a background job
  // IMPLEMENT_ME();

  struct Job current_job;

  // iterate of length of job q
  for (int j = 0; j < length_job_queue(&j_q); j++)
  {
    current_job = pop_front_job_queue(&j_q);

    if (current_job.jobID == job_id)
    {
      process_queue current_p_q = current_job.p_q;

      while (length_process_queue(&current_p_q) != 0)
      {
        pid_t current_pid = pop_front_process_queue(&current_p_q);
        // kill the job process
        kill(current_pid, signal);
      }
    }

    push_back_job_queue(&j_q, current_job);
  }
}


// Prints the current working directory to stdout
void run_pwd() {
  // TODO: Print the current working directory
  // char *buf = get_current_dir_name() ;
  // printf("%s\n",buf);
  // free(buf);
  // IMPLEMENT_ME();

  bool should_free;
  char* current_directory = get_current_directory(&should_free);

  printf("%s\n", current_directory);

  // Free the allocated memory if needed
  if (should_free) {
      free(current_directory);
  }

  // Flush the buffer before returning
  fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  // TODO: Print background jobs
  // IMPLEMENT_ME();

  // int number = length_job_queue(&j_q);

  // iterate over jobs
  for (int i = 0; i < length_job_queue(&j_q) ; i++)
  {
    struct Job current_job = pop_front_job_queue(&j_q);
    // printing job
    print_job(current_job.jobID, current_job.pid, current_job.cmd);
    push_back_job_queue(&j_q, current_job);
  }

  // Flush the buffer before returning
  fflush(stdout);
}

/***************************************************************************
 * Functions for command resolution and process setup
 ***************************************************************************/

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for child processes.
 *
 * This version of the function is tailored to commands that should be run in
 * the child process of a fork.
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void child_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case GENERIC:
    run_generic(cmd.generic);
    break;

  case ECHO:
    run_echo(cmd.echo);
    break;

  case PWD:
    run_pwd();
    break;

  case JOBS:
    run_jobs();
    break;

  case EXPORT:
  case CD:
  case KILL:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief A dispatch function to resolve the correct @a Command variant
 * function for the quash process.
 *
 * This version of the function is tailored to commands that should be run in
 * the parent process (quash).
 *
 * @param cmd The Command to try to run
 *
 * @sa Command
 */
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
  case EXPORT:
    run_export(cmd.export);
    break;

  case CD:
    run_cd(cmd.cd);
    break;

  case KILL:
    run_kill(cmd.kill);
    break;

  case GENERIC:
  case ECHO:
  case PWD:
  case JOBS:
  case EXIT:
  case EOC:
    break;

  default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
 * @brief Creates one new process centered around the @a Command in the @a
 * CommandHolder setting up redirects and pipes where needed
 *
 * @note Processes are not the same as jobs. A single job can have multiple
 * processes running under it. This function creates a process that is part of a
 * larger job.
 *
 * @note Not all commands should be run in the child process. A few need to
 * change the quash process in some way
 *
 * @param holder The CommandHolder to try to run
 *
 * @sa Command CommandHolder
 */
void create_process(CommandHolder holder) {
  // Read the flags field from the parser
  bool p_in  = holder.flags & PIPE_IN;
  bool p_out = holder.flags & PIPE_OUT;
  bool r_in  = holder.flags & REDIRECT_IN;
  bool r_out = holder.flags & REDIRECT_OUT;
  bool r_app = holder.flags & REDIRECT_APPEND; // This can only be true if r_out
                                               // is true

  // TODO: Remove warning silencers
  // (void) p_in;  // Silence unused variable warning
  // (void) p_out; // Silence unused variable warning
  // (void) r_in;  // Silence unused variable warning
  // (void) r_out; // Silence unused variable warning
  // (void) r_app; // Silence unused variable warning

  // TODO: Setup pipes, redirects, and new process
  // IMPLEMENT_ME();

  if (p_out)
  {
    pipe(pipes);
  }

  // using the fork system for creating new process that's called the child process
  pid_t pid = fork();

  push_back_process_queue(&p_q, pid);
  if (pid == 0) {
    // we will be using pipes here now
    // pipes was created in the very top
    if (p_in)
    {
      // dup system creates a constructor for us
      dup2(pipes[READ_END], STDIN_FILENO);
      close(pipes[WRITE_END]);
    }
    if (p_out)
    {
      dup2(pipes[WRITE_END], STDOUT_FILENO);
      close(pipes[READ_END]);
    }
    if (r_in)
    {
      FILE *file = fopen(holder.redirect_in, "r");
      dup2(fileno(file), STDIN_FILENO);
    }
    if (r_out)
    {
      if (r_app)
      {
        FILE *file = fopen(holder.redirect_out, "a");
        dup2(fileno(file), STDOUT_FILENO);
      }
      else
      {
        FILE *file = fopen(holder.redirect_out, "w");
        dup2(fileno(file), STDOUT_FILENO);
      }
    }

    child_run_command(holder.cmd);
    exit(0);
  }
  else {
    if (p_out) {
      close(pipes[WRITE_END]);
    }
    parent_run_command(holder.cmd);
  }

  //parent_run_command(holder.cmd); // This should be done in the parent branch of
                                  // a fork
  //child_run_command(holder.cmd); // This should be done in the child branch of a fork
}

// Run a list of commands
void run_script(CommandHolder* holders) {
  if(on == 0) {
    j_q = new_job_queue(1);
    on = 1;
  }
  p_q = new_process_queue(1);

  if (holders == NULL)
    return;

  check_jobs_bg_status();

  if (get_command_holder_type(holders[0]) == EXIT &&
      get_command_holder_type(holders[1]) == EOC) {
    end_main_loop();
    return;
  }

  CommandType type;

  // Run all commands in the `holder` array
  for (int i = 0; (type = get_command_holder_type(holders[i])) != EOC; ++i)
    create_process(holders[i]);

  if (!(holders[0].flags & BACKGROUND)) {
    // Not a background Job
    // TODO: Wait for all processes under the job to complete
    // IMPLEMENT_ME();
    while (!is_empty_process_queue(&p_q))
    {
      pid_t current_pid = pop_front_process_queue(&p_q);
      int state;

      waitpid(current_pid, &state, 0);
    }

    destroy_process_queue(&p_q);
  }
  else {
    // A background job.
    // TODO: Push the new job to the job queue
    // IMPLEMENT_ME();

    struct Job current_job;
    current_job.jobID = num_job;

    num_job++;

    current_job.p_q = p_q;
    current_job.pid = peek_back_process_queue(&p_q);
    current_job.cmd = get_command_string();

    // TODO: Once jobs are implemented, uncomment and fill the following line
    // print_job_bg_start(job_id, pid, cmd);
    push_back_job_queue(&j_q, current_job);
    print_job_bg_start(current_job.jobID, current_job.pid, current_job.cmd);
  }
}
