/* Standard C header files */
#include <stdlib.h> 
#include <assert.h> 
#include <stdio.h> 
#include <stdlib.h> 

/* RTEMS header files */
#include <bsp.h> 
#include <rtems.h>
#include <rtems/bspIo.h> 
#include <rtems/cpuuse.h> 

/* Functions */
rtems_task Init(rtems_task_argument argument);
rtems_task The_Task(unsigned int Task_num);

/****************************************************************************/
/* RTEMS resources configuration                                            */
/****************************************************************************/

#define MAX_TASKS 5
#define MAX_PERIODS 100 

/* RTEMS system configuration informations */
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER 
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_MAXIMUM_TASKS          9
#define CONFIGURE_EXTRA_TASK_STACKS     (3 * RTEMS_MINIMUM_STACK_SIZE)
#define CONFIGURE_MAXIMUM_PERIODS        10000
#define CONFIGURE_MICROSECONDS_PER_TICK  100

#define CONFIGURE_INIT
#include <rtems/confdefs.h>

/****************************************************************************/
/* Task configuration                                                       */
/****************************************************************************/

rtems_task_priority Priorities[9] = 
  {0, 20, 21, 22, 23, 24, 25, 26, 27}; 
int                 Periods[9]    = 
  {0, 50, 200, 300, 400, 500, 600, 700, 800}; 
rtems_id            Task_id[9]    = {0, 1, 2, 3, 4, 5, 0, 0, 0};    
rtems_name          Task_name[9]  = {0, 0, 0, 0, 0, 0, 0, 0, 0};
int                 Done[9]       = {0, 0, 0, 0, 0, 0, 0, 0, 0}; 
int                 Periods_Done[9]; 
int                 Total_Passes  = 0; 

#define MAX_TOTAL_PASSES 1000
#define RMS_WORKLOAD 32000

/****************************************************************************/
/* The_Task(): body of the task                                             */
/****************************************************************************/

rtems_task The_Task(unsigned int Task_num){ 
  rtems_id          RMS_Id; 
  rtems_status_code Status; 
  rtems_mode        Previous_Mode; 
  int               My_Counter;
  int               My_Period_Passes = 0;
  int               My_Num           = Task_num;
  
  printf ("Task %d-- : starting\n", My_Num); 

  /* Set preemption */ 
  Status = rtems_task_mode(RTEMS_PREEMPT, RTEMS_PREEMPT_MASK, &Previous_Mode); 
  if (Status != RTEMS_SUCCESSFUL) 
    printf("Task %d : rtems_task_mode failed\n", My_Num); 

  /* Create RMS control block */
  rtems_name curr_name = rtems_build_name('R','M','S','0'+Task_num);
  Status = rtems_rate_monotonic_create(curr_name, &RMS_Id);
  if (Status != RTEMS_SUCCESSFUL ){
    printf("rtems_monotonic_create failed with status of %d. \n",Status);
    exit(1);
  } 
  while (1){ /* RMS  loop */ 
    // Wait and check if deadline is missed
    if (rtems_rate_monotonic_period(RMS_Id,Periods[Task_num]) == RTEMS_TIMEOUT){
      printf("Task %d, TIMEOUT \n",Task_num);
      break;
    }
    /* Work to be done (RMS_WORKLOAD is C coefficient)*/ 
    for (My_Counter = 0; My_Counter <= RMS_WORKLOAD; My_Counter++) {};  
    My_Period_Passes++;  
    Total_Passes++; 
    if (Total_Passes >= MAX_TOTAL_PASSES) break; 
  }  /* RMS loop end */ 

  Periods_Done[Task_num] = My_Period_Passes; 

  /* Cancel and delete the RMS block */
  Status = rtems_rate_monotonic_delete(RMS_Id);
  if (Status != RTEMS_SUCCESSFUL){
    printf("rtems_rate_monotonic_delete failed with status of %d. \n", Status);
    exit(1);
  }

  // rtems rate monotonic statistics
  rtems_rate_monotonic_report_statistics();

  // Tell the main task that this one terminated 
  Done[Task_num] = 1;
  rtems_task_suspend(RTEMS_SELF);
  rtems_task_delete(RTEMS_SELF); 
}

