#include "io.h"
#ifdef _WIN32
  #undef rc
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #define strtok_r strtok_s
#else
  #include <unistd.h>
  #include <sys/wait.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "x.h"
#include "sym.h"
#include "ops.h"

extern K* null;

static K* split(char *b) {
  size_t n=0,m=32;
  char *p,*q;
  K *r=kv0(m);
  q=strtok_r(b,"\r\n",&p);
  while(q) {
    ++n;
    if(n==m) { m<<=1; r->v=xrealloc(r->v,sizeof(K*)*m); }
    v0(r)[n-1]=knew(-3,strlen(q),q,0,0,0);
    q=strtok_r(0,"\r\n",&p);
  }
  r->c=n;
  return r;
}

#ifdef _WIN32
static char* read_(HANDLE f) {
  size_t m=32,n=0;
  char buf[1024],*b;
  DWORD len;
  BOOL res;

  b=xmalloc(m);
  while(1) {
    res=ReadFile(f,buf,1024,&len,0);
    if(!res||!len) break;
    while(m<n+len) { m<<=1; b=xrealloc(b,m); }
    memcpy(b+n,buf,len);
    n+=len;
  }
  b[n]=0;
  return b;
}

K* b3colon1_(K *a) {
  char *cmd;
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  BOOL res;

  if(at!=-3) return kerror("type");
  if(!ac) return kerror("domain");

  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);

  cmd=xmalloc(ac+8);
  sprintf(cmd,"cmd /c ");
  strncat(cmd,v3(a),ac);

  res=CreateProcess(0,
     cmd,  // command line
     0,    // process security attributes
     0,    // primary thread security attributes
     TRUE, // handles are inherited
     0,    // creation flags
     0,    // use parent's environment
     0,    // use parent's current directory
     &si,  // STARTUPINFO pointer
     &pi); // receives PROCESS_INFORMATION

  xfree(cmd);
  if(!res) return kerror("domain");

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return null;
}

K* b4colon1_(K *a) {
  K *r;
  char *cmd,*buf1,*buf2;
  DWORD status;
  HANDLE out[2],err[2];
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  BOOL res;
  SECURITY_ATTRIBUTES sa;

  if(at!=-3) return kerror("type");
  if(!ac) return kerror("domain");

  // Set the bInheritHandle flag so pipe handles are inherited.
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = 0;

  if(!CreatePipe(&out[0], &out[1], &sa, 0)) return kerror("domain");
  if(!SetHandleInformation(out[0], HANDLE_FLAG_INHERIT, 0)) return kerror("domain");
  if(!CreatePipe(&err[0], &err[1], &sa, 0)) return kerror("domain");
  if(!SetHandleInformation(err[0], HANDLE_FLAG_INHERIT, 0)) return kerror("domain");

  ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
  ZeroMemory(&si, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);
  si.hStdOutput = out[1];
  si.hStdError = err[1];
  si.dwFlags |= STARTF_USESTDHANDLES;

  cmd=xmalloc(ac+8);
  sprintf(cmd,"cmd /c ");
  strncat(cmd,v3(a),ac);

  res=CreateProcess(0,
     cmd,  // command line
     0,    // process security attributes
     0,    // primary thread security attributes
     TRUE, // handles are inherited
     0,    // creation flags
     0,    // use parent's environment
     0,    // use parent's current directory
     &si,  // STARTUPINFO pointer
     &pi); // receives PROCESS_INFORMATION

  xfree(cmd);
  if(!res) return kerror("domain");

  CloseHandle(out[1]);
  CloseHandle(err[1]);
  buf1=read_(out[0]);
  buf2=read_(err[0]);
  fprintf(stderr,"%s",buf2);
  //WaitForSingleObject(pi.hProcess,INFINITE);
  //WaitForSingleObject doesn't work for "cmd1 & cmd2 & cmd3"
  //should be safe to assum process has exited since read_ doesn't return
  //until stdout/stderr is closed
  if(!GetExitCodeProcess(pi.hProcess,&status)) return kerror("domain");
  if(status) return kerror("domain");
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  r=split(buf1);
  xfree(buf1);
  return r;
}

