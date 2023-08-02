/* proj_1 simulate CPU and IO process with exponential distribution for inter event times */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
  int t_arrival; // arrival time
  int n_cpu_bursts; 
  int * cpu_burst_time; // array of CPU burst time
  int * io_burst_time; // array of IO burst time
  int io_ft; // supposed finish time of io burst; if process i is not in io burst, io_ft = -1
  int tau; // estimated cpu burst time: for SRT and RR
  int * wait_time;
  int current_burst;
  int enterQ;
  int cb_time; // the actual time spent in CPU for a single cpu burst 
  int * preemption;
} Process;

char * print_queue(int * Q, int q_size, char * queue);
int * pop(int * Q, int q_size);
// fcfs & sjf
void simulator(Process * processes, int num_procs, int t_cs, int n_io_procs, float alpha, char * algo);
// srt
void srt(Process * processes, int num_procs, int t_cs, int n_io_procs, float alpha);
//rr
void rr(Process * processes, int num_procs, int t_cs, int n_io_procs, int t_slice);

// helper funcs
void sort_by_name(int * Q, int q_size);
void sort_by_rt(int * Q, int q_size, Process * processes);
void sort_by_cb(int * Q, int q_size, Process * processes);
double ceiling(double x);
void write_to_file(double total_cpu_time, Process * processes, int t, int t_cs, int n_io_procs, int num_procs, int num_cs_cb, int num_cs_ib, char * alg);
void initialize_processes(Process * processes, int num_procs, double lambda);

//next_exp generate only 1 inter event time
double next_exp( double lambda, int cutoff )
{
  /* use a hard-coded value below instead of the current timestamp
   *  to generate the same pseudo-random number sequence each time
   *   you run this code
   */
 // srand48( time( NULL ) );
 // srand48( seed );
  double x; 
  int iterations = 1;  
  for ( int i = 0 ; i < iterations ; i++ )
  {
    double r = drand48();  /* uniform dist [0.00,1.00) -- also see random() */
    /* generate the next pseudo-random value x */
    x = -log( r ) / lambda;   /* log() is natural log (see man page) */
#if 1
     //skip values that are far down the "long tail" of the distribution 
    if ( x > cutoff ) { i--; continue; }
#endif
   } 
  
  return x;
}

int main(int argc , char** argv) {
  int interarrival;
  int n_cpu_bursts;
  int interarrival_cpu;
  int interarrival_io;


  if (argc != 9) {
    fprintf(stderr,"ERROR: <Invalid Arguments>\n");
    return EXIT_FAILURE;
  }

  int num_procs = atoi(*(argv+1));
  int n_cpu_procs = atoi(*(argv+2));
  int n_io_procs = num_procs - n_cpu_procs; 
  int seed = atoi(*(argv+3));
  double lambda = atof(*(argv+4));
  int cutoff = atoi(*(argv+5));
  int t_cs = atoi(*(argv+6)); 
  float alpha = atof(*(argv+7));
  int t_slice = atoi(*(argv+8)); // time slice for RR
  if (t_cs % 2 != 0) {
    fprintf(stderr,"ERROR: <Invalid Arguments>\n");
    return EXIT_FAILURE;
  }
  srand48( seed ); 
  if (n_cpu_procs==1){
    printf("<<< PROJECT PART I -- process set (n=%d) with %d CPU-bound process >>>\n", num_procs, n_cpu_procs );
  }
  else{
    printf("<<< PROJECT PART I -- process set (n=%d) with %d CPU-bound processes >>>\n",num_procs, n_cpu_procs );
  }

  /* Generate all process and record arrival time, number of CPU bursts, CPU burst time and IO burst time */
  Process * processes = calloc(num_procs, sizeof(Process));

  //io process
  for (int i =0; i< n_io_procs ; i++) {
    interarrival = floor(next_exp( lambda, cutoff ));
    n_cpu_bursts = ceil(drand48() * 100);
    //printf("I/O-bound process %c: arrival time %dms; %d CPU bursts:\n",i+65,interarrival,n_cpu_bursts);
        // each cpu burst with io burst
    processes[i].cpu_burst_time = calloc(n_cpu_bursts, sizeof(int));
    processes[i].io_burst_time = calloc(n_cpu_bursts-1, sizeof(int));
    for (int j=0;j<n_cpu_bursts -1;j++ ){
      interarrival_cpu = ceil(next_exp(  lambda, cutoff ));
      interarrival_io = ceil(next_exp(  lambda, cutoff ))*10;
      //printf("--> CPU burst %dms --> I/O burst %dms\n",interarrival_cpu,interarrival_io );

      processes[i].cpu_burst_time[j] = interarrival_cpu;
      processes[i].io_burst_time[j] = interarrival_io;
    }
    // generate last cpu burst
    interarrival_cpu = ceil(next_exp( lambda, cutoff ));
    //printf("--> CPU burst %dms\n", interarrival_cpu);
    processes[i].cpu_burst_time[n_cpu_bursts-1] = interarrival_cpu;
    processes[i].t_arrival = interarrival;
    processes[i].n_cpu_bursts = n_cpu_bursts;
    processes[i].wait_time = calloc(n_cpu_bursts, sizeof(int));
    processes[i].preemption = calloc(n_cpu_bursts, sizeof(int));
  }
  // cpu -process
  for (int i =0; i< n_cpu_procs ; i++) {
    interarrival = floor(next_exp(  lambda, cutoff ));
    n_cpu_bursts = ceil(drand48()*100);
    //printf("CPU-bound process %c: arrival time %dms; %d CPU bursts:\n",i+65+n_io_procs,interarrival,n_cpu_bursts);
    // each cpu burst with io burst
    processes[i+n_io_procs].cpu_burst_time = calloc(n_cpu_bursts, sizeof(int));
    processes[i+n_io_procs].io_burst_time = calloc(n_cpu_bursts-1, sizeof(int));
    for (int j=0;j<n_cpu_bursts -1;j++ ){
      interarrival_cpu = ceil(next_exp(  lambda, cutoff ))*4;
      interarrival_io = ceil(next_exp(  lambda, cutoff ))*10/4;
      //printf("--> CPU burst %dms --> I/O burst %dms\n",interarrival_cpu,interarrival_io );

      processes[i+n_io_procs].cpu_burst_time[j] = interarrival_cpu;
      processes[i+n_io_procs].io_burst_time[j] = interarrival_io;
    }
    // generate last cpu burst
    interarrival_cpu = ceil(next_exp(  lambda, cutoff ))*4;
    //printf("--> CPU burst %dms\n", interarrival_cpu);
    processes[i+n_io_procs].cpu_burst_time[n_cpu_bursts-1] = interarrival_cpu;
    processes[i+n_io_procs].t_arrival = interarrival;
    processes[i+n_io_procs].n_cpu_bursts = n_cpu_bursts;
    processes[i+n_io_procs].wait_time = calloc(n_cpu_bursts, sizeof(int));
    processes[i+n_io_procs].preemption = calloc(n_cpu_bursts, sizeof(int));
  } 
  
  for (int i = 0; i < n_io_procs; i++) {
    printf("I/O-bound process %c: arrival time %dms; %d CPU bursts\n", i+65, processes[i].t_arrival, processes[i].n_cpu_bursts);
  }
  for (int i = n_io_procs; i < num_procs; i++) {
    printf("CPU-bound process %c: arrival time %dms; %d CPU bursts\n", i+65, processes[i].t_arrival, processes[i].n_cpu_bursts);
  }
  printf("\n");
  printf("<<< PROJECT PART II -- t_cs=%dms; alpha=%.2f; t_slice=%dms >>>\n", t_cs, alpha, t_slice);
  // FCFS 
  initialize_processes(processes, num_procs, lambda);
  simulator(processes, num_procs, t_cs, n_io_procs, alpha, "FCFS");
  printf("\n");
  
  // SJF 
  initialize_processes(processes, num_procs, lambda);
  simulator(processes, num_procs, t_cs, n_io_procs, alpha, "SJF");
  printf("\n");

  // SRT 
  initialize_processes(processes, num_procs, lambda);
  srt(processes, num_procs, t_cs, n_io_procs, alpha);
  printf("\n");

  // RR 
  initialize_processes(processes, num_procs, lambda);
  rr(processes, num_procs, t_cs, n_io_procs, t_slice);

  // free memory
  for (int i = 0; i < num_procs; i++) {
    free(processes[i].cpu_burst_time);
    free(processes[i].io_burst_time);
    free(processes[i].wait_time);
    free(processes[i].preemption);
  }
  free(processes);

  return EXIT_SUCCESS;
}

