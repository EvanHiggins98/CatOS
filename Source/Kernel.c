
/******************************************************************************
*******************************************************************************
ECE 270 –Fall2021
Evan Higgins
Kernel.c
create a TCB.
This file is the Kernel implementation
Homework #: 4
Copyright DigiPen (USA) Corporation
All Rights Reserved
*******************************************************************************
******************************************************************************/
/******************************************************************************
*******************************************************************************
  References:

  [1] RM0090 Reference manual 
  STM32F405/415, STM32F407/417, STM32F427/437 and STM32F429/439 
    advanced ARM®-based 32-bit MCUs

  [2] UM1670 User manual
  Discovery kit with STM32F429ZI MCU

  [3] STM32F427xx/STM32F429xx Datasheet
  ARM Cortex-M4 32b MCU+FPU, 225DMIPS, up to 2MB Flash/256+4KB RAM, USB
    OTG HS/FS, Ethernet, 17 TIMs, 3 ADCs, 20 comm. interfaces, camera & LCD-TFT

  [4] PM0214 Programming manual
  STM32F3, STM32F4 and STM32L4 Series
    Cortex®-M4 programming manual
    
  [5] Real-Time Operating Systems for
  ARM Cortex-M Microcontrollers

*******************************************************************************
******************************************************************************/

/*
        This code provides the Hardware Abstraction Layer (HAL) for the
    kernel.  This HAL only supports the STM32F429ZI microcontroller. 
*/

/******************************************************************************
*******************************************************************************
    Includes
*******************************************************************************
******************************************************************************/
#include 	"Kernel.h"

/******************************************************************************
*******************************************************************************
    Definitions
*******************************************************************************
******************************************************************************/

// Optional definitions not required to produce working code
#ifndef			MAX_WAIT
	#define		BON(X)			|=(X)
	#define		BOFF(X)			&=~(X)
	#define		BTOG(X)			^=(X)
	#define		MAX_WAIT		0xFFFF
#endif		//	MAX_WAIT

/******************************************************************************
*******************************************************************************
    Prototypes
*******************************************************************************
******************************************************************************/
void
OS_IdleTask(void);

/******************************************************************************
*******************************************************************************
    Declarations & Types
*******************************************************************************
******************************************************************************/
typedef	void (* OS_TaskAddress)(void);	

typedef enum {READY=OS_READY, RUNNING=OS_RUNNING, DONE=OS_DONE, BLOCKED=OS_BLOCKED} taskState_t;

typedef struct {
    unsigned int	*sp;
    unsigned int	taskID;
    taskState_t		taskState;
    OS_TaskAddress	taskAddress;
    unsigned int        priority;
    unsigned int        executionTime;
    unsigned int        time;
    unsigned int        deadline;
} TCB;	

typedef struct{
  TCB tcb;
  void *next;
}taskNode;

typedef struct{
  unsigned int size;
  unsigned int index;
}heapNode;

typedef struct{
  unsigned int ID;
  unsigned int type;
  int tokenCount;
  unsigned int tokenMax;
  int recurseCount;
  taskNode *owner;
  taskNode *waitingHead;
  taskNode *waitingTail;
}semaphore;

// Globals used by the system
taskNode        *readyListHead;
taskNode        *doneListHead;
taskNode        *doneListTail;
taskNode        Tasks[OS_MAX_TASKS];
semaphore       sems[OS_MAX_SEMS];
taskNode        *OS_TaskNEW;
taskNode        *OS_TaskRUNNING;
unsigned int    heap[OS_MAX_HEAP_SIZE];
heapNode        heapTree[OS_MAX_HEAP_SIZE];
int             heapMap[OS_MAX_HEAP_SIZE];
kernelErrors    lastErr = NO_ERROR;
unsigned char   OS_tickCount;
unsigned char 	newTaskID;
unsigned int    OS_numTasks;
unsigned int    OS_blockSize;
unsigned int    OS_blockMax;
unsigned int    OS_stackSize;
unsigned int 	Stacks[OS_MAX_TASKS][OS_MAX_STACK_SIZE];

/******************************************************************************
*******************************************************************************
    Helper Functions
*******************************************************************************
******************************************************************************/

/******************************************************************************
    OS_GetError
		
      Gets Error code
******************************************************************************/  
kernelErrors OS_GetError(void){
  return lastErr;
}

/******************************************************************************
    OSp_SetError
		
      Sets Error code
******************************************************************************/  
void OSp_SetError(kernelErrors err){
  lastErr=err;
}

