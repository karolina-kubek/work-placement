/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*---------------------------------------------------------------------------*/
/**
 * \file
 *         Example on how to use CFS/Coffee.
 * \author
 *         Nicolas Tsiftes <nvt@sics.se>
 * \edits 
 *         Karolina Kubek
 *
 * Coffee determines the file length by finding the last non-zero byte
 * of the file. This I/O semantic requires that each record should end
 * with a non-zero, if we are writing multiple records and closing the
 * file descriptor in between.
 *
 * In this example, in which the file_test function can be called
 * multiple times, we ensure that the sequence counter starts at 1.
 *
**/
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"
/*---------------------------------------------------------------------------*/
PROCESS(example_coffee_process, "Coffee example");
AUTOSTART_PROCESSES(&example_coffee_process);
/*---------------------------------------------------------------------------*/
#define FILENAME "test"
/*---------------------------------------------------------------------------*/
  /* Formatting is needed if the storage device is in an unknown state;
   e.g., when using Coffee on the storage device for the first time. */
#ifndef NEED_FORMATTING
#define NEED_FORMATTING 0
#endif

/*---------------------------------------------------------------------------*/
static int
file_test(const char *filename, uint8_t *msg1, uint8_t *msg2 )
{
  static uint8_t sequence; /* Counter for remeining space */
  int fd;                  /* File discriptor */
  int r;                   /* Record written into the open file */
  
  /*---------------------------------------------------------------------------*/
    /* Struct that you can change according to your own needs */
  
  struct record {
    uint8_t message1;
    uint8_t message2;
    uint8_t sequence;
  } record;
     
 /*---------------------------------------------------------------------------*/
   /* Returning -1 when the memory is full and no more sequence numbers are avaliable */ 
  
  if(sequence == 255) {
    return -1;
  }
  sequence++;
  
/*---------------------------------------------------------------------------*/
  /* Assigning data received as parameters from the function */
  /* This can be changed to assigning user input, if you are to embed this code in your application */
  
  record.message1 = *(uint8_t *)msg1;
  record.message2 = *(uint8_t *)msg2;
  record.sequence = sequence;
  
/*---------------------------------------------------------------------------*/
  /* Obtain a file descriptor for the file, capable of handling both
     reads and writes. */
     
  fd = cfs_open(FILENAME, CFS_WRITE | CFS_APPEND | CFS_READ);
  if(fd < 0) {
    printf("failed to open %s\n", FILENAME);
    return 0;
  }
  
/*---------------------------------------------------------------------------*/
  /* Write the struct into the open file */

  r = cfs_write(fd, &record, sizeof(record));
  if(r != sizeof(record)) {
    printf("failed to write %d bytes to %s\n",
           (int)sizeof(record), FILENAME);
    cfs_close(fd);
    return 0;
  }

  printf("Wrote message \"%u\", \"%u\", sequence %u\n",
         record.message1, record.message2, record.sequence);
         
/*---------------------------------------------------------------------------*/
  /* Moving the pointer to the beginning */
  /* To read back the message, we need to move the file pointer to the
     beginning of the file. */
     
  if(cfs_seek(fd, 0, CFS_SEEK_SET) != 0) {
    printf("seek failed\n");
    cfs_close(fd);
    return 0;
  }
  
/*---------------------------------------------------------------------------*/
  /* Read - Output */
  /* Read all written records and print their contents. */
  
  for(;;) {
    r = cfs_read(fd, &record, sizeof(record));
    if(r == 0) {
      break;
    } else if(r < sizeof(record)) {
      printf("failed to read %d bytes from %s, got %d\n",
             (int)sizeof(record), FILENAME, r);
      cfs_close(fd);
      return 0;
    }

    printf("Read message \"%u\", \"%u\", sequence %u\n",
           record.message1, record.message2, record.sequence);
  }
  
/*---------------------------------------------------------------------------*/
  /* Release the internal resources */
  /* Referance -> cfs_open */
  
  cfs_close(fd);

  return 1;
  
}
/*---------------------------------------------------------------------------*/
static int
dir_test(void)
{
  struct cfs_dir dir;
  struct cfs_dirent dirent;

  /* Coffee provides a root directory only. */
  if(cfs_opendir(&dir, "/") != 0) {
    printf("failed to open the root directory\n");
    return 0;
  }

  /* List all files and their file sizes. */
  printf("Available files\n");
  while(cfs_readdir(&dir, &dirent) == 0) {
    printf("%s (%lu bytes)\n", dirent.name, (unsigned long)dirent.size);
  }

  cfs_closedir(&dir);

  return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(example_coffee_process, ev, data)
{
  PROCESS_BEGIN();
  uint8_t test_data1;
  uint8_t test_data2;
#if NEED_FORMATTING
  cfs_coffee_format();
#endif
  /*---------------------------------------------------------------------------*/
    /* Ensure that we will be working with a new file. */
  cfs_remove(FILENAME);
  
  /*---------------------------------------------------------------------------*/
    /* Running the file_test function (two writes)*/
  test_data1 = 76;
  test_data2 = 77;  
  if(file_test(FILENAME, &test_data1, &test_data2) == 0) {
    printf("file test 1 failed\n");
  }
  test_data1 = 55;
  test_data2 = 56;
  if(file_test(FILENAME, &test_data1, &test_data2) == 0) {
    printf("file test 2 failed\n");
  }
  
  /*---------------------------------------------------------------------------*/
    /* Checking if the directory can be opened and how many files were written to it (+size)*/
  if(dir_test() == 0) {
    printf("dir test failed\n");
  }

  PROCESS_END();
}