void simulator(Process * processes, int num_procs, int t_cs, int n_io_procs, float alpha, char * algo) {
  /* The only change compared to fcfs is that we sort the Q right after adding elements to it */
  
  int * Q = NULL; // Ready queue
  int q_size = 0;
  char * queue = calloc(12,sizeof(char)); // printed format of Q
  int cpu_timer = 0; // elapsed time for a single CPU burst
  int pid = -1; // the process that is using the CPU; if -1, no process is using CPU
  int t = 0; // elapsed time
  int n_finished = 0; // number of finished processes
  int io_ft;
  double total_cpu_time = 0; 
  int remaining_cpu_bursts;
  int num_cs_cb = 0; // # of context siwtch for cpu-bound
  int num_cs_ib = 0; // # of context siwtch for io-bound
  bool print_cond = true;
  int old_tau;
  int old_pid;
  printf("time 0ms: Simulator started for %s [Q <empty>]\n", algo);

  while (n_finished != num_procs) {
    /*******Uncomment this before submission!!!********/
    //if (t > 10000) print_cond = false;
    /**************************************************/
    if (pid != -1) {
      //Process proc = processes[pid];
      cpu_timer += 1;
      total_cpu_time += 1;
      /* ********************CPU burst Completion********************** */
      if (cpu_timer == processes[pid].cpu_burst_time[processes[pid].current_burst]) { 
        //reset tau
        queue = print_queue(Q, q_size, queue);
        // update the number of completed cpu burst
        processes[pid].current_burst += 1;
        if (processes[pid].current_burst == processes[pid].n_cpu_bursts) {  
          /* ********************Process finished all CPU bursts********************** */
          printf("time %dms: Process %c terminated %s\n", t, pid+65, queue);
          n_finished += 1;
        } else { 
          //printf("current_burst %d processes[pid].n_cpu_bursts %d\n", processes[pid].current_burst, processes[pid].n_cpu_bursts);
          remaining_cpu_bursts = processes[pid].n_cpu_bursts - processes[pid].current_burst;
          if (print_cond) {
            char * b;
            b = (remaining_cpu_bursts == 1) ? "burst" : "bursts";
            if (strcmp(algo, "SJF") == 0) {
              printf("time %dms: Process %c (tau %dms) completed a CPU burst; %d %s to go %s\n", t, pid+65, processes[pid].tau, remaining_cpu_bursts, b, queue);
            } else {
              printf("time %dms: Process %c completed a CPU burst; %d %s to go %s\n", t, pid+65, remaining_cpu_bursts, b, queue);
            }
          }
         
          // update cpu burst estimate. why here? we need to make sure the next time we use tau, the cpu estimate is correct. so far the t hasn't been updated, meaning we're still in the same ms as CPU takes in a project
          old_tau = processes[pid].tau;
          processes[pid].tau = ceil(alpha * cpu_timer + (1 - alpha) * processes[pid].tau);
          io_ft = t + t_cs/2 + processes[pid].io_burst_time[processes[pid].current_burst-1];
          //printf("processes[pid].current_burst %d, processes[pid].io_burst_time[processes[pid].current_burst] %d\n",processes[pid].current_burst, processes[pid].io_burst_time[processes[pid].current_burst]);
          if (print_cond){
            if (strcmp(algo, "SJF") == 0) {
              printf("time %dms: Recalculating tau for process %c: old tau %dms ==> new tau %dms %s\n", t, pid+65, old_tau, processes[pid].tau, queue);
            }
            printf("time %dms: Process %c switching out of CPU; blocking on I/O until time %dms %s\n", t, pid+65, io_ft, queue);
          }
          // update process stats: io_ft 
          processes[pid].io_ft = io_ft;
        }
        t += t_cs/2; // context switch out time
        // release CPU
        cpu_timer = 0;
        pid = -1; // set the status as no process is using cpu
      }
    }

    /* ********************Pop Ready Queue -> start using CPU********************** */
    // pop the first item of Q and start using CPU, update pid
    if (q_size != 0 && pid == -1) {
      old_pid = pid;
      pid = Q[0];
      // update pid's wait time, excluding the context switch time
      if (old_pid == -1) {
        processes[pid].wait_time[processes[pid].current_burst] += t - processes[pid].enterQ;
      } else {
        processes[pid].wait_time[processes[pid].current_burst] += t - processes[pid].enterQ - t_cs/2; // exclude the previous process' context switch out time
      }
      t += t_cs/2; // context switch in time
      q_size -= 1;
      if (pid < n_io_procs) {
        num_cs_ib += 1;
      } else {
        num_cs_cb += 1;
      }

      Q = pop(Q, q_size);
      
      queue = print_queue(Q, q_size, queue);
      if (print_cond){
        if (strcmp(algo, "SJF") == 0) {
          printf("time %dms: Process %c (tau %dms) started using the CPU for %dms burst %s\n", t, pid+65, processes[pid].tau,processes[pid].cpu_burst_time[processes[pid].current_burst], queue);
        } else {
          printf("time %dms: Process %c started using the CPU for %dms burst %s\n", t, pid+65,processes[pid].cpu_burst_time[processes[pid].current_burst], queue);
        }
      }
    }
    
    /* ********************Check IO burst completion********************** */
    int arrive = 0;
    for (int i = 0; i < num_procs; i++) {
      if (processes[i].io_ft == t) {
        arrive = 1;
        if (q_size == 0) {
          Q = calloc(1, sizeof(int));
          Q[0] = i;
        } else {
          Q = realloc(Q, sizeof(int)*(q_size+1));
          Q[q_size] = i;
        }
        q_size += 1;
        // adding i to queue -> update i's time of entry to the ready queue
        processes[i].enterQ = t;
        
        if (strcmp(algo, "SJF") == 0) {
          sort_by_name(Q, q_size);
          sort_by_cb(Q, q_size, processes);
        }

        queue = print_queue(Q, q_size, queue);
        if (print_cond){
          if (strcmp(algo, "SJF") == 0) { 
            printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue %s\n", t, i+65,processes[i].tau, queue);
          } else {
            printf("time %dms: Process %c completed I/O; added to ready queue %s\n", t, i+65, queue);
          }
        }
      }
    }

    /* ********************Check Process Arrival********************** */
    // loop through t_arrival, if t_arrival == t, add process to Q
    for (int i = 0; i < num_procs; i++) {
      if (processes[i].current_burst == processes[i].n_cpu_bursts) {continue;} // skip finished jobs
      if (processes[i].t_arrival == t) {
        if (q_size == 0) {
          Q = calloc(1, sizeof(int));
          Q[0] = i;
        } else {
          Q = realloc(Q, sizeof(int)*(q_size+1));
          Q[q_size] = i;
        }
        q_size += 1;
        arrive = 1;
        
        if (strcmp(algo, "SJF") == 0) {
          sort_by_name(Q, q_size);
          sort_by_cb(Q, q_size, processes);
        }
        
        queue = print_queue(Q, q_size, queue);
        if (print_cond){
          if (strcmp(algo, "SJF") == 0) {
            printf("time %dms: Process %c (tau %dms) arrived; added to ready queue %s\n", t, i+65, processes[i].tau,queue);
          } else {
            printf("time %dms: Process %c arrived; added to ready queue %s\n", t, i+65, queue);
          }
        }
        // adding i to queue -> update i's time of entry to the ready queue
        processes[i].enterQ = t;
      }
    }
    

    /* ********************Update Time********************** */
    if (pid == -1) { // when nothing is in the CPU
      if (arrive == 0) { // when no process arrives, we update the time
        t += 1;
      } // when CPU is idle and some process arrives at ready queue, we do not increment time, because the CPU will take that process immediately. Example to check: when process A arrives, the time is 136ms, when A starts executing, the time is 138ms, the 2ms difference *only* comes from the context switch-in time, if we increment t here, the time of A to start using CPU would be 139ms, which is wrong.
    } else {
      t += 1; // when CPU is working, we keep incrementing time
    }
  }
  queue = print_queue(Q, q_size, queue);
  printf("time %dms: Simulator ended for %s %s\n", t-1, algo, queue);
  free(queue);
  free(Q);

  char * alg = calloc(100, sizeof(char));
  sprintf(alg, "Algorithm %s\n", algo);
  write_to_file(total_cpu_time, processes, t, t_cs, n_io_procs, num_procs, num_cs_cb, num_cs_ib, alg);
  FILE *file = fopen("simout.txt", "a");
  fputs("\n\n", file);
  fclose(file);
}



