/*
UNIX Daemon Server Programming Sample Program
Levent Karakas <levent at mektup dot at> May 2001

To compile:cc -o exampled examped.c
To run:./exampled
To test daemon:ps -ef|grep exampled (or ps -aux on BSD systems)
To test log:tail -f /tmp/exampled.log
To test signal:kill -HUP `cat /tmp/exampled.lock`
To terminate:kill `cat /tmp/exampled.lock`
*/

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define RUNNING_DIR "/opt/apm_launcher"
#define LOCK_FILE   "exampled.lock"
#define LOG_FILE    "exampled.log"

void log_message(char* filename, char* message)
{
  FILE *logfile = NULL;
  logfile = fopen(filename, "a");
  if(!logfile) return;
  fprintf(logfile, "%s\n", message);
  fclose(logfile);
}

void signal_handler(int sig)
{
  switch(sig) {
  case SIGHUP:
    log_message(LOG_FILE, "hangup signal catched");
    break;
  case SIGTERM:
    //case SIGKILL:
    log_message(LOG_FILE, "terminate signal catched");
    //unlink(LOG_FILE);
    exit(0);
    break;
  }
}

void daemonize()
{
  int i, lfp;
  char str[10];
  if(getppid() == 1) return; /* already a daemon */
  i = fork();
  if (i<0) exit(1); /* fork error */
  if (i>0) exit(0); /* parent exits */
  /* child (daemon) continues */
  setsid(); /* obtain a new process group */
  for (i = getdtablesize(); i >= 0; --i) close(i); /* close all descriptors */
  i=open("/dev/null", O_RDWR); dup(i); dup(i); /* handle standart I/O */
  umask(027); /* set newly created file permissions */
  chdir(RUNNING_DIR); /* change running directory */
  lfp = open(LOCK_FILE,O_RDWR|O_CREAT,0640);
  if (lfp < 0) exit(1); /* can not open */
  if (lockf(lfp, F_TLOCK, 0) < 0) exit(0); /* can not lock */
  /* first instance continues */
  sprintf(str, "%d\n", getpid());
  write(lfp, str, strlen(str)); /* record pid to lockfile */
  signal(SIGCHLD, SIG_IGN); /* ignore child */
  signal(SIGTSTP, SIG_IGN); /* ignore tty signals */
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGHUP,  signal_handler); /* catch hangup signal */
  signal(SIGTERM, signal_handler); /* catch term signal */
  //signal(SIGKILL, signal_handler); /* catch kill signal */
}

//Respberry Pi GPIO memory 
//#define BCM2708_PERI_BASE   0x20000000
#define BCM2708_PERI_BASE   0x3F000000
#define GPIO_BASE           (BCM2708_PERI_BASE + 0x200000) 
#define PAGE_SIZE           (4*1024)
#define BLOCK_SIZE          (4*1024)

// GPIO setup. Always use INP_GPIO(x) before OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define GPIO_MODE_IN(g)     *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3)) 
#define GPIO_MODE_OUT(g)    *(gpio+((g)/10)) |=  (1<<(((g)%10)*3)) 
#define GPIO_MODE_ALT(g,a)  *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))
#define GPIO_SET_HIGH       *(gpio+7)  // sets   bits which are 1 
#define GPIO_SET_LOW        *(gpio+10) // clears bits which are 1 
#define GPIO_GET(g)         (*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH 

#define RPI_GPIO_22   22   // Pin 15 
#define RPI_GPIO_24   24   // Pin 18 

volatile uint32_t *gpio;

void get_gpio_address()
{
  int mem_fd;

  // open /dev/mem
  if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
    //log_message(LOG_FILE, "Can't open /dev/mem");
    printf("Can't open /dev/mem\n");
    exit(0);
  }

  // mmap GPIO
  void *gpio_map = mmap(
                   NULL,                 // Any adddress in our space will do
	           BLOCK_SIZE,           // Map length
                   PROT_READ|PROT_WRITE, // Enable reading & writting to mapped memory
                   MAP_SHARED,           // Shared with other processes
                   mem_fd,               // File to map
	           GPIO_BASE             // Offset to GPIO peripheral
		  );

  close(mem_fd); // No need to keep mem_fd open after mmap

  if (gpio_map == MAP_FAILED) {
    //log_message(LOG_FILE, "Can't map /dev/mem");
    printf("Can't map /dev/mem\n");
    exit(0);
  }

  gpio = (volatile uint32_t *)gpio_map; // Always use volatile pointer!
}

#define MSG_QUEEN_NAME "/request_send_queue"
static mqd_t q_read;
int main()
{
  static uint32_t old_pin_value  = 0;
  uint32_t new_pin_value  = 0;
  char status[64] = {0};
  char in_str[1024] = {0};
  char wpa_passphrase_cmd[256] = {0};
  struct mq_attr mqattr;
  char* v = NULL;
  char* settings[3] = {NULL}; //0: ssid, 1: encrypt, 2: key
  int msg_len = 0;
  int i = 0;

  mqattr.mq_maxmsg = 10;
  mqattr.mq_msgsize = 1024;
  q_read = mq_open(MSG_QUEEN_NAME, O_RDONLY | O_NONBLOCK | O_CREAT, S_IRUSR | S_IWUSR, &mqattr);

  //unlink(RUNNING_DIR);
  system("mkdir -p /opt/apm_launcher");
  get_gpio_address();
  GPIO_MODE_IN(RPI_GPIO_24);

  //daemonize();
  while(1) {
    memset(in_str, 0, 1024);
    settings[0] = settings[1] = settings[2] = NULL;
    new_pin_value = GPIO_GET(RPI_GPIO_24);
    if (new_pin_value != old_pin_value) {
      sprintf(status, "SoftAP pin value is %d\n", new_pin_value);
      //log_message(LOG_FILE, status);
      printf(status);

      if (new_pin_value) {
        //log_message(LOG_FILE, "SoftAP mode");
        printf("SoftAP mode\n");
        //system("sh station_stop.sh");
        //system("sh softap_start.sh");
      } else {
        //log_message(LOG_FILE, "Station mode");
        printf("Station mode\n");
        //system("sh softap_stop.sh");
        //system("sh station_start.sh");
      }
      old_pin_value = new_pin_value;
    } else {
       printf("No pin change\n");
    }

    if (new_pin_value == 0) { // Station mode
      msg_len = mq_receive(q_read, in_str, 1024, NULL);
      if (msg_len > 0) {
          v = strtok (in_str, ":");
          while (v != NULL) {
              settings[i] = v;
              v = strtok (NULL, ":");
              i++;
          }
          printf("ssid/psk/encrypt: %s | %s | %s\n", settings[0], settings[1], settings[2]);
          if (settings[0] != NULL && settings[2] != NULL && strlen(settings[0]) != 0 && strlen(settings[1]) != 0) {
              sprintf(wpa_passphrase_cmd, "wpa_passphrase %s %s >> /etc/wpa_supplicant.conf", settings[0], settings[1]);
              system(wpa_passphrase_cmd);
              printf(wpa_passphrase_cmd);
              system("ifdown wlan0");
              system("ifup wlan0");
          }
      }
    }
    sleep(2);
  }

  return 0;
}

/* EOF */
