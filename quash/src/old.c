/**
 * @file execute.c
 *
 * @brief Implements interface functions between Quash and the environment and
 * functions that interpret an execute commands.
 *
 * @note As you add things to this file you may want to change the method signature
 */

#include "execute.h"

#include <stdio.h>

#include <sys/types.h>

#include <sys/wait.h>

#include <unistd.h>

#include "quash.h"

#include <sys/stat.h>

#include "deque.h"

#include <fcntl.h>

// Process Queue
IMPLEMENT_DEQUE_STRUCT (process_queue, pid_t);
IMPLEMENT_DEQUE (process_queue, pid_t);
process_queue pq;

// Job Struct
struct Job {
  int job_id;
  pid_t pid;
  char* cmd;
  process_queue pq;
}Job;

// Job Queue
IMPLEMENT_DEQUE_STRUCT (job_queue, struct Job);
IMPLEMENT_DEQUE (job_queue, struct Job);
job_queue jq;
int job_num = 1;

bool initialize = 0;
static int pipes[2];


/***************************************************************************
 * Interface Functions
 ***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  // TODO: Get the current working directory. This will fix the prompt path.
  // HINT: This should be pretty simple
  // Change this to true if necessary
  char* wd = NULL;
  wd = getcwd(NULL, 1024);
  *should_free = true;

  return wd;
}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  // TODO: Lookup environment variables. This is required for parser to be able
  // to interpret variables from the command line and display the prompt
  // correctly
  // HINT: This should be pretty simple

  return getenv(env_var);
}

// Check the status of background jobs
void check_jobs_bg_status() {
  // TODO: Check on the statuses of all processes belonging to all background
  // jobs. This function should remove jobs from the jobs queue once all
  // processes belonging to a job have completed.
  int num_jobs = length_job_queue(&jq);
  for(int i = 0; i < num_jobs; i++)
  {
    struct Job curr_job = pop_front_job_queue(&jq);

    int num_processes = length_process_queue(&curr_job.pq);
    pid_t front = peek_front_process_queue(&curr_job.pq);
    for(int j = 0; j < num_processes; j++)
    {
      pid_t curr_pid = pop_front_process_queue(&curr_job.pq);
      int status;
      if(waitpid(curr_pid, &status, WNOHANG) == 0)
      {
        push_back_process_queue(&curr_job.pq, curr_pid);
      }
    }

    if((is_empty_process_queue(&curr_job.pq)))
    {
      print_job_bg_complete(curr_job.job_id, front, curr_job.cmd);
    }
    else
    {
      push_back_job_queue(&jq, curr_job);
    }

  }

  // TODO: Once jobs are implemented, uncomment and fill the following line
  // print_job_bg_complete(job_id, pid, cmd);
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

  execvp(exec, args);

  perror("ERROR: Failed to execute program");
}

// Print strings
void run_echo(EchoCommand cmd) {
  // Print an array of strings. The args array is a NULL terminated (last
  // string is always NULL) list of strings.
  char** str = cmd.args;

  for(int i = 0; str[i] != NULL; i++)
  {
    printf("%s ", str[i]);
  }
  printf("\n");

  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
  // Write an environment variable
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;

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
  bool should_free = false;
  char* old_dir = get_current_directory(&should_free);
  chdir(dir);
  char* new_dir = get_current_directory(&should_free);
  setenv("OLD_PWD", old_dir, 1);
  setenv("PWD", new_dir, 1);

  free(old_dir);
  free(new_dir);
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
  int signal = cmd.sig;
  int job_id = cmd.job;

  // TODO: Kill all processes associated with a background job
  struct Job kill_job;
  int num_jobs = length_job_queue(&jq);
  for(int i = 0; i < num_jobs; i++)
  {
    kill_job = pop_front_job_queue(&jq);
    if(kill_job.job_id == job_id)
    {
      process_queue kill_pq = kill_job.pq;
      while(length_process_queue(&kill_pq) != 0)
      {
        pid_t kill_pid = pop_front_process_queue(&kill_pq);
        kill(kill_pid, signal);
      }
    }
    push_back_job_queue(&jq, kill_job);
  }
}


// Prints the current working directory to stdout
void run_pwd() {
  // TODO: Print the current working directory
  bool should_free;
  char* curr_dir = get_current_directory(&should_free);
  printf("%s\n", curr_dir);

  if(should_free)
  {
    free(curr_dir);
  }

  // Flush the buffer before returning
  fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  // TODO: Print background jobs
  int num_jobs = length_job_queue(&jq);
  for(int i = 0; i < num_jobs; i++)
  {
    struct Job curr_job = pop_front_job_queue(&jq);
    print_job(curr_job.job_id, curr_job.pid, curr_job.cmd);
    push_back_job_queue(&jq, curr_job);
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

  // TODO: Setup pipes, redirects, and new process
  if(p_out)
  {
    pipe(pipes);
  }

  pid_t pid = fork();
  push_back_process_queue(&pq, pid);

  if(pid == 0)
  {
    if(p_in)
    {
      dup2(pipes[0], STDIN_FILENO);
      close(pipes[1]); 
    }
    if(p_out)
    {
        dup2(pipes[1], STDOUT_FILENO);
        close(pipes[0]); 
    }
    if(r_in)
    {
        /*int fd = open(holder.redirect_in, O_RDONLY); //Fix
        dup2(fd, STDIN_FILENO);*/
        FILE* f = fopen(holder.redirect_in, "r");
        dup2(fileno(f), STDIN_FILENO);
    }
    if(r_out)
    {
      if(r_app)
      {
        /*int fd = open(holder.redirect_in, O_APPEND); //Fix
        dup2(fd, STDOUT_FILENO);*/
        FILE* f = fopen(holder.redirect_out, "a");
        dup2(fileno(f), STDOUT_FILENO);
      }
      else
      {
        /*int fd = open(holder.redirect_in, O_WRONLY); //Fix
        dup2(fd, STDOUT_FILENO);*/
        FILE* f = fopen(holder.redirect_out, "w");
        dup2(fileno(f), STDOUT_FILENO);
      }
    }

    child_run_command(holder.cmd);
    exit(0);
  }
  else
  {
    if(p_out)
    {
      close(pipes[1]);
    }
    parent_run_command(holder.cmd);
  }

  //parent_run_command(holder.cmd); // This should be done in the parent branch of
                                  // a fork
  //child_run_command(holder.cmd); // This should be done in the child branch of a fork
}

// Run a list of commands
void run_script(CommandHolder* holders) {
  
  //Need to initialize Job Queue and Process Queue Here?
  if(initialize == 0)
  {
    jq = new_job_queue(1);
    initialize = 1;
  }
  pq = new_process_queue(1);

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
    while(!is_empty_process_queue(&pq))
    {
      pid_t curr_process = pop_front_process_queue(&pq);
      int status;
      waitpid(curr_process, &status, 0);
    }
    destroy_process_queue(&pq);
  }
  else {
    // A background job.
    // TODO: Push the new job to the job queue
    struct Job new_job;
    new_job.job_id = job_num;
    ++job_num;
    new_job.pq = pq;
    new_job.cmd = get_command_string();
    new_job.pid = peek_back_process_queue(&pq);
    push_back_job_queue(&jq, new_job);

    // TODO: Once jobs are implemented, uncomment and fill the following line
     print_job_bg_start(new_job.job_id, new_job.pid, new_job.cmd);
  }
}