/* ************************************** SRT ****************************************** */
/* ************************************************************************************* */

void srt(Process * processes, int num_procs, int t_cs, int n_io_procs, float alpha) {
  int * Q = NULL; // Ready queue
  int q_size = 0;
  char * queue = calloc(12,sizeof(char)); // printed format of Q
  int pid = -1; // the process that is using the CPU; if -1, no process is using CPU
  int t = 0; // elapsed time
  int n_finished = 0; // number of finished processes
  int io_ft;
  double total_cpu_time = 0; 
  int remaining_cpu_bursts;
  int num_cs_cb = 0; // # of context siwtch for cpu-bound
  int num_cs_ib = 0; // # of context siwtch for io-bound
  bool print_cond = true;
  int old_tau;
  int old_pid;
  int preempt_pid = -1;
  int min_rt;
  printf("time 0ms: Simulator started for SRT [Q <empty>]\n");

  while (n_finished != num_procs) {
    /*******Uncomment this before submission!!!********/
    //if (t > 10000) print_cond = false;
    /**************************************************/
    if (preempt_pid != -1) {
      t += t_cs/2; // context switch out
      processes[pid].preemption[processes[pid].current_burst] += 1;// update number of preemptions
      // add process back to Q
      Q = realloc(Q, sizeof(int)*(q_size+1));
      Q[q_size] = pid;
      q_size += 1;
      sort_by_rt(Q, q_size, processes);
      processes[pid].enterQ = t;
      pid = -1;
      
    }
    if (pid != -1) {
      
      processes[pid].cb_time += 1;
      /* ********************CPU burst Completion********************** */
      if (processes[pid].cb_time == processes[pid].cpu_burst_time[processes[pid].current_burst]) { 
        queue = print_queue(Q, q_size, queue);
        // update the number of completed cpu burst
        processes[pid].current_burst += 1;
        if (processes[pid].current_burst == processes[pid].n_cpu_bursts) {  
          /* ********************Process finished all CPU bursts********************** */
          printf("time %dms: Process %c terminated %s\n", t, pid+65, queue);
          n_finished += 1;
        } else { 
          //printf("current_burst %d processes[pid].n_cpu_bursts %d\n", processes[pid].current_burst, processes[pid].n_cpu_bursts);
          remaining_cpu_bursts = processes[pid].n_cpu_bursts - processes[pid].current_burst;
          if (remaining_cpu_bursts == 1) {
            if (print_cond){
              printf("time %dms: Process %c (tau %dms) completed a CPU burst; %d burst to go %s\n", t, pid+65, processes[pid].tau, remaining_cpu_bursts, queue);
            }
          } else {
            if (print_cond){
              printf("time %dms: Process %c (tau %dms) completed a CPU burst; %d bursts to go %s\n", t, pid+65, processes[pid].tau,remaining_cpu_bursts, queue);
            }
          }
          // update cpu burst estimate. why here? we need to make sure the next time we use tau, the cpu estimate is correct. so far the t hasn't been updated, meaning we're still in the same ms as CPU takes in a project
          old_tau = processes[pid].tau;
          processes[pid].tau = ceil(alpha * processes[pid].cb_time + (1 - alpha) * processes[pid].tau);
          io_ft = t + t_cs/2 + processes[pid].io_burst_time[processes[pid].current_burst-1];
          //printf("processes[pid].current_burst %d, processes[pid].io_burst_time[processes[pid].current_burst] %d\n",processes[pid].current_burst, processes[pid].io_burst_time[processes[pid].current_burst]);
          if (print_cond){
            printf("time %dms: Recalculating tau for process %c: old tau %dms ==> new tau %dms %s\n", t, pid+65, old_tau, processes[pid].tau, queue);
            printf("time %dms: Process %c switching out of CPU; blocking on I/O until time %dms %s\n", t, pid+65, io_ft, queue);
          }
          // update process stats: io_ft 
          processes[pid].io_ft = io_ft;
        }
        t += t_cs/2; // context switch out time
        // release CPU
        processes[pid].cb_time = 0;
        pid = -1; // set the status as no process is using cpu
      } 
      total_cpu_time += 1;
    }
    

    /* ********************Pop Ready Queue -> start using CPU********************** */
    // pop the first item of Q and start using CPU, update pid
    if (q_size != 0 && pid == -1) {
      old_pid = pid;
      pid = Q[0];
      // update pid's wait time, excluding the context switch time
      if (old_pid == -1) {
        processes[pid].wait_time[processes[pid].current_burst] += t - processes[pid].enterQ;
      } else {
        processes[pid].wait_time[processes[pid].current_burst] += t - processes[pid].enterQ - t_cs/2; // exclude the previous process' context switch out time
      }
      t += t_cs/2; // context switch in time
      q_size -= 1;
      if (pid < n_io_procs) {
        num_cs_ib += 1;
      } else {
        num_cs_cb += 1;
      }

      Q = pop(Q, q_size);
      
      queue = print_queue(Q, q_size, queue);
      if (print_cond){
        if (processes[pid].cb_time != 0) {
          printf("time %dms: Process %c (tau %dms) started using the CPU for remaining %dms of %dms burst %s\n", t, pid+65, processes[pid].tau,processes[pid].cpu_burst_time[processes[pid].current_burst] - processes[pid].cb_time, processes[pid].cpu_burst_time[processes[pid].current_burst], queue);
        } else {
          printf("time %dms: Process %c (tau %dms) started using the CPU for %dms burst %s\n", t, pid+65, processes[pid].tau,processes[pid].cpu_burst_time[processes[pid].current_burst], queue);
        }
        
      }
    }
    
    /* ********************Check IO burst completion********************** */
    int arrive = 0;
    int * io_complete = calloc(num_procs, sizeof(int));
    preempt_pid = -1;
    if (pid != -1) {
      min_rt = processes[pid].tau - processes[pid].cb_time;
    } 
    
    for (int i = 0; i < num_procs; i++) {
      if (processes[i].io_ft == t) {
        arrive = 1;
        io_complete[i] = 1;
        if (q_size == 0) {
          Q = calloc(1, sizeof(int));
          Q[0] = i;
        } else {
          Q = realloc(Q, sizeof(int)*(q_size+1));
          Q[q_size] = i;
        }
        q_size += 1;
        // adding i to queue -> update i's time of entry to the ready queue
        processes[i].enterQ = t;
       
        // sort Q
        sort_by_name(Q, q_size);
        sort_by_rt(Q, q_size, processes);
        queue = print_queue(Q, q_size, queue);

        if (pid != -1) {
          if (processes[i].tau - processes[i].cb_time < min_rt) {
            preempt_pid = i;
            min_rt = processes[i].tau - processes[i].cb_time;
            printf("time %dms: Process %c (tau %dms) completed I/O; preempting %c %s\n", t, i+65,processes[i].tau, pid+65, queue);
          } else {
            printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue %s\n", t, i+65,processes[i].tau, queue);
          }
        } else {
          printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue %s\n", t, i+65,processes[i].tau, queue);
        }

      }
    }
    
    /* ********************Check Process Arrival********************** */
    // loop through t_arrival, if t_arrival == t, add process to Q
    int * arrival = calloc(num_procs, sizeof(int));
    for (int i = 0; i < num_procs; i++) {
      if (processes[i].current_burst == processes[i].n_cpu_bursts) {continue;} // skip finished jobs
      if (processes[i].t_arrival == t) {
        arrival[i] = 1;
        if (q_size == 0) {
          Q = calloc(1, sizeof(int));
          Q[0] = i;
        } else {
          Q = realloc(Q, sizeof(int)*(q_size+1));
          Q[q_size] = i;
        }
        q_size += 1;
        arrive = 1;
        sort_by_name(Q, q_size);
        sort_by_rt(Q, q_size, processes);
        queue = print_queue(Q, q_size, queue);
        // update i's time of entry to the ready queue
        processes[i].enterQ = t;

        if (pid != -1) {
          if (processes[i].tau - processes[i].cb_time < min_rt) {
            preempt_pid = i;
            min_rt = processes[i].tau - processes[i].cb_time;
            printf("time %dms: Process %c (tau %dms) arrived; preempting %c %s\n", t, i+65, processes[i].tau, pid+65, queue);
          } else {
            printf("time %dms: Process %c (tau %dms) arrived; added to ready queue %s\n", t, i+65, processes[i].tau, queue);
          }
        } else {
          printf("time %dms: Process %c (tau %dms) arrived; added to ready queue %s\n", t, i+65, processes[i].tau, queue);
        }
      }
    }
    // print after adding everything
    // if (print_cond){
    //   for (int i = 0; i < num_procs; i++) {
    //     if (io_complete[i] == 1) {
    //       if (preempt_pid == i) {
    //         printf("time %dms: Process %c (tau %dms) completed I/O; preempting %c %s\n", t, i+65,processes[i].tau, pid+65, queue);
    //       } else {
    //         printf("time %dms: Process %c (tau %dms) completed I/O; added to ready queue %s\n", t, i+65,processes[i].tau, queue);
    //       }
    //     }
    //     if (arrival[i] == 1) {
    //       if (preempt_pid == i) {
    //         printf("time %dms: Process %c (tau %dms) arrived; preempting %c %s\n", t, i+65, processes[i].tau, pid+65, queue);
    //       } else {
    //         printf("time %dms: Process %c (tau %dms) arrived; added to ready queue %s\n", t, i+65, processes[i].tau, queue);
    //       }
    //     }
    //   }
    // }
    free(io_complete);
    free(arrival);


    /* ********************Update Time********************** */
    if (preempt_pid != -1) {
      continue;
    }
    if (pid == -1) { // when nothing is in the CPU
      if (arrive == 0) { // when no process arrives, we update the time
        t += 1;
      } // when CPU is idle and some process arrives at ready queue, we do not increment time, because the CPU will take that process immediately. Example to check: when process A arrives, the time is 136ms, when A starts executing, the time is 138ms, the 2ms difference *only* comes from the context switch-in time, if we increment t here, the time of A to start using CPU would be 139ms, which is wrong.
    } else {
      t += 1; // when CPU is working, we keep incrementing time
    }
    
  }
  queue = print_queue(Q, q_size, queue);
  printf("time %dms: Simulator ended for SRT %s\n", t-1, queue);
  free(queue);
  free(Q);

  char * alg = calloc(100, sizeof(char));
  strcpy(alg, "Algorithm SRT\n");
  write_to_file(total_cpu_time, processes, t, t_cs, n_io_procs, num_procs, num_cs_cb, num_cs_ib, alg);
  FILE *file = fopen("simout.txt", "a");
  fputs("\n\n", file);
  fclose(file);
}


