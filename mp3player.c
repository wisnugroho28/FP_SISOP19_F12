#include <mpg123.h>
#include <ao/ao.h>
#include <termios.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BITS 8
#define WORKDIR "/home/[user]/fuse/"

int playStatus=0;
int finishStatus=0;
int isTransition=0;
int songIndex=0;
int songAmount=0;
char playList[1000][1000];

//fungsi untuk menampilkan playlist
void* display(void *arg);

//fungsi untuk menghandle aksi user melalui keyboard
void* userAction(void *arg);

//fungsi untuk menghandle file mp3
void* musicHandler(void *arg);

//fungsi untuk menghiraukan buffer saat di terminal
void changemode(int);

//fungsi untuk menunggu keyboard hit
int  kbhit(void);

//fungsi untuk check ekstensi file
bool extensionMatch(const char *fileName, const char *ext);

int main(){
    int i=0;
    pthread_t myThread;
    changemode(1);

    // load seluruh lagi kedalam array playList
    // INIT
    DIR *d;
    struct dirent *de;
    if ((d = opendir (WORKDIR)) != NULL) {
        while ((de = readdir (d)) != NULL) {
            struct stat st;
            memset(&st, 0, sizeof(st));
		    st.st_mode = de->d_type << 12;
            if(!S_ISDIR(st.st_mode) || extensionMatch(de->d_name, ".mp3")){
                strcpy(playList[i++], de->d_name);
            }
        }
        closedir (d);
    } else {
        perror ("");
        return EXIT_FAILURE;
    }
    songAmount=i;
    //END INIT

    //jalankan threads
    if(pthread_create(&myThread, NULL, musicHandler, NULL)<0){
        perror("could not create thread");
        exit(EXIT_FAILURE);
    }

    if(pthread_create(&myThread, NULL, display, NULL)<0){
        perror("could not create myThread");
        exit(EXIT_FAILURE);
    }

    if(pthread_create(&myThread, NULL, userAction, NULL)<0){
        perror("could not create myThread");
        exit(EXIT_FAILURE);
    }

    while(1);

    return 0;
}

void* display(void *arg){
    while(1){
        int i;
        for(i=0; i<songAmount;i++){
            if(i == songIndex){
                printf(">> %s\n", playList[i]);
            }else{
                printf("%d %s\n", i, playList[i]);
            }
        }
        sleep(1);
        system("clear");
    }
}

void* userAction(void *arg){
    char ch;
    while(1){
        while(!kbhit());
        ch = getchar();
        if(ch=='p'){
            playStatus=!playStatus;
        }else if(ch=='b'){
            songIndex+=songAmount;
            songIndex--;
            playStatus=0;
            isTransition=1;
            finishStatus=1;
        }else if(ch=='n'){
            songIndex++;
            playStatus=0;
            isTransition=1;
            finishStatus=1;
        }else if(ch=='q'){
            changemode(0);
            exit(0);
        }   
    }
}

void* musicHandler(void *arg){
    mpg123_handle *mh;
    unsigned char *buffer;
    size_t buffer_size;
    size_t done;
    int err;

    int driver;
    ao_device *dev;

    ao_sample_format format;
    int channels, encoding;
    long rate;

    /* initializations */
    ao_initialize();
    driver = ao_default_driver_id();
    mpg123_init();
    mh = mpg123_new(NULL, &err);
    buffer_size = mpg123_outblock(mh);
    buffer = (unsigned char*) malloc(buffer_size * sizeof(unsigned char));

    while(1){
        char file[1000];
        strcpy(file, WORKDIR);
        strcat(file, playList[songIndex]);

        /* open the file and get the decoding format */
        mpg123_open(mh, file);
        mpg123_getformat(mh, &rate, &channels, &encoding);

        /* set the output format and open the output device */
        format.bits = mpg123_encsize(encoding) * BITS;
        format.rate = rate;
        format.channels = channels;
        format.byte_format = AO_FMT_NATIVE;
        format.matrix = 0;
        dev = ao_open_live(driver, &format, NULL);

        if(isTransition){
            playStatus=1;
            isTransition=0;
        }

        finishStatus=0;

        while(!finishStatus){
            sleep(0.000001);
            if(finishStatus){
                break;
            }
            if(playStatus){
                if(mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK){
                    ao_play(dev, buffer, done);
                }else{
                    finishStatus=1;
                }
            }else{
                continue;
            }
        }

        if(playStatus && isTransition){
            songIndex+=1;
        }
        songIndex%=songAmount;
    }

    /* clean up */
    free(buffer);
    ao_close(dev);
    mpg123_close(mh);
    mpg123_delete(mh);
    mpg123_exit();
    ao_shutdown();
    // musicHandler(NULL);
}

void changemode(int dir){
  static struct termios oldt, newt;
 
  if ( dir == 1 )
  {
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
  }
  else
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}
 
int kbhit (void){
  struct timeval tv;
  fd_set rdfs;
 
  tv.tv_sec = 0;
  tv.tv_usec = 0;
 
  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);
 
  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);
}

bool extensionMatch(const char *name, const char *ext){
	size_t nl = strlen(name), el = strlen(ext);
	return nl >= el && !strcmp(name + nl - el, ext);
}