unsigned int OSp_AllocateInit(unsigned int blockSize){
  if(blockSize==0||blockSize%8!=0||blockSize>OS_MAX_HEAP_SIZE/2||OS_MAX_HEAP_SIZE%blockSize!=0){
    OSp_SetError(HEAP_BAD_BLOCK_SIZE);
    return 0;
  }
  for(int i=1; i<OS_MAX_HEAP_SIZE; ++i){
    heapTree[i].index=0;
    heapTree[i].size=0;
    heapMap[i]=0;
  }
  OS_blockMax = OS_MAX_HEAP_SIZE/blockSize;
  heapTree[0].size = OS_blockMax;
  heapMap[0] = 0;
  heapMap[OS_blockMax-1] = 0;
  OS_blockSize = blockSize;
  return OS_blockMax;
}

void heapSwap(unsigned int h1, unsigned int h2){
  unsigned int temp;
  heapMap[heapTree[h1].index] = h2;
  heapMap[heapTree[h1].index + heapTree[h1].size-1] = h2;
  heapMap[heapTree[h2].index] = h1;
  heapMap[heapTree[h2].index + heapTree[h2].size-1] = h1;
  temp = heapTree[h1].index;
  heapTree[h1].index=heapTree[h2].index;
  heapTree[h2].index=temp;
  temp=heapTree[h1].size;
  heapTree[h1].size=heapTree[h2].size;
  heapTree[h2].size=temp;
}

unsigned int correctHeapDown(unsigned int index){
  unsigned int cur = index+1;
  while((cur*2-1<OS_blockMax &&
        heapTree[cur-1].size<heapTree[cur*2-1].size) ||
        (cur*2-1<OS_blockMax &&
        heapTree[cur-1].size<heapTree[cur*2].size)){
    if(!(cur*2-1<OS_blockMax)){
      heapSwap(cur-1, cur*2);
      cur = cur*2+1;
    }else if(!(cur*2<OS_blockMax)){
      heapSwap(cur-1, cur*2-1);
      cur = cur*2;
    }else if(heapTree[cur*2-1].size<heapTree[cur*2].size){
      heapSwap(cur-1, cur*2);
      cur = cur*2+1;
    }else{
      heapSwap(cur-1, cur*2-1);
      cur = cur*2;
    }
  }
  return cur;
}

void* OS_Malloc(unsigned int size){
  OS_Critical_Begin();
  if(size==0){
    OSp_SetError(ALLOC_BAD_SIZE);
    OS_Critical_End();
    return 0;
  }
  if(heapTree[0].size==0){
    OSp_SetError(ALLOC_NO_MEM);
    OS_Critical_End();
    return 0;
  }
  if(OS_blockSize%size!=0){
    size += (OS_blockSize - (size%OS_blockSize));
  }

  size/=OS_blockSize;
  if(heapTree[0].size<=size){
    OSp_SetError(ALLOC_NO_MEM);
    OS_Critical_End();
    return 0;
  }
  heapMap[heapTree[0].index] = -size;
  heapMap[heapTree[0].index + size-1] = -size;
  void* ptr = heap+heapTree[0].index;
  heapTree[0].index+=size;
  heapTree[0].size-=size;
  correctHeapDown(0);
  OS_Critical_End();
  return ptr;
}

unsigned int correctHeapUp(unsigned int index){
  if(index==0)
    return 1;
  unsigned int cur = index+1;
  while(heapTree[cur-1].size>heapTree[(cur/2)>0?(cur/2-1):0].size){
    heapSwap(cur-1,(cur/2-1)>0?cur/2-1:0);
    cur=cur/2;
  }
  return cur;
}