#define BUFSIZE 1024
K* b8colon1_(K *a) {
  K *r;
  char *cmd,*buf1,*buf2;
  HANDLE out[2],err[2];
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  BOOL res;
  SECURITY_ATTRIBUTES sa;

  if(at!=-3) return kerror("type");
  if(!ac) return kerror("domain");

  // Set the bInheritHandle flag so pipe handles are inherited.
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = 0;

  if(!CreatePipe(&out[0], &out[1], &sa, 0)) return kerror("domain");
  if(!CreatePipe(&err[0], &err[1], &sa, 0)) return kerror("domain");
  if(!SetHandleInformation(out[0], HANDLE_FLAG_INHERIT, 0)) return kerror("domain");
  if(!SetHandleInformation(err[0], HANDLE_FLAG_INHERIT, 0)) return kerror("domain");

  ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );
  ZeroMemory( &si, sizeof(STARTUPINFO) );
  si.cb = sizeof(STARTUPINFO);
  si.hStdError = err[1];
  si.hStdOutput = out[1];
  si.dwFlags |= STARTF_USESTDHANDLES;

  cmd=xmalloc(ac+8);
  sprintf(cmd,"cmd /c ");
  strncat(cmd,v3(a),ac);

  res=CreateProcess(0,
     cmd,  // command line
     0,    // process security attributes
     0,    // primary thread security attributes
     TRUE, // handles are inherited
     0,    // creation flags
     0,    // use parent's environment
     0,    // use parent's current directory
     &si,  // STARTUPINFO pointer
     &pi); // receives PROCESS_INFORMATION

  if(!res) return kerror("domain");

  // Close handles to the stdout pipe no longer needed by the child process.
  CloseHandle(out[1]);
  CloseHandle(err[1]);

  buf1=read_(out[0]);
  buf2=read_(err[0]);

  // Close handles to the child process and its primary thread.
  WaitForSingleObject(pi.hProcess,INFINITE);
  DWORD status = 0;
  if(!GetExitCodeProcess(pi.hProcess, &status)) { status = (DWORD)-1; }
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  xfree(cmd);
  r=kv0(3);
  v0(r)[0]=k1(status);
  v0(r)[1]=split(buf1);
  v0(r)[2]=split(buf2);
  xfree(buf1);
  xfree(buf2);
  return r;
}
#else
static char* read_(int f) {
  size_t m=32,n=0;
  char buf[1024],*b;
  ssize_t len;
  b=xmalloc(m);
  while((len=read(f,buf,1024))) {
    while(m<n+len) { m<<=1; b=xrealloc(b,m); }
    memcpy(b+n,buf,len);
    n+=len;
  }
  b[n]=0;
  return b;
}

K* b3colon1_(K *a) {
  char *argv[4];
  if(at!=-3) return kerror("type");
  if(!ac) return kerror("domain");
  argv[0]="/bin/bash";
  argv[1]="-c";
  argv[2]=xstrndup(a->v,ac);
  argv[3]=0;
  if(!fork()) execvp(argv[0],argv);
  xfree(argv[2]);
  return null;
}

K* b4colon1_(K *a) {
  K *r;
  pid_t p;
  char *argv[4],*buf;
  int out[2],status;
  if(at!=-3) return kerror("type");
  if(!ac) return kerror("domain");
  if(pipe(out)) return kerror("pipe");
  argv[0]="/bin/bash";
  argv[1]="-c";
  argv[2]=xstrndup(a->v,ac);
  argv[3]=0;
  if(!(p=fork())) { /* child */
    close(out[0]);
    dup2(out[1],1);
    close(out[1]);
    execvp(argv[0],argv);
  }
  waitpid(p,&status,0);
  close(out[1]);
  buf=read_(out[0]);
  close(out[0]);
  xfree(argv[2]);
  r=status?kerror("domain"):split(buf);
  xfree(buf);
  return r;
}

K* b8colon1_(K *a) {
  K *r;
  pid_t p;
  char *argv[4],*buf1,*buf2;
  int out[2],err[2],status;
  if(at!=-3) return kerror("type");
  if(!ac) return kerror("domain");
  if(pipe(out)) return kerror("pipe");
  if(pipe(err)) return kerror("pipe");
  argv[0]="/bin/bash";
  argv[1]="-c";
  argv[2]=xstrndup(a->v,ac);
  argv[3]=0;
  if(!(p=fork())) { /* child */
    close(out[0]); close(err[0]);
    dup2(out[1],1); dup2(err[1],2);
    close(out[1]); close(err[1]);
    execvp(argv[0],argv);
  }
  waitpid(p,&status,0);
  if(WIFEXITED(status)) status = WEXITSTATUS(status);
  else if(WIFSIGNALED(status)) status = 128 + WTERMSIG(status);
  else status = -1;
  close(out[1]); close(err[1]);
  buf1=read_(out[0]);
  buf2=read_(err[0]);
  close(out[0]); close(err[0]);
  xfree(argv[2]);
  r=kv0(3);
  v0(r)[0]=k1(status);
  v0(r)[1]=split(buf1);
  v0(r)[2]=split(buf2);
  xfree(buf1);
  xfree(buf2);
  return r;
}
#endif