/* ************************************** RR ****************************************** */
/* ************************************************************************************* */
void rr(Process * processes, int num_procs, int t_cs, int n_io_procs, int t_slice) {
  int * Q = NULL; // Ready queue
  int q_size = 0;
  char * queue = calloc(12,sizeof(char)); // printed format of Q
  int pid = -1; // the process that is using the CPU; if -1, no process is using CPU
  int t = 0; // elapsed time
  int n_finished = 0; // number of finished processes
  int io_ft;
  double total_cpu_time = 0; 
  int remaining_cpu_bursts;
  int num_cs_cb = 0; // # of context siwtch for cpu-bound
  int num_cs_ib = 0; // # of context siwtch for io-bound
  bool print_cond = true;
  int old_pid;
  printf("time 0ms: Simulator started for RR [Q <empty>]\n");

  while (n_finished != num_procs) {
    /*******Uncomment this before submission!!!********/
    //if (t > 10000) print_cond = false;
    /**************************************************/
    if (pid != -1) {
      total_cpu_time += 1;
      processes[pid].cb_time += 1;
      
      /* ********************CPU burst Completion********************** */
      if (processes[pid].cb_time == processes[pid].cpu_burst_time[processes[pid].current_burst]) { 
        queue = print_queue(Q, q_size, queue);
        // update the number of completed cpu burst
        processes[pid].current_burst += 1;
        if (processes[pid].current_burst == processes[pid].n_cpu_bursts) {  
          /* ********************Process finished all CPU bursts********************** */
          printf("time %dms: Process %c terminated %s\n", t, pid+65, queue);
          n_finished += 1;
        } else { 
          //printf("current_burst %d processes[pid].n_cpu_bursts %d\n", processes[pid].current_burst, processes[pid].n_cpu_bursts);
          remaining_cpu_bursts = processes[pid].n_cpu_bursts - processes[pid].current_burst;
          if (remaining_cpu_bursts == 1) {
            if (print_cond){
              printf("time %dms: Process %c completed a CPU burst; %d burst to go %s\n", t, pid+65, remaining_cpu_bursts, queue);
            }
          } else {
            if (print_cond){
              printf("time %dms: Process %c completed a CPU burst; %d bursts to go %s\n", t, pid+65,remaining_cpu_bursts, queue);
            }
          }
          io_ft = t + t_cs/2 + processes[pid].io_burst_time[processes[pid].current_burst-1];
          if (print_cond){
            printf("time %dms: Process %c switching out of CPU; blocking on I/O until time %dms %s\n", t, pid+65, io_ft, queue);
          }
          // update process stats: io_ft 
          processes[pid].io_ft = io_ft;
        }
        t += t_cs/2; // context switch out time
        // release CPU
        processes[pid].cb_time = 0;
        pid = -1; // set the status as no process is using cpu
      }  
    }

    if (pid != -1) {    
      queue = print_queue(Q, q_size, queue);
      // printf("t = %d processes[pid].cb_time = %d ", t, processes[pid].cb_time);
      // printf("processes[pid].cb_time %% t_slice = %d ", processes[pid].cb_time % t_slice);
      // printf("processes[pid].cb_time / t_slice = %d\n", processes[pid].cb_time / t_slice );
      if (processes[pid].cb_time % t_slice == 0 && processes[pid].cb_time / t_slice > 0) {
        if (q_size == 0) {
          printf("time %dms: Time slice expired; no preemption because ready queue is empty %s\n", t, queue);
        } else {
          printf("time %dms: Time slice expired; preempting process %c with %dms remaining %s\n", t, pid+65, processes[pid].cpu_burst_time[processes[pid].current_burst] - processes[pid].cb_time, queue);
          processes[pid].preemption[processes[pid].current_burst] += 1;
          t += t_cs/2; // context switch out
          Q = realloc(Q, sizeof(int)*(q_size+1));
          Q[q_size] = pid;
          q_size += 1;
          processes[pid].enterQ = t;
          pid = -1;
        }
      } 
    }
    

    /* ********************Pop Ready Queue -> start using CPU********************** */
    // pop the first item of Q and start using CPU, update pid
    if (q_size != 0 && pid == -1) {
      old_pid = pid;
      pid = Q[0];
      // update pid's wait time, excluding the context switch time
      if (old_pid == -1) {
        processes[pid].wait_time[processes[pid].current_burst] += t - processes[pid].enterQ;
      } else {
        processes[pid].wait_time[processes[pid].current_burst] += t - processes[pid].enterQ - t_cs/2; // exclude the previous process' context switch out time
      }
      t += t_cs/2; // context switch in time
      q_size -= 1;
      if (pid < n_io_procs) {
        num_cs_ib += 1;
      } else {
        num_cs_cb += 1;
      }

      Q = pop(Q, q_size);
      
      queue = print_queue(Q, q_size, queue);
      if (print_cond){
        if (processes[pid].cb_time != 0) {
          printf("time %dms: Process %c started using the CPU for remaining %dms of %dms burst %s\n", t, pid+65, processes[pid].cpu_burst_time[processes[pid].current_burst] - processes[pid].cb_time, processes[pid].cpu_burst_time[processes[pid].current_burst], queue);
        } else {
          printf("time %dms: Process %c started using the CPU for %dms burst %s\n", t, pid+65, processes[pid].cpu_burst_time[processes[pid].current_burst], queue);
        }
        
      }
    }
    
    /* ********************Check IO burst completion********************** */
    int arrive = 0;
    int * io_complete = calloc(num_procs, sizeof(int)); 
    
    for (int i = 0; i < num_procs; i++) {
      if (processes[i].io_ft == t) {
        arrive = 1;
        io_complete[i] = 1;
        if (q_size == 0) {
          Q = calloc(1, sizeof(int));
          Q[0] = i;
        } else {
          Q = realloc(Q, sizeof(int)*(q_size+1));
          Q[q_size] = i;
        }
        q_size += 1;
        // adding i to queue -> update i's time of entry to the ready queue
        processes[i].enterQ = t;
        //sort_by_name(Q, q_size);
        queue = print_queue(Q, q_size, queue);
        if (print_cond) {
          printf("time %dms: Process %c completed I/O; added to ready queue %s\n", t, i+65, queue);
        }
      }
    }
    
    /* ********************Check Process Arrival********************** */
    // loop through t_arrival, if t_arrival == t, add process to Q
    int * arrival = calloc(num_procs, sizeof(int));
    for (int i = 0; i < num_procs; i++) {
      if (processes[i].current_burst == processes[i].n_cpu_bursts) {continue;} // skip finished jobs
      if (processes[i].t_arrival == t) {
        arrival[i] = 1;
        if (q_size == 0) {
          Q = calloc(1, sizeof(int));
          Q[0] = i;
        } else {
          Q = realloc(Q, sizeof(int)*(q_size+1));
          Q[q_size] = i;
        }
        q_size += 1;
        arrive = 1;
        //sort_by_name(Q, q_size);
        queue = print_queue(Q, q_size, queue);
        // update i's time of entry to the ready queue
        processes[i].enterQ = t;

        if (print_cond) {
          printf("time %dms: Process %c arrived; added to ready queue %s\n", t, i+65, queue);
        }
      }
    }
    
    free(io_complete);
    free(arrival);

    
    /* ********************Update Time********************** */
    if (pid == -1) { // when nothing is in the CPU
      if (arrive == 0) { // when no process arrives, we update the time
        t += 1;
      } // when CPU is idle and some process arrives at ready queue, we do not increment time, because the CPU will take that process immediately. Example to check: when process A arrives, the time is 136ms, when A starts executing, the time is 138ms, the 2ms difference *only* comes from the context switch-in time, if we increment t here, the time of A to start using CPU would be 139ms, which is wrong.
    } else {
      t += 1; // when CPU is working, we keep incrementing time
    }
    
  }
  queue = print_queue(Q, q_size, queue);
  printf("time %dms: Simulator ended for RR %s", t-1, queue);
  free(queue);
  free(Q);

  char * alg = calloc(100, sizeof(char));
  strcpy(alg, "Algorithm RR\n");
  write_to_file(total_cpu_time, processes, t, t_cs, n_io_procs, num_procs, num_cs_cb, num_cs_ib, alg);
}