/****************************************************************************/
/* Init(): main entrypoint                                                  */
/****************************************************************************/

rtems_task Init( rtems_task_argument ignored ){ 
  rtems_status_code    status; 
  rtems_task_priority  the_priority; 
  rtems_time_of_day    time; 
  rtems_mode           Previous_Mode; 
  rtems_interval       ticks_per_second; 

  int j;  
  int Main_Done; 
	
  /* Clock value */ 
  time.year   = 2011; 
  time.month  = 10; 
  time.day    = 11; 
  time.hour   = 9; 
  time.minute = 0; 
  time.second = 0; 
  time.ticks  = 0; 

  status = rtems_clock_set( &time );  
  
  printf ("main-- starting\n"); 

  rtems_clock_get(RTEMS_CLOCK_GET_TICKS_PER_SECOND, &ticks_per_second); 
  printf("main-- ticks_per_second : %d\n", (int)ticks_per_second); 

  status = rtems_task_set_priority(RTEMS_SELF, 50 , &the_priority); 

  /* Check current priority  */ 
  status = rtems_task_set_priority
    (RTEMS_SELF, RTEMS_CURRENT_PRIORITY, &the_priority); 
  printf("main-- initial current priority : %d\n", (int)the_priority); 

  /* Change current task priority  */ 
  status = rtems_task_set_priority(RTEMS_SELF, 10, &the_priority); 

  /* Check new current priority  */ 
  status = rtems_task_set_priority
    (RTEMS_SELF, RTEMS_CURRENT_PRIORITY, &the_priority); 
  printf("main-- actual current priority:%d\n", (int)the_priority); 
  
  /* Create tasks with right parameters */ 
  for (j = 0; j < MAX_TASKS; j++){ 
    Task_name[j+1] = rtems_build_name('T','A','S','0'+(j+1));
    status = rtems_task_create(Task_name[j+1],Priorities[j+1],RTEMS_MINIMUM_STACK_SIZE,RTEMS_DEFAULT_MODES,RTEMS_DEFAULT_ATTRIBUTES,&Task_id[j+1]);    
    rtems_task_start(Task_id[j+1],(rtems_task_entry) The_Task,j+1);    
    printf("main-- Task nb. %d created, status = %d, priority = %d, id = %x\n",
	   j+1, (int)status, (int)Priorities[j+1], (int)Task_id[j+1]); 
  } 

  status = rtems_task_mode(RTEMS_PREEMPT, RTEMS_PREEMPT_MASK, &Previous_Mode); 
  if(status != RTEMS_SUCCESSFUL)
    printf("main-- rtems_task_mode failed\n"); 

  
  status = rtems_clock_get(RTEMS_CLOCK_GET_TOD, &time); 
  printf("main-- start time(s) : %d\n", (int)time.second);

  /* Set current task priority lower than RMS tasks priorities,
   * so they begin running right now 
   */
  status = rtems_task_set_priority(RTEMS_SELF, 50, &the_priority);
  printf("main-- current priority : %d, set to 50\n", (int)the_priority);
  
  /* Wait until all the task terminated their job before shutting down
   * the main task
   */
  while(1){
    Main_Done = 1;
    for (j=0 ; j< MAX_TASKS; j++){
      if(Done[j+1] == 0){
        Main_Done = 0;
      }
    }
    if (Main_Done == 1) break;
  }

  /* Resume and delete the created tasks */
  for (j = 0; j < MAX_TASKS; j++){
    printf("main-- Periods done for task %d : %d\n", j+1, Periods_Done[j+1]);
    status = rtems_task_resume(Task_id[j+1]);
  }
  for (j = 0; j < MAX_TASKS; j++)
    rtems_task_delete(Task_id[j+1]);
  
  /* Exit main task and do cpu usage report */
  printf("main-- exiting\n");
  rtems_cpu_usage_report();
  rtems_task_delete(RTEMS_SELF);
}
