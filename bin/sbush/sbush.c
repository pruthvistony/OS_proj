#include <stdio.h>

int write(int fd, char * b, int len);
int fork();
int execvpe(char *file, char *argv[], char **envp);

int output(int num)
{
while(1){;}
}

int main(int argc, char *argv[], char *envp[]) {
  //puts("sbush> ");
  char buf[] = "hello from sbush";
  //char ch[] = "in child";
  char par[] = "in parent";
  int pid = 0, rw = 0;
  char *buf1 = buf;
  rw = write(1, buf1, 16);
  pid = fork();

  char *ls_args[] = {"ls", 0};
  /*char *arg1[] ;
  ls_args[0] = 
*/
  if(pid == 0)
  {
    //rw = write(1, &ch[0], 8);
    //output(rw);
   execvpe(ls_args[0],ls_args,envp);
  }
  else
  {
    rw = write(1, &par[0], 9);
    output(rw);
  }

  while(1){;}
  return 0;
}


