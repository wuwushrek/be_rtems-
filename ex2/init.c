/* Standard C header files */
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

/* RTEMS header files */
#include <rtems.h>
#include <bsp.h>
#include <rtems/bspIo.h>
#include <rtems/cpuuse.h> 
 
/* Functions */
rtems_task Init(rtems_task_argument argument);
rtems_task Task_1(unsigned int argument);
rtems_task Task_2(unsigned int argument);
rtems_task Task_3(unsigned int argument);

/****************************************************************************/
/* RTEMS resources configuration                                            */
/****************************************************************************/

#define MAX_TASKS 3
#define MAX_PERIODS 100 

/* RTEMS system configuration informations */
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_MAXIMUM_TASKS              9
#define CONFIGURE_EXTRA_TASK_STACKS         (3 * RTEMS_MINIMUM_STACK_SIZE)
#define CONFIGURE_MAXIMUM_PERIODS            10000
#define CONFIGURE_MICROSECONDS_PER_TICK      100 
#define CONFIGURE_INIT

#include <rtems/confdefs.h>

/****************************************************************************/
/* The following is used to waste some CPU time, roughly 100 ms with
 * tsim-leon3.  
 * Note: this requires optimization level of -O0 wih gcc
 */
#define TEMPO 0x000fff
#define WASTE_CPU for (i =0 ; i < TEMPO; i++)

/******************************************************************************/
/* Global variables                                                           */
/******************************************************************************/

rtems_name          Task_name[9]      = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
rtems_task_priority Task_Priority[9]  = { 0, 21, 22, 23, 0, 0, 0, 0, 0 };
rtems_id            Task_id[9]        = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
rtems_name          Semaphore_name[2] = { 0, 0};
rtems_id            Semaphore_id[2]   = { 0, 0};
unsigned char       Task_done[9]      = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
unsigned char       notify_task       = 0;

#define RMS_WORKLOAD 32000

/******************************************************************************/
/* Utility function that represents a job to achieve by a task                         */
/******************************************************************************/

void job_to_achieve(unsigned int max_total_passes){
  unsigned int myCounter, current_total_passes;
  while(1){
    for(myCounter =0 ; myCounter<RMS_WORKLOAD; myCounter++){};
    current_total_passes++;
    if (current_total_passes>= max_total_passes) break;
  }
}

/******************************************************************************/
/* Task #1 (high priority)                                                    */
/******************************************************************************/

rtems_task Task_1( unsigned int waste_time_tick) {
  rtems_status_code status;
  rtems_mode Previous_Mode;
  unsigned long i;

  printf("Task1 is starting ...\n");

  /* Set preemption */
  status = rtems_task_mode(RTEMS_PREEMPT, RTEMS_PREEMPT_MASK, &Previous_Mode); 
  if(status != RTEMS_SUCCESSFUL)
    printf("main-- rtems_task_mode failed\n");

  /* Wait until T3 is in his critical section by Trying to acquire SM2 */
  //rtems_semaphore_obtain(Semaphore_id[1],RTEMS_WAIT, RTEMS_NO_TIMEOUT);
  while (notify_task == 0) rtems_task_wake_after(100);
  printf("T3 -> T1 && ! P(T1,SM1)\n");
  /* Try to acquire SM1 */
  rtems_semaphore_obtain(Semaphore_id[0],RTEMS_WAIT,RTEMS_NO_TIMEOUT);
  printf("P(T1,SM1)\n");
  /* Waste some time */
  WASTE_CPU;
  printf("! V(T1,SM1)\n");
  /* Release SM1 */
  rtems_semaphore_release(Semaphore_id[0]);
  printf("V(T1,SM1)\n");
  /* Waste some time */
  WASTE_CPU;
  /* Achieve a job */
  job_to_achieve(waste_time_tick);
  /* Notify main task for termination */
  Task_done[1] = 1;
  printf("job T1 achieved ...\n");

  rtems_task_suspend(RTEMS_SELF);
  rtems_task_delete(RTEMS_SELF);
}

/******************************************************************************/
/* Task #2 (medium priority)                                                  */
/******************************************************************************/

rtems_task Task_2( unsigned int waste_time_tick) {
  rtems_status_code status;
  rtems_mode Previous_Mode;
  unsigned long i;

  printf("Task2 is starting ...\n");

  /* Set preemption */
  status = rtems_task_mode(RTEMS_PREEMPT, RTEMS_PREEMPT_MASK, &Previous_Mode); 
  if(status != RTEMS_SUCCESSFUL)
    printf("main-- rtems_task_mode failed\n");

  /* Wait until T3 is in his critical section by Trying to acquire SM2 */
  //status = rtems_semaphore_obtain(Semaphore_id[1],RTEMS_WAIT, RTEMS_NO_TIMEOUT);
  while (notify_task == 0) rtems_task_wake_after(100);
  printf("T3 -> T2 \n");
  /* Waste some time */
  WASTE_CPU;
  /* Achieve a job */
  job_to_achieve(waste_time_tick);
  /* Notify main task for termination */
  Task_done[2] = 1;
  printf("job T2 achieved ...\n");

  rtems_task_suspend(RTEMS_SELF);
  rtems_task_delete(RTEMS_SELF);
}

/******************************************************************************/
/* Task #3 (low priority)                                                     */
/******************************************************************************/

