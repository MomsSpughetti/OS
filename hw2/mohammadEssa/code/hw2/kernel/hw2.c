#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/sched.h>

asmlinkage long sys_hello(void) {
    printk("Hello, World!\n");
    return 0;
}

asmlinkage long sys_set_weight(int weight){
    if(weight < 0){
        return -EINVAL;
    }
    current->task_weight = weight;
    return 0;
}

asmlinkage long sys_get_weight(void){
    return current->task_weight;
}

asmlinkage long sys_get_ancestor_sum(void){
    long sum = 0;
    struct task_struct* currentTask = current; 
    while(currentTask->pid != 1){
        sum+=currentTask->task_weight;
        currentTask = currentTask->real_parent; 
    }
    return sum;
}

static void sys_get_heaviest_descendant_aux(const struct list_head* head,pid_t* heaviestPtr, int* maxWeightPtr){
    struct list_head* currentTask;
    const struct task_struct* task;
    list_for_each(currentTask,head) {
        task = list_entry(currentTask,struct task_struct, sibling);
        if(task->task_weight > *maxWeightPtr || (task->task_weight == *maxWeightPtr && *heaviestPtr > task->pid)){
            *heaviestPtr = task->pid;
            *maxWeightPtr = task->task_weight;
        }
        if(!list_empty(&task->children)){
            sys_get_heaviest_descendant_aux(&task->children,heaviestPtr,maxWeightPtr);
        }
    }
}

asmlinkage long sys_get_heaviest_descendant(void){
    pid_t pid = -1;
    int weight = -1;
    if(list_empty(&current->children)){
        return -ECHILD;
    }
    sys_get_heaviest_descendant_aux(&current->children,&pid,&weight);
    return pid;
}
