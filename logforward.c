#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <sys/sysinfo.h>

#include <time.h>

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

#define MESSAGE_MAX (1024)	/* does my buf look big in this? */

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

char *hostname="wombat";

char *to_rfc_format(char *out_buf, size_t out_buf_len, struct entry *e)
{
  /* rfc 3164 is ill-specified, but rfc 5424 is unused.  We go with the older
   * version, except that we use iso timestamp */
  char timestamp[40];
  struct tm tm;
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  gmtime_r(&(ts.tv_sec), &tm);
  strftime(timestamp, 40, "%FT%T", &tm);
  snprintf(out_buf, out_buf_len, "<%ld> %s.%ldZ %s %s %s",
	   e->prefix,
	   timestamp,
	   ts.tv_nsec,
	   hostname,
	   "kernel",
	   e->message);
  return out_buf;
}

int main(int argc, char *argv[])
{
  int in_fd = open(argv[1], O_RDONLY);
  char out_buf[1024];
  struct entry *e;
  while(e = read_entry(in_fd)) {
    puts(to_rfc_format(out_buf, 1024, e));
  }    
}
  
