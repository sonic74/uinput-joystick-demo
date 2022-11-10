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


// demo showing how to provide a joystick/gamepad instance from
// userspace.

int main(int argc, char* argv[]) {
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

        ioctl(fd[no], UI_SET_KEYBIT, KEY_LEFTCTRL);
        ioctl(fd[no], UI_SET_KEYBIT, KEY_SPACE);
        ioctl(fd[no], UI_SET_KEYBIT, KEY_COMMA);
        ioctl(fd[no], UI_SET_KEYBIT, KEY_DOT);
        ioctl(fd[no], UI_SET_KEYBIT, KEY_ENTER);
        ioctl(fd[no], UI_SET_KEYBIT, KEY_ESC);
        ioctl(fd[no], UI_SET_KEYBIT, KEY_LEFTSHIFT);
        ioctl(fd[no], UI_SET_KEYBIT, KEY_UP);
        ioctl(fd[no], UI_SET_KEYBIT, KEY_DOWN);
        ioctl(fd[no], UI_SET_KEYBIT, KEY_LEFT);
        ioctl(fd[no], UI_SET_KEYBIT, KEY_RIGHT);

        int version;
        if (ioctl(fd[no], UI_GET_VERSION, &version))
        {
            perror("UI_GET_VERSION");
            return 1;
        }
        printf("UI_GET_VERSION:%i\n", version);
        if (version >= 5) {
            /* use UI_DEV_SETUP */
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

        } else {
            struct uinput_user_dev uud;
            memset(&uud, 0, sizeof(uud));
            snprintf(uud.name, UINPUT_MAX_NAME_SIZE, "uinput old interface %i", no);
            write(fd[no], &uud, sizeof(uud));
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
                if(/*ev[i].joyno==1 &&*/ argc>1) {
                    if(     ev[i].ev.code==BTN_A         ) ev[i].ev.code=KEY_LEFTCTRL;
                    else if(ev[i].ev.code==BTN_B         ) ev[i].ev.code=KEY_SPACE;
                    //else if(ev[i].ev.code==BTN_X         ) ev[i].ev.code=KEY_;
                    //else if(ev[i].ev.code==BTN_Y         ) ev[i].ev.code=KEY_;
                    else if(ev[i].ev.code==BTN_TL        ) ev[i].ev.code=KEY_COMMA;
                    else if(ev[i].ev.code==BTN_TR        ) ev[i].ev.code=KEY_DOT;
                    else if(ev[i].ev.code==BTN_START     ) ev[i].ev.code=KEY_ENTER;
                    else if(ev[i].ev.code==BTN_SELECT    ) ev[i].ev.code=KEY_ESC;
                    //else if(ev[i].ev.code==BTN_THUMBL    ) ev[i].ev.code=KEY_;
                    else if(ev[i].ev.code==BTN_THUMBR    ) ev[i].ev.code=KEY_LEFTSHIFT;
                    else if(ev[i].ev.code==BTN_DPAD_UP   ) ev[i].ev.code=KEY_UP;
                    else if(ev[i].ev.code==BTN_DPAD_DOWN ) ev[i].ev.code=KEY_DOWN;
                    else if(ev[i].ev.code==BTN_DPAD_LEFT ) ev[i].ev.code=KEY_LEFT;
                    else if(ev[i].ev.code==BTN_DPAD_RIGHT) ev[i].ev.code=KEY_RIGHT;
                    printf("BTN->KEY  ");
                }
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
