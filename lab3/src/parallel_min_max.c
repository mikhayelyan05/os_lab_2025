#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
              printf("seed must be a positive number\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
              printf("array_size must be a positive number\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
              printf("pnum must be a positive number\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  // Create pipes for communication if not using files
  int *pipe_fds = NULL;
  if (!with_files) {
    pipe_fds = malloc(2 * pnum * sizeof(int));
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipe_fds + 2 * i) < 0) {
        printf("Failed to create pipe\n");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  // Calculate the size of each part
  int part_size = array_size / pnum;
  int remainder = array_size % pnum;

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      if (child_pid == 0) {
        // child process
        int start_index = i * part_size;
        int end_index = (i == pnum - 1) ? array_size : (i + 1) * part_size;
        
        // Adjust for remainder
        if (i < remainder) {
          start_index += i;
          end_index = start_index + part_size + 1;
        } else {
          start_index += remainder;
          end_index = start_index + part_size;
        }

        struct MinMax local_min_max = GetMinMax(array, start_index, end_index);

        if (with_files) {
          // Use files for communication
          char filename[256];
          sprintf(filename, "minmax_%d.txt", getpid());
          
          FILE *file = fopen(filename, "w");
          if (file != NULL) {
            fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
            fclose(file);
          }
        } else {
          // Use pipes for communication
          close(pipe_fds[2 * i]); // Close read end
          
          write(pipe_fds[2 * i + 1], &local_min_max.min, sizeof(int));
          write(pipe_fds[2 * i + 1], &local_min_max.max, sizeof(int));
          
          close(pipe_fds[2 * i + 1]); // Close write end
        }
        
        free(array);
        exit(0);
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // Parent process
  if (!with_files) {
    // Close write ends of all pipes in parent
    for (int i = 0; i < pnum; i++) {
      close(pipe_fds[2 * i + 1]);
    }
  }

  while (active_child_processes > 0) {
    wait(NULL);
    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      // Read from files
      char filename[256];
      sprintf(filename, "minmax_%d.txt", getpid() + i + 1); // PID of child process
      
      FILE *file = fopen(filename, "r");
      if (file != NULL) {
        fscanf(file, "%d %d", &min, &max);
        fclose(file);
        remove(filename); // Clean up
      }
    } else {
      // Read from pipes
      read(pipe_fds[2 * i], &min, sizeof(int));
      read(pipe_fds[2 * i], &max, sizeof(int));
      close(pipe_fds[2 * i]); // Close read end after reading
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  if (!with_files) {
    free(pipe_fds);
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}