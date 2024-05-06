/**
 * Full Name: Bingkun Liu
 * Course ID: eecs3221 M
 * Description: Implement 2 manager algorithm for resource allocation
 *              Optimistic manager use FIFO algorithm
 *              Banker use banker's algorithm
 */
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

// Enum the type of activity
enum activity { initiate, request, compute, release, terminate };

// Define activity
typedef struct Act {
    // Activity type
    enum activity activity;
    // Task-number
    int para1;
    // Resource-type, number-of-cycles or unused
    int para2;
    // Initial-claim, number-requested, number-released, unused
    int para3;
} Act;

// Define task struct
typedef struct Task {
    // Task state, 0: aborted; 1: running; 2: terminated; 3: from blocked to running; negative: blocked
    int state;
    // Max initial of per task
    int *max;
    // Allocated resource for per task
    int *allocation;
    // Activity list
    Act *activities;
    // Current running activity id
    int p;
    // Executed time, not consider wait time
    int exetime;
    // Waited time
    int waittime;
} Task;

void manager(Task *tasks, int *resources, int tn, int rn, bool fifo);
void cycle(Task *tasks, int *resources, int tn, int rn, int *blocknum, bool fifo);
void exeAct(
    Task *tasks, int tn, int ti, int *resources, int rn, int *blocknum, int *releases, bool fifo);
bool isSafe(Task *tasks, int tn, int *resources, int rn);
bool isDead(Task *tasks, int tn, int *resources);
void delete_task(Task *tasks, int tn);

