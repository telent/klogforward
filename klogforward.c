#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/* 

syslog prefix
, 
64 bit message sequence number
,
monotonic timestamp in microseconds
,
flag field: - (single line), c (first line), + (subsequent line)
;
Human readable text string, followed by 
\n

continuation lines start with ' '
   KEY=value
   KEY=value

*/

struct entry {
  const char *unparsed;		/* so it can be freed */
  long prefix;			/* priority & facility */
  long sequence;
  long timestamp;
  char * message;
  size_t num_pairs;
  char* pairs[1];
};

void free_entry(struct entry *e) {
  free((void *)e->unparsed);
  free((void *)e);
}

/* the minimum maximum MUST be at least 480 octets in length.

   Any transport receiver MUST be able to accept messages of up to and
   including 480 octets in length.  All transport receiver
   implementations SHOULD be able to accept messages of up to and
   including 2048 octets in length.  Transport receivers MAY receive
   messages larger than 2048 octets in length. */

#define MESSAGE_MAX (2048)	/* does my buf look big in this? */

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

size_t count_newlines(char * buf)
{
  size_t number = 0;
  while((buf = strchr(buf, '\n')) != NULL) {
    buf++;
    number++;
  }
  return number;
}

void warn(const char *first, const char *second)
{
  write(2, first, strlen(first));
  write(2, second, strlen(second));
  write(2, "\n", 1);
}

long read_long(const char *buf, char **endptr, const char *field_name)
{
  long value = strtol(buf, endptr, 10);
  if(buf == *endptr) {
    warn("can't read ", field_name);
    return -1;
  }
  if(! ( (**endptr==';') || (**endptr==','))) {
    warn("junk after ", field_name);
    return -1;
  }
  return value;
}

struct entry * read_entry(int fd)
{
  char buf[MESSAGE_MAX];
  char * unparsed;
  char * end;
  struct entry *e;
  char * newline ;
  ssize_t len = read(fd, buf, MESSAGE_MAX);
  unparsed = strndup(buf, len);
  size_t num_pairs=count_newlines(unparsed) - 1;
  e = malloc(sizeof (struct entry) + num_pairs * sizeof(char *));
  e->unparsed = unparsed;
  e->num_pairs = num_pairs;
 
  e->prefix = read_long(unparsed, &end, "prefix");
  if(e->prefix < 0) goto fail;
  if(*end == ';') goto message;
  unparsed = end+1;

  e->sequence = read_long(unparsed, &end, "sequence");
  if(e->sequence < 0) goto fail;
  if(*end == ';') goto message;
  unparsed=end+1;
    
  e->timestamp = read_long(unparsed, &end, "timestamp");
  if(e->timestamp < 0) goto fail;

  end = strchr(end, ';');
 message:
  newline = strchr(end+1, '\n');
  *newline = '\0';
  e->message = end+1;
    
  return e;
 fail:
  return NULL;
}

char hostname[HOST_NAME_MAX];

int to_rfc_format(char *out_buf, size_t out_buf_len, struct entry *e)
{
  /* rfc 5424 is the 'standard' for syslog.  73.6% of syslog
   * sources "in the wild" don't conform to it, but the RFC was
   * written by the author of rsyslog, so there is at least one
   * halfway decent program available that will read it */
  char timestamp[40];
  struct tm tm;
  struct timespec ts;

  if(e->prefix < 8) {		/* kernel */
    clock_gettime(CLOCK_REALTIME, &ts);
    gmtime_r(&(ts.tv_sec), &tm);
    strftime(timestamp, 40, "%FT%T", &tm);
    return
      snprintf(out_buf, out_buf_len, "<%ld>1 %s.%6ldZ %s %s %s %s %s %s",
	       e->prefix,
	       timestamp,
	       (ts.tv_nsec / 1000),
	       hostname,
	       "kernel",		/* APP-NAME */
	       "-",		/* no PROCID */
	       "-",		/* no STRUCTURED-DATA */
	       "-",		/* no MSGID */
	       e->message);
  } else {
    strncpy(out_buf,  e->message, out_buf_len-1);
    out_buf[out_buf_len-1] = '\0';
  }
}

static inline int open_socket(const char *hostname, char* port)
{
   struct addrinfo hints;
   struct addrinfo *result, *rp;
   int sfd, s;
   
   /* Obtain address(es) matching host/port */
   
   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
   hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
   hints.ai_flags = 0;
   hints.ai_protocol = 0;          /* Any protocol */
     
   s = getaddrinfo(hostname, port, &hints, &result);
   if (s != 0) {
     warn("getaddrinfo: ", gai_strerror(s));
     exit(EXIT_FAILURE);
   }
   
   /* getaddrinfo() returns a list of address structures.
      Try each address until we successfully connect(2).
      If socket(2) (or connect(2)) fails, we (close the socket
      and) try the next address. */
   
   for (rp = result; rp != NULL; rp = rp->ai_next) {
     sfd = socket(rp->ai_family, rp->ai_socktype,
		  rp->ai_protocol);
     if (sfd == -1)
       continue;
     if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
       break;                  /* Success */
     }
     close(sfd);
   }
   
   if (rp == NULL) {               /* No address succeeded */
     warn("Could not connect","\n");
     exit(EXIT_FAILURE);
   }
   
   freeaddrinfo(result);           /* No longer needed */

   return sfd;
}


int main(int argc, char *argv[])
{
  /* $0  filename hostname port */
  int in_fd = open(argv[1], O_RDONLY);
  char * port = (argc >= 4) ? argv[3] : "514";
  int out_fd = open_socket(argv[2], port);
  char out_buf[MESSAGE_MAX];
  struct entry *e;
  gethostname(hostname, HOST_NAME_MAX);
  while(e = read_entry(in_fd)) {
    int bytes = to_rfc_format(out_buf, MESSAGE_MAX, e);
    int written = write(out_fd, out_buf, bytes);
    if(written < 0)
      warn("could not send: ", strerror(errno));
    free_entry(e);
  }    
}
  
