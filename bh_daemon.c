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

//Respberry Pi GPIO memory
//#define BCM2708_PERI_BASE   0x20000000
#define BCM2708_PERI_BASE   0x3F000000
#define GPIO_BASE           (BCM2708_PERI_BASE + 0x200000) //FIXME: Have to distingush RPI v1 and V2
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

#define L2W_MSG_QUEUE "/l2w_msg_queue"
#define W2L_MSG_QUEUE "/w2l_msg_queue"

volatile uint32_t *gpio;
static mqd_t l2w_qhd;
static mqd_t w2l_qhd;

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
    mq_close(l2w_qhd);
    mq_close(w2l_qhd);
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
  lfp = open(LOCK_FILE, O_RDWR|O_CREAT, 0640);
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

void check_set_wifi_setting(int pin_value)
{
  int msg_len = 0;
  int i = 0;
  char* v = NULL;
  char* params[3] = {NULL}; //0: ssid, 1: encrypt, 2: key
  char wpa_passphrase_cmd[256] = {0};
  char req[1024] = {0};

  msg_len = mq_receive(l2w_qhd, req, 1024, NULL);
  if (msg_len > 0) {
      if (0 == pin_value) {
          mq_send(w2l_qhd, "station", strlen("station"), 1);
          printf("Set wifi setting\n");
          v = strtok (req, ":");
          while (v != NULL) {
              params[i] = v;
              v = strtok (NULL, ":");
              i++;
          }
          printf("ssid/encrypt/psk: %s | %s | %s\n", params[0], params[1], params[2]);
          if (params[0] != NULL && params[2] != NULL && strlen(params[0]) != 0 && strlen(params[2]) != 0) {
              sprintf(wpa_passphrase_cmd, "wpa_passphrase %s %s >> /etc/wpa_supplicant.conf", params[0], params[2]);
              system(wpa_passphrase_cmd);
              printf(wpa_passphrase_cmd);
              system("ifdown wlan0");
              system("ifup wlan0");
          }
      }
      else {
          mq_send(w2l_qhd, "softap", strlen("softap"), 1);
      }
  }
}

void launch_station_mode()
{
  printf("Station mode\n");
  system("sh script/station_start.sh");
}

void launch_softap_mode()
{
  printf("SoftAP mode\n");
  system("sh script/softap_start.sh");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main()
{
  static uint32_t old_pin_value  = 0;
  uint32_t new_pin_value  = 0;
  char status[64] = {0};
  struct mq_attr mqattr;

  mqattr.mq_maxmsg = 10;
  mqattr.mq_msgsize = 1024;
  l2w_qhd = mq_open(L2W_MSG_QUEUE, O_RDONLY | O_NONBLOCK | O_CREAT, S_IRUSR | S_IWUSR, &mqattr);
  w2l_qhd = mq_open(W2L_MSG_QUEUE, O_WRONLY | O_NONBLOCK | O_CREAT, S_IRUSR | S_IWUSR, &mqattr);

  //unlink(RUNNING_DIR);
  system("mkdir -p /opt/apm_launcher");
  get_gpio_address();
  GPIO_MODE_IN(RPI_GPIO_24);

  //daemonize();
  signal(SIGTERM, signal_handler); /* catch term signal */
  while(1) {
    new_pin_value = GPIO_GET(RPI_GPIO_24);
    printf("wifi config pin value: %d\n", new_pin_value);
    check_set_wifi_setting(new_pin_value); // check wifi setting request

    if (new_pin_value != old_pin_value) {
      //log_message(LOG_FILE, status);

      if (new_pin_value) {
        //log_message(LOG_FILE, "SoftAP mode");
        launch_softap_mode();
      } else {
        //log_message(LOG_FILE, "Station mode");
        launch_station_mode();
      }

      old_pin_value = new_pin_value;
    } else {
       printf("No pin change\n");
    }

    sleep(2);
  }

  return 0;
}

/* EOF */