int main(int argc, char *argv[]) {
    // Check input
    if (argc != 2) {
        printf("Usage: %s inputfile\n", argv[0]);
        return 1;
    }
    // Open input file
    FILE *fin = fopen(argv[1], "r");
    if (fin == NULL) {
        printf("Error: can't open %s\n", argv[1]);
        return 1;
    }
    // Check number error
    int tn, rn;
    fscanf(fin, "%d %d", &tn, &rn);
    if (tn <= 0 || rn <= 0) {
        printf("the number of tasks and resource types should be positive integer\n");
        return 0;
    }
    int resources[rn], fifoResources[rn];
    for (int i = 0; i < rn; i++) {
        fscanf(fin, "%d", resources + i);
        if (resources[i] < 0) {
            printf("the number of units present of each resource type should be nonnegative "
                   "integer\n");
            return 0;
        }
    }
    // Get the max activities for per task stored in maxactn
    char c[20];
    int para1, para2, para3;
    int actn = 0;
    int maxactn = 0;
    while (fscanf(fin, "%s %d %d %d", c, &para1, &para2, &para3) != EOF) {
        actn++;
        if (strcmp(c, "terminate") == 0) {
            maxactn = actn > maxactn ? actn : maxactn;
            actn = 0;
        }
    }
    // Restart read input
    rewind(fin);
    // 2 tasks for 2 manager
    Task tasks[tn], fifoTasks[tn];
    // Fill every filed in task
    for (int i = 0; i < tn; i++) {
        fifoTasks[i].state = tasks[i].state = 1;
        tasks[i].max = (int *) malloc(rn * sizeof(int));
        fifoTasks[i].max = (int *) malloc(rn * sizeof(int));
        memset(tasks[i].max, 0, rn * sizeof(int));
        memset(fifoTasks[i].max, 0, rn * sizeof(int));
        tasks[i].allocation = (int *) malloc(rn * sizeof(int));
        fifoTasks[i].allocation = (int *) malloc(rn * sizeof(int));
        memset(tasks[i].allocation, 0, rn * sizeof(int));
        memset(fifoTasks[i].allocation, 0, rn * sizeof(int));
        tasks[i].activities = (Act *) malloc(maxactn * sizeof(Act));
        fifoTasks[i].activities = (Act *) malloc(maxactn * sizeof(Act));
        memset(tasks[i].activities, 0, maxactn * sizeof(Act));
        memset(fifoTasks[i].activities, 0, maxactn * sizeof(Act));
        fifoTasks[i].p = tasks[i].p = 0;
        fifoTasks[i].exetime = tasks[i].exetime = 0;
        fifoTasks[i].waittime = tasks[i].waittime = 0;
    }
    // Fill every activity
    fscanf(fin, "%d %d", &tn, &rn);
    for (int i = 0; i < rn; i++) {
        fscanf(fin, "%d", fifoResources + i);
    }
    int ti = 0;
    int ai = 0;
    while (fscanf(fin, "%s %d %d %d", c, &para1, &para2, &para3) != EOF) {
        ti = para1 - 1;
        fifoTasks[ti].activities[ai].para1 = tasks[ti].activities[ai].para1 = para1;
        fifoTasks[ti].activities[ai].para2 = tasks[ti].activities[ai].para2 = para2;
        fifoTasks[ti].activities[ai].para3 = tasks[ti].activities[ai].para3 = para3;

        if (strcmp(c, "initiate") == 0) {
            fifoTasks[ti].activities[ai].activity = tasks[ti].activities[ai].activity = initiate;
        } else if (strcmp(c, "request") == 0) {
            fifoTasks[ti].activities[ai].activity = tasks[ti].activities[ai].activity = request;
        } else if (strcmp(c, "compute") == 0) {
            fifoTasks[ti].activities[ai].activity = tasks[ti].activities[ai].activity = compute;
        } else if (strcmp(c, "release") == 0) {
            fifoTasks[ti].activities[ai].activity = tasks[ti].activities[ai].activity = release;
        } else if (strcmp(c, "terminate") == 0) {
            fifoTasks[ti].activities[ai].activity = tasks[ti].activities[ai].activity = terminate;
            ai = -1;
        }
        ai++;
    }
    // Simulate 2 manager
    manager(fifoTasks, fifoResources, tn, rn, true);
    manager(tasks, resources, tn, rn, false);
    // Output
    printf("\t\t\tFIFO\t\t\tBANKER's\n");
    int fifoTotalExe = 0, fifoTotalWait = 0, totalExe = 0, totalWait = 0;
    for (int i = 0; i < tn; i++) {
        printf("Task %d:\t", i + 1);
        if (fifoTasks[i].state == 0) {
            printf("\t\taborted\t\t");
        } else {
            int temp1 = fifoTasks[i].exetime + fifoTasks[i].waittime;
            int temp2 = fifoTasks[i].waittime;
            fifoTotalExe += temp1;
            fifoTotalWait += temp2;
            printf("%d\t%d\t%.0f%%\t\t", temp1, temp2, temp2 * 100.0 / temp1);
        }

        printf("Task %d:\t", i + 1);
        if (tasks[i].state == 0) {
            printf("aborted\n");
        } else {
            int temp1 = tasks[i].exetime + tasks[i].waittime;
            int temp2 = tasks[i].waittime;
            totalExe += temp1;
            totalWait += temp2;
            printf("%d\t%d\t%.0f%%\n", temp1, temp2, temp2 * 100.0 / temp1);
        }
    }
    printf("total\t%d\t%d\t%.0f%%\t\t", fifoTotalExe, fifoTotalWait,
        fifoTotalWait * 100.0 / fifoTotalExe);
    printf("total\t%d\t%d\t%.0f%%\n", totalExe, totalWait, totalWait * 100.0 / totalExe);
    // Close file
    fclose(fin);
    // Free memory
    delete_task(tasks, tn);
    delete_task(fifoTasks, tn);
    return 0;
}

/**
 * @brief Simulate algorithm for allocating resource
 * 
 * @param tasks Whole task array
 * @param resources Whole resource array
 * @param tn Number of tasks
 * @param rn Number of resources
 * @param fifo fifo or banker algorithm
 */
void manager(Task *tasks, int *resources, int tn, int rn, bool fifo) {
    bool allFinish = false;
    // Blocked task number
    int blocknum = 0;
    while (allFinish == false) {
        allFinish = true;
        // Simulate a cycle
        cycle(tasks, resources, tn, rn, &blocknum, fifo);
        for (int i = 0; i < tn; i++) {
            if (tasks[i].state == 1 || blocknum > 0) {
                // Still unfinished
                allFinish = false;
                break;
            }
        }
    }
}

/**
 * @brief Run all tasks in one cycle
 * 
 * @param tasks Task array
 * @param resources Resource array
 * @param tn Number of tasks
 * @param rn Number of resources
 * @param blocknum Current blocked task
 * @param fifo fifo or banker algorithm
 */