rtems_task Task_3( unsigned int waste_time_tick) {
  rtems_status_code status;
  rtems_mode Previous_Mode;
  unsigned long i;

  printf("Task3 is starting ...\n");

  /* Set preemption */
  status = rtems_task_mode(RTEMS_PREEMPT, RTEMS_PREEMPT_MASK, &Previous_Mode); 
  if(status != RTEMS_SUCCESSFUL)
    printf("main-- rtems_task_mode failed\n");

  /* Try to acquire SM1 */
  printf("! P(T3,SM1)\n");
  rtems_semaphore_obtain(Semaphore_id[0],RTEMS_WAIT,RTEMS_NO_TIMEOUT);
  printf("P(T3,SM1)\n");
  /* Notify T1 and T2 that we are in the critical section */
  //rtems_semaphore_release(Semaphore_id[1]);
  //rtems_semaphore_release(Semaphore_id[1]);
  notify_task = 1;
  /* Waste some time */
  WASTE_CPU;
  printf("! V(T3,SM1)\n");
  /* Release SM1 */
  rtems_semaphore_release(Semaphore_id[0]);
  printf("V(T3,SM1)\n");
  /* Waste some time */
  WASTE_CPU;
  /* Achieve a job */
  job_to_achieve(waste_time_tick);
  /* Notify main task for termination */
  Task_done[3] = 1;
  printf("job T3 achieved ...\n");

  rtems_task_suspend(RTEMS_SELF);
  rtems_task_delete(RTEMS_SELF);
}

/****************************************************************************/
/* Init(): main entrypoint                                                  */
/****************************************************************************/

rtems_task Init( rtems_task_argument argument)
{
  rtems_status_code status;
  rtems_task_priority the_priority;
  rtems_time_of_day time;
  int j;
  unsigned char Main_Done;
  rtems_mode Previous_Mode;
  
  status=rtems_task_wake_after(1000);  /* wait about 100 ms */
  
  rtems_cpu_usage_reset(); 
  rtems_cpu_usage_report();  

  /* get clock value */
  status = rtems_clock_get( RTEMS_CLOCK_GET_TOD, &time );    

  /* Set current task priority to 10 */
  status = rtems_task_set_priority(RTEMS_SELF, 10, &the_priority);
  status = rtems_task_set_priority
    (RTEMS_SELF, RTEMS_CURRENT_PRIORITY, &the_priority);
  printf("actual current priority:%d\n", (int)the_priority);
  
  /* Create semaphore SM1 */
  //RTEMS_DEFAULT_ATTRIBUTES | RTEMS_BINARY_SEMAPHORE 
  //RTEMS_PRIORITY | RTEMS_BINARY_SEMAPHORE | RTEMS_PRIORITY_CEILING | RTEMS_LOCAL
  Semaphore_name[0] = rtems_build_name( 'S', 'M', '0', ' ' );
  status = rtems_semaphore_create(Semaphore_name[0], 1 ,RTEMS_PRIORITY | RTEMS_BINARY_SEMAPHORE | RTEMS_PRIORITY_CEILING | RTEMS_LOCAL,Task_Priority[1] , &Semaphore_id[0]);
  printf("Semaphore (0) creation status code : %d \n", (int) status);

  /* Create semaphores SM2 easy achievement of precedence constraints */
  //Semaphore_name[1] = rtems_build_name('S', 'M', '1',' ');
  //status = rtems_semaphore_create(Semaphore_name[1], 0 ,RTEMS_DEFAULT_ATTRIBUTES, Task_Priority[1],&Semaphore_id[1]);
  //printf("Semaphore (%d) creation status code : %d \n", 1 ,(int) status);

  /* Tasks initialization */
  for ( j=0 ; j<MAX_TASKS; j++){
    Task_name[j+1] = rtems_build_name('T', 'A', 'S', '0'+(j+1));
    status = rtems_task_create(Task_name[j+1],Task_Priority[j+1],RTEMS_MINIMUM_STACK_SIZE,RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES,&Task_id[j+1]); 
    printf("main-- Task nb. %d created, status = %d, priority = %d, id=%x\n",j+1,(int) status, (int) Task_Priority[j+1], (int) Task_id[j+1]);    
  }

  /* Tasks launching */
  rtems_task_start(Task_id[1], (rtems_task_entry) Task_1, 10);
  rtems_task_start(Task_id[2], (rtems_task_entry) Task_2, 10);
  rtems_task_start(Task_id[3], (rtems_task_entry) Task_3, 10);
  
  /* Set preemption */
  status = rtems_task_mode(RTEMS_PREEMPT, RTEMS_PREEMPT_MASK, &Previous_Mode); 
  if(status != RTEMS_SUCCESSFUL)
    printf("main-- rtems_task_mode failed\n"); 

  /* Lower main task  priority so others tasks begin running  */
  status = rtems_task_set_priority(RTEMS_SELF, 40, &the_priority);
  printf("main -- current priority : %d, is set to (40) : \n", 
	 (int)the_priority);
  
  /* Resume the task and wait for their completion then delete them */
  while(1){
    Main_Done = 1;
    for (j=0 ; j< MAX_TASKS; j++){
      if(Task_done[j+1] == 0){
        Main_Done = 0;
      }
    }
    if (Main_Done == 1) break;
  }
  for (j = 0; j < MAX_TASKS; j++){
    printf("main-- Job done for task %d\n", j+1);
    status = rtems_task_resume(Task_id[j+1]);
  }
  for (j = 0; j < MAX_TASKS; j++)
    rtems_task_delete(Task_id[j+1]);

  /* Poper delete of the two Semaphore */
  rtems_semaphore_delete(Semaphore_id[0]);
  //rtems_semaphore_delete(Semaphore_id[1]);

  /* This will print when other tasks are finished */
  status = rtems_task_set_priority
    (RTEMS_SELF, RTEMS_CURRENT_PRIORITY, &the_priority);
  printf("main -- current priority : %d\n", (int)the_priority);
  
  printf("main (init) : exit\n");

  //Report the CPU usage
  rtems_cpu_usage_report();

  rtems_task_delete(RTEMS_SELF);
}   /* Init end */