#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

struct cpu_times
{
    double idle;
    double total;
};

struct cpu_stat
{
    struct cpu_times cores[256];
    double c_delta[256];
    double i_delta[256];
    double c_perc[256];
    int n_core;
};

int read_stat(struct cpu_stat *out, struct cpu_stat *start)
{
    char data[65535] = {0};
    char *stat_lines[256] = {0};
    char *tokens[10];
    int num_lines = 1,n_token;
    int n_core = 0;
    FILE *fp = fopen("/proc/stat", "r");
    if(fp == NULL)
        return -1;
    size_t length = fread(data, sizeof(char), sizeof(data), fp);
    if(ferror(fp) != 0 || length == 0)
        return -1;
    fclose(fp);
    if(out)
        memset(out, 0, sizeof(*out));
    stat_lines[0] = strtok(data, "\n");
    while((stat_lines[num_lines] = strtok(NULL, "\n")) != NULL)
        num_lines++;
    for(int i = 1; i < num_lines; i++) {
        if(strstr(stat_lines[i], "cpu") == NULL)
            break;
         strtok(stat_lines[i], " ");
         n_token = 0;
         while ((tokens[n_token] = strtok(NULL, " ")) != NULL)
             n_token++;
         if (!out) {
             for (int t = 0; t < n_token; t++)
                 start->cores[n_core].total += (double) atoi(tokens[t]);
             start->cores[n_core].idle = (double) atoi(tokens[3]);
             start->n_core++;
         } else {
             for (int t = 0; t < n_token; t++)
                 out->cores[n_core].total += (double) atoi(tokens[t]);
             out->cores[n_core].idle = (double) atoi(tokens[3]);
         }
         if (out) {
             out->c_delta[out->n_core] = out->cores[out->n_core].total - start->cores[out->n_core].total;
             out->i_delta[out->n_core] = out->cores[out->n_core].idle - start->cores[out->n_core].idle;
             double used = out->c_delta[out->n_core] - out->i_delta[out->n_core];
             out->c_perc[out->n_core] = 100 * (used / out->c_delta[out->n_core]);
             start->cores[out->n_core].total = out->cores[out->n_core].total;
             start->cores[out->n_core].idle = out->cores[out->n_core].idle;
             out->n_core++;
         }
         n_core++;
    }
    return 0;
}

void *cpu_monitor(void *arg __attribute__((__unused__)))
{
    bool first = true;
    struct cpu_stat previous = {0}, current = {0};
    int num_cpu = 0;
    while(1) {
        if(first)
        {
            read_stat(NULL, &previous);
            num_cpu = previous.n_core;
            first = false;
            continue;
        }
        sleep(5);
        memset(&current, 0, sizeof(current));
        read_stat(&current, &previous);
        for(int i = 0; i < num_cpu; i++) {
            printf("CPU %u utilization: %.2f%%\n",i, current.c_perc[i]);
        }
    }
}

int main() {
    pthread_t mon_thread;
    pthread_create(&mon_thread, NULL, cpu_monitor, NULL);
    pthread_join(mon_thread, NULL);
	return 0;
}