unsigned int OS_Free(void* ptr){
  OS_Critical_Begin();
  if(!ptr || (int)ptr<(int)heap || (int)ptr>(int)heap+OS_blockMax*OS_blockSize){
    OSp_SetError(FREE_INVALID);
    return 0;
  }
  unsigned int index = ((int)ptr-(int)heap)/OS_blockSize;
  int size = -heapMap[index];
  unsigned int combinationFound = 0x0;
  unsigned int newSize=size;

  if(index>0 && heapMap[index-1]>=0){
    combinationFound|=0x1;
    newSize+=heapTree[heapMap[index-1]].size;
  }
  if(index+size<OS_blockMax && heapMap[index+size]>=0){
    combinationFound|=0x2;
    newSize+=heapTree[heapMap[index+size]].size;
  }

  if(combinationFound==0x3){
    heapTree[heapMap[index-1]].size=newSize;
    heapTree[heapMap[index+size]].size=0;
    heapMap[index+size+(heapTree[heapMap[index+size]].size)-1]=heapMap[index-1];
    correctHeapDown(heapMap[index+size]);
    correctHeapUp(heapMap[index-1]);
    heapMap[index+size] = 0;
    heapMap[index-1] = 0;
    heapMap[index]=0;
    heapMap[index+size-1]=0;
  }else if(combinationFound==0x2){
    heapTree[heapMap[index+size]].index = index;
    heapTree[heapMap[index+size]].size = newSize;
    heapMap[index] = heapMap[index+size];
    correctHeapUp(heapMap[index]);
    heapMap[index+size] = 0;
    heapMap[index+size-1]=0;
  }else if(combinationFound==0x1){
    heapTree[heapMap[index-1]].size = newSize;
    heapMap[index+size-1] = heapMap[index-1];
    correctHeapUp(heapMap[index-1]);
    heapMap[index-1] = 0;
    heapMap[index]=0;
  }else{
    heapTree[OS_blockMax-1].size = size;
    heapTree[OS_blockMax-1].index = index;
    heapMap[index]=OS_blockMax-1;
    heapMap[index+size]=OS_blockMax-1;
    correctHeapUp(OS_blockMax-1);
  }
  OS_Critical_End();
  return (unsigned int)ptr;
}

/******************************************************************************
    OS_InitKernel
		
      Prepares the Kernel for use, but does not start any services.  No OS_
    function should be called until after this one has executed.
******************************************************************************/    
unsigned int    
OS_InitKernel(unsigned int numTasks, unsigned int stackSize) {
    if(numTasks>OS_MAX_TASKS){
      OSp_SetError(TASK_MAX_TOO_LARGE);
      return 0;
    }
    if(stackSize>OS_MAX_STACK_SIZE){
      OSp_SetError(STACK_SIZE_TOO_LARGE);
      return 0;
    }
    if(OSp_AllocateInit(8)==0){
      return 0;
    }
    OS_numTasks=numTasks;
    OS_stackSize=stackSize;
    OS_InitKernelHAL();
    OS_CreateTask(OS_IdleTask,0,0xFFFFFFFF,0xFFFFFFFF);
    readyListHead = 0;
    OS_TaskRUNNING = &(Tasks[0x00]);
    OS_tickCount=0;
    return 1;
    
} // end OS_InitKernel

/******************************************************************************
    OSp_TaskListInstert
		
      Moves task to the readylist.
******************************************************************************/
void OSp_TaskListInstert(taskNode *node){
  //does node belong on front?
  if(readyListHead && //tasklist exists
    (node->tcb.deadline > readyListHead->tcb.deadline||//deadline is later than head
    (node->tcb.deadline==readyListHead->tcb.deadline && node->tcb.priority<readyListHead->tcb.priority))){ //deadlines are equal and priority is less than head
      taskNode *curNode = readyListHead;
      taskNode *nextNode = readyListHead->next;
      //use cudNode->next in case need to add to end of the list
      while(curNode->next && (node->tcb.deadline > nextNode->tcb.deadline || //deadline is after next
            (node->tcb.deadline==nextNode->tcb.deadline && node->tcb.priority < nextNode->tcb.priority))){//deadline is equal but priority is less than current.
          curNode = curNode->next;
          nextNode=curNode->next;
      }
      node->next=curNode->next;
      curNode->next=node;//add node where it belongs
  }else{//node belongs on front
    node->next = readyListHead;
    readyListHead = node;
  }
}

