/*
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

//Server program for use with websocketd

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>

#define ON 1
#define OFF 0
#define UNKNOWN 2

#define CONTROLS 2

int expectedState[CONTROLS];

int dowrite(){
  FILE *file;
  file = fopen("/home/acarle/relay/status", "w+");
  if (file){
    for (int i = 0; i < CONTROLS; i++){
      fprintf(file, "%s", (expectedState[i] == ON) ? "1" : "0");
    }
    fclose(file);
    return 0;
  }else{
    return 1;
  }
}

void wsSendState(int control){
  char buffer[100];
  sprintf(buffer, "{\"control\":%d, \"state\": \"%s\"}", control, (expectedState[control] == ON) ? "ON" : "OFF");
  fprintf(stdout, "%s\n", buffer);
}

void wsSendAll(){
  for (int i = 0; i < CONTROLS; i++){
    wsSendState(i);
  }
}

int checkSocket(){
  char buffer[1000];
  struct pollfd fds;
  int ret;
  fds.fd = 0; /* this is STDIN */
  fds.events = POLLIN;
  ret = poll(&fds, 1, 0);
  if(ret == 1){
    int i = 0;
    int num = -1;
    char command[1000];
    
    do{
      read(0, buffer + i, 1);
      i++;
    }while(buffer[i - 1] != '\n' && i < 1000);
    if (i == 1000) return i;
    buffer[i - 1] = 0;
    
    sscanf(buffer, "%s %d", command, &num);
    if (! (strcmp(command, "ON"))){
      expectedState[num] = ON;
      return dowrite();
    }else if (! (strcmp(command, "OFF"))){
      expectedState[num] = OFF;
      return dowrite();
    }else if (! (strcmp(command, "GET"))){
      wsSendAll();
    }
  }
}

int checkFile(){
  FILE *file;
  char buffer[CONTROLS + 4];
  file = fopen("/home/acarle/relay/status", "r");
  if (file){
    fgets(buffer, (CONTROLS + 1), file);
    fclose(file);
    //fprintf(stderr, "buffer is: *%s*\n", buffer);
    for (int i = 0; i < CONTROLS; i++){
      int state = UNKNOWN;
      if (buffer[i] == '1'){
        state = ON;
        //fprintf(stderr, "%s\n", "state is on");
      }else if(buffer[i] == '0'){
        state = OFF;
        //fprintf(stderr, "%s\n", "state is off");
      }
      
      if (expectedState[i] != UNKNOWN && expectedState[i] != state && state != UNKNOWN){
        //fprintf(stderr, "%s\n", "which was not expected");
        expectedState[i] = state;
        wsSendState(i);
      }else if(expectedState[i] == UNKNOWN){
        expectedState[i] = state;
      }
    }
    return 0;
  }else{
    fprintf(stderr, "%s\n", "COULD NOT OPEN FILE\n");
    return 1;
  }
}

void initState(){
  for (int i = 0; i < CONTROLS; i++){
    expectedState[i] = UNKNOWN;
  }
}

int main(){
  initState();
  setbuf(stdout, NULL);
  while (1){
    checkFile();
    checkSocket();
    sleep(1);
  }
  return 0;
}
