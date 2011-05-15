/*
 * mac_dump - dump memory from a target process
 *
 * Written by Dean Pucsek <dean@lightbulbone.com>
 * Copyright 2011, All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <mach/mach.h>
#include <mach/mach_traps.h>
#include <mach/mach_vm.h>
#include <mach/kern_return.h>



int fork_calculator() {
  int pid = fork();
  if(pid < 0) {
    fprintf(stderr, "fork error\n");
    return -1;
  } else if(pid == 0) { /* child */
    execl("/Applications/Calculator.app/Contents/MacOS/Calculator", 
          "/Applications/Calculator.app/Contents/MacOS/Calculator", 
          NULL);
    perror("execl");
  } else { /* parent */
    sleep(1); /* this is necessary */
    return pid;
  }

  return -1;
}

int mem_dump(mach_port_name_t tport) {
  kern_return_t ret = 0;
  pointer_t data;
  mach_msg_type_number_t data_size;

  long dump_base_addr = 0x10000190c;
  long dump_size = 0x1000;

  ret = mach_vm_read(tport, dump_base_addr, dump_size, &data, &data_size);
  if(ret != KERN_SUCCESS) {
    fprintf(stderr, "mem_dump: failed to read data at 0x%lx (0x%x)\n", dump_base_addr, ret);
    return -1;
  }

  int fd;
  fd = open("mem.dump", O_CREAT|O_RDWR, S_IRWXU);
  if(fd == -1) {
    perror("open");
    return -1;
  }

  if(write(fd, (const void *)data, (size_t)data_size) == -1) {
    perror("write");
    return -1;
  }

  return 0;
}

int main(int argc, char **argv) {
  kern_return_t ret = -1;
  mach_port_name_t tport = -1;
  int tpid = -1;

  tpid = fork_calculator();
  
  ret = task_for_pid(mach_task_self(), tpid, &tport);
  if(ret != KERN_SUCCESS) {
    fprintf(stderr, "main: failed to get task for pid %d (%d)\n", tpid, ret);
    return -1;
  }

  if(mem_dump(tport) < 0) {
    fprintf(stderr, "main: failed to dump memory\n");
    return -1;
  }

  kill(tpid, SIGTERM);

  return 0;
}