/******************************************************************************
    OS_TaskCreate
		
      Takes the assigned function pointer and uses it to create a kernel task
    that is ready for execution.
******************************************************************************/
unsigned int
OS_CreateTask(void (* newTask)(void), unsigned int priority, unsigned int executionTime,unsigned int deadline) {

    if(newTaskID>OS_MAX_TASKS)
    {
        OSp_SetError(TASK_MAX_REACHED);
        return 0;
    }
    static unsigned int newTaskID = 0x00;

    Tasks[newTaskID].tcb.taskAddress=newTask;

    Tasks[newTaskID].tcb.taskID = newTaskID;

    Tasks[newTaskID].tcb.taskState=READY;

    Stacks[newTaskID][OS_stackSize-1]=OS_STACK_MARKER;
    Stacks[newTaskID][OS_stackSize-2]=0x01000000;
    Stacks[newTaskID][OS_stackSize-3]=(unsigned int)newTask;
    Stacks[newTaskID][OS_stackSize-4]=0x13131313;
    Stacks[newTaskID][OS_stackSize-5]=0x12121212;
    Stacks[newTaskID][OS_stackSize-6]=0x03030303;
    Stacks[newTaskID][OS_stackSize-7]=0x02020202;
    Stacks[newTaskID][OS_stackSize-8]=0x01010101;
    Stacks[newTaskID][OS_stackSize-9]=0x00000000;
    Stacks[newTaskID][OS_stackSize-10]=0x11111111;
    Stacks[newTaskID][OS_stackSize-11]=0x10101010;
    Stacks[newTaskID][OS_stackSize-12]=0x09090909;
    Stacks[newTaskID][OS_stackSize-13]=0x08080808;
    Stacks[newTaskID][OS_stackSize-14]=0x07070707;
    Stacks[newTaskID][OS_stackSize-15]=0x06060606;
    Stacks[newTaskID][OS_stackSize-16]=0x05050505;
    Stacks[newTaskID][OS_stackSize-17]=0x04040404;
    Tasks[newTaskID].tcb.sp = &(Stacks[newTaskID][OS_stackSize-0x11]);

    Tasks[newTaskID].tcb.priority=priority;

    Tasks[newTaskID].tcb.executionTime=executionTime;

    Tasks[newTaskID].tcb.time=0;

    Tasks[newTaskID].tcb.deadline=deadline;
    OSp_TaskListInstert(&Tasks[newTaskID]);
    ++newTaskID;
    return 1;

} // end OS_TaskCreate

/******************************************************************************
    OS_IdleTask
		
      This task should always be created, have the lowest possible (worst) 
    priority, and never be prevented from running.
******************************************************************************/
void
OS_IdleTask(void) {
    while (0x01) {
      ;
    } // end while

} // end OS_IdleTask

void OSp_updateDoneTasks(){
  ++OS_tickCount;
  taskNode *curNode = doneListHead;
  taskNode *prevNode = 0;
  while(curNode){
    if(OS_tickCount%curNode->tcb.deadline==0){
      curNode->tcb.taskState = READY;
      if(doneListHead==curNode){
        doneListHead=doneListHead->next;
      }

      if(doneListTail==curNode){
        doneListTail=prevNode;
      }

      if(prevNode){
        prevNode->next=curNode->next;
        OSp_TaskListInstert(curNode);
        curNode=prevNode->next;
      }else{
        OSp_TaskListInstert(curNode);
        curNode=doneListHead;
      }
    }else{
      prevNode=curNode;
      curNode=curNode->next;
    }
  }
}

/******************************************************************************
    OSp_TaskDone
		
      Moves task to the done list to wait for deadline.
******************************************************************************/
void OSp_TaskDone(taskNode *node){
  node->tcb.taskState = DONE;
  if(doneListHead){//if doneListHead exists
    doneListTail->next=node;//add to end
    doneListTail=doneListTail->next;//move tail
    node->next = 0;//remove cross contamination
  }else{//no nodes in donelist
    doneListHead=node;//set donelist to current.
    doneListTail=doneListHead;//set tail
    node->next = 0;//remove cross contamination
  }
}

unsigned int
OSp_ScheduleTask(void){
  OSp_updateDoneTasks();//refresh any done tasks past deadline
  if(!doneListHead && doneListTail){
    lastErr=0;
  }
  if(OS_TaskRUNNING->tcb.taskState==BLOCKED){
    OS_TaskNEW=readyListHead;
    readyListHead = readyListHead->next;
    OS_TaskNEW->next=0;
    return 1;
  }else if(++(OS_TaskRUNNING->tcb.time)>=OS_TaskRUNNING->tcb.executionTime){//update time and check if its done
    OS_TaskRUNNING->tcb.time=0;//reset time
    OS_TaskNEW=readyListHead;//set new task
    OS_TaskNEW->tcb.taskState = RUNNING;
    readyListHead = readyListHead->next;//move task head
    OS_TaskNEW->next=0;
    OSp_TaskDone(OS_TaskRUNNING);//insert old task
    return 1;
  }else if((readyListHead) &&
          (readyListHead->tcb.deadline < OS_TaskRUNNING->tcb.deadline || //a deadline is sooner
          (readyListHead->tcb.deadline == OS_TaskRUNNING->tcb.deadline && readyListHead->tcb.priority>OS_TaskRUNNING->tcb.priority))){//a deadline is equal but higher priority
    OS_TaskNEW=readyListHead;//set new task
    readyListHead = readyListHead->next;//move task head
    OS_TaskNEW->next=0;
    OSp_TaskListInstert(OS_TaskRUNNING);//insert old task.
    return 1;
  }
  return 0;
}