/* ************************************** Helper Functions ****************************************** */
/* ************************************************************************************************** */
void sort_by_name(int * Q, int q_size) {
  for (int i = 0; i < q_size - 1; i++) {
    int min_idx = i;
    for (int j = i + 1; j < q_size; j++) {
        if ( Q[j] < Q[i] ) {
          min_idx = j;
        }
    }
    int temp = Q[i];
    Q[i] = Q[min_idx];
    Q[min_idx] = temp;
  }
}

void sort_by_rt(int * Q, int q_size, Process * processes) {
  /* sort by remaining time */
  int rt;
  int min_rt;
  for (int i = 0; i < q_size - 1; i++) {
    int min_idx = i;
    for (int j = i + 1; j < q_size; j++) {
        rt = processes[Q[j]].tau - processes[Q[j]].cb_time;
        min_rt = processes[Q[min_idx]].tau - processes[Q[min_idx]].cb_time;
        if ( rt < min_rt ) {
          min_idx = j;
        }
    }
    int temp = Q[i];
    Q[i] = Q[min_idx];
    Q[min_idx] = temp;
  }
}

void sort_by_cb(int * Q, int q_size, Process * processes) {
  /* sort by cpu burst */
  for (int i = 0; i < q_size - 1; i++) {
    int min_idx = i;
    for (int j = i + 1; j < q_size; j++) {
        if (processes[Q[j]].tau < processes[Q[min_idx]].tau ) {
          min_idx = j;
        }
    }
    int temp = Q[i];
    Q[i] = Q[min_idx];
    Q[min_idx] = temp;
  }
}

