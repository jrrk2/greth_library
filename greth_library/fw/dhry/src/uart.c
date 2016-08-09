/*****************************************************************************
 * @file
 * @author   Sergey Khabarov
 * @brief    Firmware example. 
 ****************************************************************************/

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include "axi_maps.h"

extern char _end;
int errno;
static char *brk = &_end;

extern int *__errno (void)
{
  return &errno;
}

void print_uart(const char *buf, int sz) {
    uart_map *uart = (uart_map *)ADDR_NASTI_SLAVE_UART1;
    for (int i = 0; i < sz; i++) {
        while (uart->status & UART_STATUS_TX_FULL) {}
        uart->data = buf[i];
    }
}

void print_string(const char *ss) {
  int ss_len = strlen(ss);
  print_uart(ss, ss_len);
}

void _exit(int code)
{
  print_string("\n_exit called. Goodbye\n");
  for(;;);
}

int close( int __fildes )
{
  char buf[99];
  sprintf(buf, "close(%d) called\n", __fildes);
  print_string(buf);
  return 0;
}

int fstat( int __fd, struct stat *__sbuf )
{
  char buf[99];
  sprintf(buf, "fstat(%d,%p) called\n", __fd, __sbuf);
  print_string(buf);
  memset(__sbuf, 0, sizeof (struct stat));
  switch(__fd)
    {
    case 1:
      __sbuf->st_dev = 14;
      __sbuf->st_ino = 21;
      __sbuf->st_nlink = 1;
      __sbuf->st_mode = 400;
      __sbuf->st_nlink = 1;
      __sbuf->st_uid = 1000;
      __sbuf->st_gid = 5;
      __sbuf->st_rdev = 34834;
      __sbuf->st_size = 0;
      __sbuf->st_blksize = 512;
      __sbuf->st_blocks = 1;
      __sbuf->st_atime = 1464947208.79300666;
      __sbuf->st_mtime = 1464947192.79300666;
      __sbuf->st_ctime = 1464784282.79300666;
      return 0;      
      break;
    default:
      return -1;
      break;
    }
}

int isatty( int __fildes )
{
  print_string("isatty called\n");
  return 1;
}

off_t lseek (int __fildes, off_t __offset, int __whence )
{
  print_string("lseek called\n");
  return __offset;
}

int read(int __fd, void *__buf, size_t __nbyte)
{
  print_string("read called\n");
  *(char *)__buf = '\n';
  return 1;
}

int write(int __fd, const void *__buf, size_t __nbyte)
{
  print_uart(__buf, __nbyte);
  return __nbyte;
}

int ignore(int ignorant)
{
  return ignorant;
}

/**
 * @name sbrk
 * @brief Increase program data space.
 * @details Malloc and related functions depend on this.
 */

void *sbrk(ptrdiff_t __incr)
{
  char *old = brk;
  char buf[99];
  brk += __incr;
  sprintf(buf, "sbrk(%d) returned %p (new brk=%p)\n", __incr, old, brk);
  print_string(buf);
  return old;
}
