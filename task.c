#include "task.h"
#include "clock.h"
#include "str.h"
#include "int.h"
#include "ap.h"

Task Tasks[MaxTaskCount]={0};
Bool InTask=0;

u16 TaskAdd(Task AddTask, Bool SystemTask){
    u16 StartIdx = SystemTask ? SystemTaskStart : UserTaskStart;
    u16 EndIdx = SystemTask ? UserTaskStart : MaxTaskCount;
    
    for (u16 i = StartIdx; i < EndIdx; i++){
        if (!Tasks[i].Active) {
            MemCopy(&Tasks[i], &AddTask, sizeof(Task));
            Tasks[i].Active = true;
            return i;
        }
    }
    return (u16)-1;
}

void TaskDel(u16 IdxTask){ 
    if (ERROR(IdxTask)) return;
    Tasks[IdxTask].Active = false;
    if (Tasks[IdxTask].DelFunc) Tasks[IdxTask].DelFunc(Tasks[IdxTask].Arg);
}

void TaskInit(){
    for (u16 i = 0; i < MaxTaskCount; i++){
        Tasks[i].Active=0;
        Tasks[i].Arg=NULL;
        Tasks[i].CallFunc=NULL;
        Tasks[i].IntervalNs=0;
        Tasks[i].Name=NULL;
        Tasks[i].CallFunc=NULL;
        Tasks[i].DelFunc=NULL;
        Tasks[i].NextWaitNs=0;
        Tasks[i].Target=NULL;
    }
}

void TaskPoll(void) {
    InTask=1;
    u64 Now = SystemGetTimeNano();
    
    for (u16 i = 0; i < MaxTaskCount; i++){
        if (!Tasks[i].Active) continue;
        
        if (Now >= Tasks[i].NextWaitNs){
            if (Tasks[i].CoreID==0){
                if (Tasks[i].CallFunc) {
                    Tasks[i].Target=Tasks[i].CallFunc(Tasks[i].Arg);
                }
                
                if (Tasks[i].IntervalNs > 0){
                    Tasks[i].NextWaitNs = Now + Tasks[i].IntervalNs;
                }
                else{
                    Tasks[i].Active = false;
                    if (Tasks[i].DelFunc) Tasks[i].DelFunc(Tasks[i].Arg);
                }
            }else{
                u64 Core=Tasks[i].CoreID%GetActualCoreCount();
                if (Tasks[i].CallFunc && (Core!=0)) {
                    APCallFunc[Core]=Tasks[i].CallFunc;
                    APCallFuncArg[Core]=Tasks[i].Arg;
                    APCallFuncTarget[Core]=&Tasks[i].Target;
                    __asm__ volatile("sfence" : : : "memory");
                    if (APAPICIDsBusy[Core]==1)
                        Tasks[i].Target=Tasks[i].CallFunc(Tasks[i].Arg);
                    SendIPI(APAPICIDs[Core], 0xEE);
                }else {
                    Tasks[i].Target=Tasks[i].CallFunc(Tasks[i].Arg);
                }
                
                if (Tasks[i].IntervalNs > 0){
                    Tasks[i].NextWaitNs = Now + Tasks[i].IntervalNs;
                }
                else{
                    Tasks[i].Active = false;
                    if (Tasks[i].DelFunc) Tasks[i].DelFunc(Tasks[i].Arg);
                }
            }
        }
    }
    __asm__("mfence");
    InTask=0;
}