void initialize_processes(Process * processes, int num_procs, double lambda) {
  /* assumption: the following is already initialized: n_cpu_bursts, cpu_burst_time, io_burst_time */
  for (int i = 0; i < num_procs; i++) {
    processes[i].io_ft = -1;
    processes[i].tau = (int) 1 / lambda;
    processes[i].current_burst = 0;
    processes[i].cb_time = 0;
    for (int j = 0; j < processes[i].n_cpu_bursts; j++) {
      processes[i].wait_time[j] = 0;
      processes[i].preemption[j] = 0;
    }
  }
}

void write_to_file(double total_cpu_time, Process * processes, int t, int t_cs, int n_io_procs, int num_procs, int num_cs_cb, int num_cs_ib, char * alg) {
  FILE *file = fopen("simout.txt", "a");
  fputs(alg, file);
  
  char * cpu_util = calloc(500, sizeof(char));
  sprintf(cpu_util, "-- CPU utilization: %.3f%%\n", ceiling(100 * total_cpu_time / (t-1)));
  fputs(cpu_util, file);
  free(cpu_util);

  double total_burst_cb = 0; 
  double total_burst_ib = 0;
  double cpu_burst_cb = 0; // cpu bound 
  double cpu_burst_ib = 0; // io bound
  double wait_time_cb = 0;
  double wait_time_ib = 0;
  double tt_cb = 0; //turnaround time  CPU bound 
  double tt_ib = 0;
  int preempt_cb = 0;
  int preempt_ib = 0;
  for (int i = 0; i < num_procs; i++) {
    for (int j = 0; j < processes[i].n_cpu_bursts; j++) {
      if (i < n_io_procs) {
        cpu_burst_ib += (double) processes[i].cpu_burst_time[j];
        wait_time_ib += (double) processes[i].wait_time[j];
        tt_ib += (double) (processes[i].cpu_burst_time[j] + processes[i].wait_time[j] + t_cs + t_cs * processes[i].preemption[j]);
        total_burst_ib += 1;
        preempt_ib += processes[i].preemption[j];
        
      } else {
        cpu_burst_cb += (double)processes[i].cpu_burst_time[j];
        wait_time_cb += (double)processes[i].wait_time[j];
        tt_cb += (double) (processes[i].cpu_burst_time[j] + processes[i].wait_time[j] + t_cs + t_cs * processes[i].preemption[j]);
        total_burst_cb += 1;
        preempt_cb += processes[i].preemption[j];
      }
    }
  }
  double avg_cpu_burst = (cpu_burst_cb + cpu_burst_ib) / (total_burst_cb+total_burst_ib);
  double avg_wait_time = (wait_time_cb + wait_time_ib) / (total_burst_cb+total_burst_ib);
  double avg_turn_around = (tt_cb + tt_ib) / (total_burst_cb+total_burst_ib);
  
  char * buffer_cpu_burst = calloc(500, sizeof(char));
  sprintf(buffer_cpu_burst, "-- average CPU burst time: %.3f ms (%.3f ms/%.3f ms)\n", ceiling(avg_cpu_burst), ceiling(cpu_burst_cb/total_burst_cb), ceiling(cpu_burst_ib/total_burst_ib));
  fputs(buffer_cpu_burst, file);
  free(buffer_cpu_burst);

  char * buffer_wait_time = calloc(500, sizeof(char));
  sprintf(buffer_wait_time, "-- average wait time: %.3f ms (%.3f ms/%.3f ms)\n", ceiling(avg_wait_time), ceiling(wait_time_cb/total_burst_cb), ceiling(wait_time_ib/total_burst_ib));
  fputs(buffer_wait_time, file);
  free(buffer_wait_time);

  char * buffer_tt = calloc(500, sizeof(char));
  sprintf(buffer_tt, "-- average turnaround time: %.3f ms (%.3f ms/%.3f ms)\n", ceiling(avg_turn_around), ceiling(tt_cb/total_burst_cb), ceiling(tt_ib/total_burst_ib));
  fputs(buffer_tt, file);
  free(buffer_tt);

  char * cs = calloc(500, sizeof(char));
  sprintf(cs, "-- number of context switches: %d (%d/%d)\n", num_cs_cb+num_cs_ib, num_cs_cb, num_cs_ib);
  fputs(cs, file);
  free(cs);
  
  char * p = calloc(500, sizeof(char));
  if (strcmp(alg, "Algorithm RR\n") == 0) {
    sprintf(p, "-- number of preemptions: %d (%d/%d)", preempt_cb+preempt_ib, preempt_cb, preempt_ib);
  } else {
    sprintf(p, "-- number of preemptions: %d (%d/%d)", preempt_cb+preempt_ib, preempt_cb, preempt_ib);
  }
  free(alg);
  fputs(p, file);
  free(p);
  fclose(file);
}