void cycle(Task *tasks, int *resources, int tn, int rn, int *blocknum, bool fifo) {
    // Store released resource, added when this cycle finished
    int releases[rn];
    for (int i = 0; i < rn; i++) {
        releases[i] = 0;
    }

    // Handle blocked tasks
    // state = -i means this task is the i-th blocked tasks, implementing queue
    for (int i = 1; i <= *blocknum; i++) {
        // Handle blocked task in order
        for (int j = 0; j < tn; j++) {
            // The i-th blocked task
            if (tasks[j].state == -i) {
                exeAct(tasks, tn, j, resources, rn, blocknum, releases, fifo);
                if (tasks[j].state == 1) {
                    // If become running state, already finish this cycle
                    tasks[j].state = 3;
                    // Simulation for dequeue, update other state
                    for (int k = 0; k < tn; k++) {
                        if (tasks[k].state < 0 && tasks[k].state < -i) {
                            tasks[k].state += 1;
                        }
                    }
                    *blocknum -= 1;
                    // Continuing handle the top of queue
                    i--;
                }
                break;
            }
        }
    }

    // Handle other unblocked tasks
    for (int i = 0; i < tn; i++) {
        // Skip blocked, aborted, terminatd tasks
        if (tasks[i].state <= 0 || tasks[i].state == 2) {
            continue;
        } else if (tasks[i].state == 3) {
            // Update to running state
            tasks[i].state = 1;
            continue;
        } else {
            // Execute this activity
            exeAct(tasks, tn, i, resources, rn, blocknum, releases, fifo);
        }
    }

    // Add all released resource
    for (int i = 0; i < rn; i++) {
        resources[i] += releases[i];
    }

    // FIFO Check Deadblock
    if (fifo) {
        while (isDead(tasks, tn, resources)) {
            // Kill deadlocked tasks, from the smallest number
            for (int i = 0; i < tn; i++) {
                if (tasks[i].state < 0) {
                    int temp = tasks[i].state;
                    tasks[i].state = 0;
                    // Free the holding resources, add up
                    for (int j = 0; j < rn; j++) {
                        resources[j] += tasks[i].allocation[j];
                    }
                    // Update state, dequeue
                    for (int k = 0; k < tn; k++) {
                        if (tasks[k].state < 0 && tasks[k].state < temp) {
                            tasks[k].state += 1;
                        }
                    }
                    *blocknum -= 1;
                    break;
                }
            }
        }
    }
}

/**
 * @brief Execute a activity of a task
 * 
 * @param tasks Task array
 * @param tn Number of tasks
 * @param ti The current task
 * @param resources Resource array
 * @param rn Number of resource
 * @param blocknum Number of current blocked task
 * @param releases Release resource
 * @param fifo FIFO or Banker
 */
void exeAct(
    Task *tasks, int tn, int ti, int *resources, int rn, int *blocknum, int *releases, bool fifo) {
    // Current activity
    int *p = &(tasks[ti].p);
    // Current task
    Task *t = tasks + ti;
    Act *a = &(t->activities[*p]);
    // a->para2 will be 1 more than index
    int rtype = a->para2 - 1;
    // Execute different activity
    switch (a->activity) {
    case initiate:
        // Valid initiation
        // FIFO will not consider this
        if (a->para3 <= resources[rtype] || fifo) {
            t->max[rtype] = a->para3;
            t->exetime += 1;
            *p += 1;
        } else {
            // Invalid initiation, abort it
            t->state = 0;
        }
        break;

    case request:
        if (t->allocation[rtype] + a->para3 > t->max[rtype] && !fifo) {
            // Execeed the max resource
            t->state = 0;
            // Free all resource
            for (int i = 0; i < rn; i++) {
                releases[i] += t->allocation[i];
            }
        } else if (a->para3 > resources[rtype]) {
            // Block
            if (t->state > 0) {
                // Unblocked to blocked
                *blocknum += 1;
                t->state = -(*blocknum);
            }
            t->waittime += 1;
        } else {
            // Can satisfy the allocation
            // Pre-allocation
            t->allocation[rtype] += a->para3;
            resources[rtype] -= a->para3;
            if (isSafe(tasks, tn, resources, rn) || fifo) {
                // Safety check, fifo will not consider
                t->state = 1;
                t->exetime += 1;
                *p += 1;
            } else {
                // Unsafe
                // Free allocation
                t->allocation[rtype] -= a->para3;
                resources[rtype] += a->para3;
                // Block
                if (t->state > 0) {
                    *blocknum += 1;
                    t->state = -(*blocknum);
                }
                t->waittime += 1;
            }
        }
        break;

    case compute:
        if (a->para2 > a->para3) {
            // Use the 3rd unused para to store already computed cycles
            t->exetime += 1;
            a->para3 += 1;
        } else {
            *p += 1;
        }
        break;

    case release:
        // Release resource
        releases[rtype] += t->allocation[rtype];
        t->allocation[rtype] = 0;
        t->exetime += 1;
        *p += 1;
        break;

    case terminate:
        // Terminate task
        t->state = 2;
        break;
    }
}