unsigned int OS_SemCreate(unsigned int type, unsigned int tokenStart, unsigned int tokenMax){
  static unsigned int semNum = 0x00;
  if(type!=MUTEX && type!=COUNTING){
    OSp_SetError(SEM_UNKNOWN_TYPE);
    return 0;
  }
  if(tokenMax==0){
    OSp_SetError(SEM_ZERO_TOKEN);
    return 0;
  }
  if(tokenStart>tokenMax){
    OSp_SetError(SEM_TOO_MANY_TOKENS);
    return 0;
  }
  if(semNum>=OS_MAX_SEMS){
    OSp_SetError(SEM_COUNT_MAX);
    return 0;
  }
  
  sems[semNum].ID = semNum+1;
  sems[semNum].type = type;
  sems[semNum].tokenCount = tokenStart;
  sems[semNum].tokenMax = tokenMax;
  sems[semNum].waitingHead = 0;
  sems[semNum].waitingTail=0;
  ++semNum;
  return semNum;
}

void OSp_BlockTask(unsigned int SemID, taskNode* blockedTask){
  blockedTask->tcb.taskState = BLOCKED;
  if(sems[SemID].waitingHead){
    sems[SemID].waitingTail->next = blockedTask;
    sems[SemID].waitingTail = sems[SemID].waitingTail->next;
    sems[SemID].waitingTail->next = 0;
  }else{
    sems[SemID].waitingHead = blockedTask;
    sems[SemID].waitingTail = sems[SemID].waitingHead;
    sems[SemID].waitingTail->next = 0;
  }
}

unsigned int OS_SemAcquire(unsigned int SemID){
  OS_Critical_Begin();
  --SemID;
  if(sems[SemID].type == COUNTING){
    if(sems[SemID].tokenCount<=0){
      OSp_BlockTask(SemID, OS_TaskRUNNING);
      OS_Critical_End();
      while(OS_TaskRUNNING->tcb.taskState==BLOCKED){};
      OS_Critical_Begin();
    }
    --(sems[SemID].tokenCount);
    OS_Critical_End();
    return SemID+1;
  }else{
    if(sems[SemID].owner){
      if(sems[SemID].owner == OS_TaskRUNNING){
        ++sems[SemID].recurseCount;
        return SemID+1;
      }else{
        OSp_BlockTask(SemID, OS_TaskRUNNING);
        OS_Critical_End();
        while(OS_TaskRUNNING->tcb.taskState==BLOCKED){};
        OS_Critical_Begin();
      }
    }
    sems[SemID].owner = OS_TaskRUNNING;
    sems[SemID].recurseCount=0;
    ++sems[SemID].recurseCount;
    OS_Critical_End();
    return SemID+1;
  }
  OS_Critical_End();
}

void OSp_UnblockTask(unsigned int SemID, taskNode* unblockedTask){
  unblockedTask->tcb.taskState = READY;
  if(sems[SemID].waitingTail == sems[SemID].waitingHead){
    sems[SemID].waitingTail = sems[SemID].waitingTail->next;
  }
  sems[SemID].waitingHead = sems[SemID].waitingHead->next;
  OSp_TaskListInstert(unblockedTask);
}

unsigned int OS_SemRelease(unsigned int SemID){
  OS_Critical_Begin();
  --SemID;
  if(sems[SemID].type==COUNTING){
    if(sems[SemID].waitingHead){
      OSp_UnblockTask(SemID, sems[SemID].waitingHead);
    }else if(sems[SemID].tokenCount>=sems[SemID].tokenMax){
      OSp_SetError(SEM_TOKEN_MAX_REACHED);
      OS_Critical_End();
      return 0;
    }else{
      ++sems[SemID].tokenCount;
    }
    OS_Critical_End();
    return 1;
  }else{
    if(sems[SemID].owner==OS_TaskRUNNING){
      --sems[SemID].recurseCount;
      if(sems[SemID].recurseCount<=0){
        if(sems[SemID].waitingHead){
          OSp_UnblockTask(SemID, sems[SemID].waitingHead);
        }else{
          sems[SemID].owner=0;
        }
      }
    }else{
      OSp_SetError(SEM_INCORRECT_OWNER);
      OS_Critical_End();
      return 0;
    }
    OS_Critical_End();
    return 1;
  }
  OS_Critical_End();
}



// EOF    Kernel.c
// Note: Some IDEs generate warnings if a file doesn't end in whitespace,
//  but Embedded Studio doesn't seem to be one of them.