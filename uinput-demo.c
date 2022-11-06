#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/uinput.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define msleep(ms) usleep((ms)*1000)

#define PORT 1234

static void setup_abs(int fd, unsigned chan, int min, int max);  


// demo showing how to provide a joystick/gamepad instance from
// userspace.

int main(void)
{ 
    int fd[4];
    for (uint16_t no = 0; no < 4; no++) {
        fd[no] = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

        if (fd[no] < 0)
        {
            perror("open /dev/uinput");
            return 1;
        }

        ioctl(fd[no], UI_SET_EVBIT, EV_KEY); // enable button/key handling

        ioctl(fd[no], UI_SET_KEYBIT, BTN_A);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_B);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_X);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_Y);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_TL);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_TR);
#if 0
        ioctl(fd[no], UI_SET_KEYBIT, BTN_TL2);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_TR2);
#endif
        ioctl(fd[no], UI_SET_KEYBIT, BTN_START);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_SELECT);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_THUMBL);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_THUMBR);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_DPAD_UP);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_DPAD_DOWN);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_DPAD_LEFT);
        ioctl(fd[no], UI_SET_KEYBIT, BTN_DPAD_RIGHT);

#if 0  
        ioctl(fd[no], UI_SET_EVBIT, EV_ABS); // enable analog absolute position handling

        setup_abs(fd[no], ABS_X, -512, 512);
        setup_abs(fd[no], ABS_Y, -512, 512);
        setup_abs(fd[no], ABS_Z, -32767, 32767);
        setup_abs(fd[no], ABS_RX, 0, 100);
        setup_abs(fd[no], ABS_RY, 0, 100);
        setup_abs(fd[no], ABS_RZ, 0, 100);
#endif

        struct uinput_setup setup =
        {
         .name = "Userspace Joystick",
         .id =
         {
          .bustype = BUS_USB,
          .vendor = 0x289B, // doesn't get merged by MiSTer
          .product = 3,
          .version = 2,
         }
        };

        if (ioctl(fd[no], UI_DEV_SETUP, &setup))
        {
            perror("UI_DEV_SETUP");
            return 1;
        }

        if (ioctl(fd[no], UI_DEV_CREATE))
        {
            perror("UI_DEV_CREATE");
            return 1;
        }
    }



struct sockaddr_in server;

memset( &server, 0, sizeof (server));
// IPv4-Adresse
server.sin_family = AF_INET;
// Jede IP-Adresse ist gültig
server.sin_addr.s_addr = htonl( INADDR_ANY );

server.sin_port = htons(PORT);
int sock = socket( AF_INET, SOCK_STREAM, 0 );
if(bind( sock, (struct sockaddr*)&server, sizeof( server)) < 0) {
          perror("bind");
          return 1;
}
else {
    printf("bind\n");
}


if( listen( sock, 5 ) == -1 ) {
          perror("listen");
          return 1;
}
else {
    printf("listen :%i\n", PORT);
}


int client_socket;
struct sockaddr_in client;
socklen_t len;
for (;;) {
        len = sizeof( client );
        client_socket = accept( sock, (struct sockaddr*)&client, &len);
        if (client_socket < 0) {
           perror("accept");
           return 1;
        }
        else {
            printf("accept\n");
        }
    struct input_event_ex {
        uint16_t joyno;
        struct input_event ev;
    };
#define XUSER_MAX_COUNT                 4
#define EVENTS_MAX_COUNT (2+2+10)*XUSER_MAX_COUNT
    struct input_event_ex ev[EVENTS_MAX_COUNT];
    struct input_event evSyn;
    int recv_size, write_size;

    evSyn.type = EV_SYN;
    evSyn.code = SYN_REPORT;
    evSyn.value = 0;
    int keepRunning = 1;
    while (keepRunning) {
        if ((recv_size = recv(client_socket, ev, sizeof ev/*[0]*/, /*0*/MSG_WAITALL)) < 0) {
            perror("recv");
//            return 1;
            keepRunning = 0;
        }
        else {
            printf("recv [%i]  ", recv_size);
            int i = 0;
            while (ev[i].ev.type) {
                printf("%i: %x, %x, %x  ", ev[i].joyno, ev[i].ev.type, ev[i].ev.code, ev[i].ev.value);
                if ((write_size = write(fd[ev[i].joyno], &ev[i].ev, sizeof ev[i].ev)) < 0) {
                    perror("write");
                    //          return 1;
                }
                i++;
            }
            printf("\n");
            // todo only modified joys
            for (int i = 0; i < 4; i++) {
                if ((write_size = write(fd[i], &evSyn, sizeof evSyn)) < 0) {
                    perror("write");
                    //          return 1;
                }
            }
        }
    }
}


#if 0  
// you can write events one at a time, but to save overhead we'll
  // update all of them in a single write

  // x and y are triangle wave 90 degrees out of phase
  // z ramps slowly
  // A B X Y buttons toggle at four different frequencies

  int x = 0;
  int xdelta = 1;
  
  int y = -512;
  int ydelta = 1;
  
  unsigned count = 0;

  while(1)
    {
      struct input_event ev[8];
      memset(&ev, 0, sizeof ev);

      ev[0].type = EV_KEY;
      ev[0].code = BTN_A;
      ev[0].value = !(count & 0b1000000);

      ev[1].type = EV_KEY;
      ev[1].code = BTN_B;
      ev[1].value = !(count & 0b0100000);

      ev[2].type = EV_KEY;
      ev[2].code = BTN_X;
      ev[2].value = !(count & 0b0010000);

      ev[3].type = EV_KEY;
      ev[3].code = BTN_Y;
      ev[3].value = !(count & 0b0001000);

      ev[4].type = EV_ABS;
      ev[4].code = ABS_Y;
      ev[4].value = y;

      ev[5].type = EV_ABS;
      ev[5].code = ABS_X;
      ev[5].value = x;
    
      ev[6].type = EV_ABS;
      ev[6].code = ABS_Z;
      ev[6].value = (count>>3) & 0x7fff;

      // sync event tells input layer we're done with a "batch" of
      // updates
    
      ev[7].type = EV_SYN;
      ev[7].code = SYN_REPORT;
      ev[7].value = 0;

      if(write(fd, &ev, sizeof ev) < 0)
        {
          perror("write");
          return 1;
        }
    
      msleep(50);

      x += xdelta;

      if (x >= 512 || x <= -512)
        xdelta = -xdelta;

      y += ydelta;

      if (y >= 512 || y <= -512)
        ydelta = -ydelta;
    
      ++count;
    }
#endif
  for (uint16_t no = 0; no < 4; no++) {
      if (ioctl(fd[no], UI_DEV_DESTROY))
      {
          printf("UI_DEV_DESTROY");
          return 1;
      }

      close(fd[no]);
  }
  return 0;
}


// enable and configure an absolute "position" analog channel

static void setup_abs(int fd, unsigned chan, int min, int max)
{
  if (ioctl(fd, UI_SET_ABSBIT, chan))
    perror("UI_SET_ABSBIT");
  
  struct uinput_abs_setup s =
    {
     .code = chan,
     .absinfo = { .minimum = min,  .maximum = max },
    };

  if (ioctl(fd, UI_ABS_SETUP, &s))
    perror("UI_ABS_SETUP");
}