/*
 * This function only determines whether the current resource allocation is safe
 * Do not change the parameters of the task and the number of remaining resources
 */

/**
 * @brief Check the allocation is safe
 * 
 * @param tasks Task array
 * @param tn Number of tasks
 * @param resources Resource array
 * @param rn Number of resource
 * @return true safe
 * @return false unsafe
 */
bool isSafe(Task *tasks, int tn, int *resources, int rn) {
    // Current resource
    int r[rn];
    for (int i = 0; i < rn; i++) {
        r[i] = resources[i];
    }
    // Need resource
    int need[tn][rn];
    // Allocated resource
    int allocation[tn][rn];
    // Flag indicate whether tn-th task finished
    int finish[tn];
    for (int i = 0; i < tn; i++) {
        if (tasks[i].state == 0 || tasks[i].state == 2) {
            finish[i] = 1;
        } else {
            finish[i] = 0;
        }
        for (int j = 0; j < rn; j++) {
            allocation[i][j] = tasks[i].allocation[j];
            need[i][j] = tasks[i].max[j] - allocation[i][j];
        }
    }

    bool canFinish = true;
    bool deadLock = false;
    while (true) {
        deadLock = true;
        for (int i = 0; i < tn; i++) {
            if (finish[i] == 1) {
                // Already finished
                continue;
            }
            canFinish = true;
            for (int j = 0; j < rn; j++) {
                if (need[i][j] > r[j]) {
                    // Resource less than need
                    canFinish = false;
                    break;
                }
            }
            // Can finish this task
            if (canFinish) {
                for (int j = 0; j < rn; j++) {
                    // Free this task resource
                    r[j] += allocation[i][j];
                }
                finish[i] = 1;
                // This task will not case dead lock, still need loop
                deadLock = false;
            }
        }

        if (deadLock == true) {
            // Dead lock or all tasks finished
            for (int i = 0; i < tn; i++) {
                if (finish[i] == 0) {
                    // Dead lock
                    return false;
                }
            }
            // All finished
            return true;
        }
    }
}

/**
 * @brief Check whether occurs dead lock in fifo manager
 * 
 * @param tasks Task array
 * @param tn Number of tasks
 * @param resources Resource array
 * @return true will dead lock
 * @return false not dead lock
 */
bool isDead(Task *tasks, int tn, int *resources) {
    bool terminate = false;
    for (int i = 0; i < tn; i++) {
        if (tasks[i].state == 1) {
            // Still running task
            return false;
        } else if (tasks[i].state < 0) {
            Act *a = tasks[i].activities + tasks[i].p;
            if (resources[a->para2 - 1] >= a->para3) {
                // This task can be satisfy
                return false;
            }
            terminate = true;
        }
    }
    // All no-terminate task cannot be satisfy
    return terminate;
}

/**
 * @brief Delete allocated memory using malloc
 * 
 * @param tasks Task array
 * @param tn Number of tasks
 */
void delete_task(Task *tasks, int tn) {
    for (int i = 0; i < tn; i++) {
        free(tasks[i].activities);
        free(tasks[i].allocation);
        free(tasks[i].max);
        tasks[i].activities = NULL;
        tasks[i].allocation = NULL;
        tasks[i].max = NULL;
    }
}
