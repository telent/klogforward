#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static int open_unix_socket(char * socket_name)
{
  int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
  if(fd == -1)
    exit(1);
  
  struct sockaddr_un saddr;
  memset(&saddr, 0, sizeof(struct sockaddr_un));
  saddr.sun_family = AF_UNIX;
  strncpy(saddr.sun_path, socket_name, strlen(socket_name));

  bind(fd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_un));
  return fd;
}

static int open_kmsg(char * node_name)
{
  return open(node_name, O_WRONLY);
}


int main(int argc, char * argv[])
{
  int src = open_unix_socket(argv[1]);
  int dst = open_kmsg(argv[2]);
  char buf[2048];
  int bytes;
  while(1){
    bytes = recvfrom(src, buf, 2048, 0, NULL, 0);
    write(dst, buf, bytes);
  }
}