double ceiling(double x) {
  return ceil(x * 1000) / 1000;
}

char * print_queue(int * Q, int q_size, char * queue) {
  if (q_size == 0) {
    queue = realloc(queue, 12*sizeof(char));
    strcpy(queue, "[Q <empty>]");
    queue[11] = '\0';
    return queue;
  } else {
    queue = realloc(queue, (3+q_size*2+1) * sizeof(char)); // 3 is for the characters [, Q, ] and 1 is for "\0", q_size*2 is for elements and white spaces before them
    queue[0] = '[';
    queue[1] = 'Q';
    queue[3+q_size*2-1] = ']';
    queue[3+q_size*2] = '\0';
    for (int i = 0; i < q_size; i++) {
      queue[2+(i*2)] = ' ';
      queue[2+(i*2+1)] = (char) Q[i]+65; // make sure Q is valid pointer
    }
    return queue;
  }
}

int * pop(int * Q, int q_size) { // q_size is the size of Q after removing an element
  if (q_size == 0) {
    free(Q);
    Q = NULL;
    return Q;
  }
  int q[q_size];
  for (int i = 1; i < q_size+1; i++) {
    q[i-1] = Q[i];
  }
  Q = realloc(Q, q_size * sizeof(int));
  for (int i = 0; i < q_size; i++) {
    Q[i] = q[i];
  }
  return Q;
}