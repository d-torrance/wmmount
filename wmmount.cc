// wmmount - The WindowMaker universal mount point
// 05/09/98  Release 1.0 Beta1
// Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>
// This software comes with ABSOLUTELY NO WARRANTY
// This software is free software, and you are welcome to redistribute it
// under certain conditions
// See the README file for a more complete notice.


// Defines, includes and global variables
// --------------------------------------

// User defines - standard
#define WINDOWMAKER false
#define USESHAPE    false
#define AFTERSTEP   false
#define NORMSIZE    64
#define ASTEPSIZE   56
#define NAME        "wmmount"
#define CLASS       "WMMount"

// User defines - custom
#define SYSRCFILE   "/etc/system.wmmount"
#define MOUNTCMD    "mount %s &"
#define UMOUNTCMD   "umount %s &"
#define OPENCMD     "xterm -e mc %s &"
#define BACKCOLOR   "#282828"
#define NTXTFONT    "-*-helvetica-bold-r-*-*-10-*-*-*-*-*-*-*"
#define NTXTCOLOR   "gray90"
#define UTXTFONT    "-*-helvetica-medium-r-*-*-10-*-*-*-*-*-*-*"
#define UTXTCOLOR   "gray90"

// Includes - standard
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Includes - custom
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#ifdef __linux__
#include <mntent.h>
#include <sys/vfs.h>
#endif
#ifdef __FreeBSD__
#include <sys/param.h>
#include <sys/mount.h>
#endif

// X-Windows includes - standard
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

// Pixmaps - standard
Pixmap pm_main;
Pixmap pm_tile;
Pixmap pm_disp;
Pixmap pm_mask;

// Xpm images - standard
#include "XPM/wmmount.xpm"
#include "XPM/tile.xpm"

// Variables for command-line arguments - standard
bool wmaker=WINDOWMAKER;
bool ushape=USESHAPE;
bool astep=AFTERSTEP;
char display[256]="";
char position[256]="";
int winsize;

// Variables for rc file - custom
char mountcmd[256]=MOUNTCMD;
char umountcmd[256]=UMOUNTCMD;
char opencmd[256]=OPENCMD;
char backcolor[256]=BACKCOLOR;
char ntxtfont[256]=NTXTFONT;
char ntxtcolor[256]=NTXTCOLOR;
char utxtfont[256]=UTXTFONT;
char utxtcolor[256]=UTXTCOLOR;

// X-Windows basics - standard
Atom _XA_GNUSTEP_WM_FUNC;
Atom deleteWin;
Display *d_display;
Window w_icon;
Window w_main;
Window w_root;
Window w_activewin;

// X-Windows basics - custom
GC gc_gc;
unsigned long color[3];
Font f_ntxtfont, f_utxtfont;


// Misc custom global variables
// ----------------------------

// Current state information
int curmpoint=0;
bool curmounted;
long curusage=-1;
char curusagestr[16]="";

// For buttons
int btnstate=0;
#define BTNNEXT  1
#define BTNPREV  2
#define BTNMNT   4

// For repeating next and prev buttons
#define RPTINTERVAL   5
int rpttimer=0;

// For double-click info box
#define DCLKINTERVAL  5
int dclktimer=0;

struct MPInfo{
   char name[256];
   char mpoint[256];
   int icon;
   int udisplay;
};
struct MPInfo *mpoint[20];

Pixmap pm_icon[20];
int mpoints=0;
int icons=0;

int ntxtx, ntxty, utxtx, utxty;


// Procedures and functions
// ------------------------

// Procedures and functions - standard
void initXWin(int argc, char **argv);
void freeXWin();
void createWin(Window *win, int x, int y);
unsigned long getColor(char *colorname);
unsigned long mixColor(char *colorname1, int prop1, char *colorname2, int prop2);

// Procedures and functions - custom
void scanArgs(int argc, char **argv);
void readFile();
void checkMount(bool forced=true);
void pressEvent(XButtonEvent *xev);
void releaseEvent(XButtonEvent *xev);
void repaint();
void update();
void drawBtns(int btns);
void drawBtn(int x, int y, int w, int h, bool down);

// Procedures and functions - custom (special)
void newSystem(char *command, char *path=NULL);


// Implementation
// --------------

int main(int argc, char **argv)
{
   scanArgs(argc, argv);
   initXWin(argc, argv);

   readFile();

   XGCValues gcv;
   unsigned long gcm;
   gcm=GCGraphicsExposures;
   gcv.graphics_exposures=false;
   gc_gc=XCreateGC(d_display, w_root, gcm, &gcv);

   color[0]=getColor(backcolor);
   color[1]=getColor(ntxtcolor);
   color[2]=getColor(utxtcolor);

   XFontStruct *xfs;

   xfs=XLoadQueryFont(d_display, ntxtfont);
   f_ntxtfont=xfs->fid;
   ntxtx=6-xfs->min_bounds.lbearing;
   ntxty=5+xfs->ascent;
   utxty=ntxty+xfs->descent;
   XFreeFontInfo(NULL, xfs, 1);

   xfs=XLoadQueryFont(d_display, utxtfont);
   f_utxtfont=xfs->fid;
   utxtx=6-xfs->min_bounds.lbearing;
   utxty=utxty+xfs->ascent;
   XFreeFontInfo(NULL, xfs, 1);

   XpmAttributes xpmattr;
   XpmColorSymbol xpmcsym[1]={{"back_color", NULL, color[0]}};
   xpmattr.numsymbols=1;
   xpmattr.colorsymbols=xpmcsym;
   xpmattr.exactColors=false;
   xpmattr.closeness=40000;
   xpmattr.valuemask=XpmColorSymbols | XpmExactColors | XpmCloseness;
   XpmCreatePixmapFromData(d_display, w_root, wmmount_xpm, &pm_main, &pm_mask, &xpmattr);
   XpmCreatePixmapFromData(d_display, w_root, tile_xpm, &pm_tile, NULL, &xpmattr);
   pm_disp=XCreatePixmap(d_display, w_root, 64, 64, DefaultDepth(d_display, DefaultScreen(d_display)));

   if(wmaker || ushape || astep)
      XShapeCombineMask(d_display, w_activewin, ShapeBounding, winsize/2-32, winsize/2-32, pm_mask, ShapeSet);
   else
      XCopyArea(d_display, pm_tile, pm_disp, gc_gc, 0, 0, 64, 64, 0, 0);

   XSetClipMask(d_display, gc_gc, pm_mask);
   XCopyArea(d_display, pm_main, pm_disp, gc_gc, 0, 0, 64, 64, 0, 0);
   XSetClipMask(d_display, gc_gc, None);

   if(mpoints==0)
      fprintf(stderr,"%s : No mountpoints configured.\n", NAME);
   else{
      checkMount(true);

      XEvent xev;
      XSelectInput(d_display, w_activewin, ExposureMask | ButtonPressMask | ButtonReleaseMask);
      XMapWindow(d_display, w_main);

      bool done=false;
      while(!done){
         while(XPending(d_display)){
            XNextEvent(d_display, &xev);
            switch(xev.type){
             case Expose:
                repaint();
             break;
             case ButtonPress:
                pressEvent(&xev.xbutton);
             break;
             case ButtonRelease:
                releaseEvent(&xev.xbutton);
             break;
             case ClientMessage:
                if(xev.xclient.data.l[0]==deleteWin)
                   done=true;
             break;
            }
         }

         if(dclktimer>0)
            dclktimer--;

         if(btnstate & (BTNPREV | BTNNEXT)){
            rpttimer++;
            if(rpttimer>=RPTINTERVAL){
               if(btnstate & BTNNEXT)
                  curmpoint++;
               else
                  curmpoint--;
               if(curmpoint<0)
                  curmpoint=mpoints-1;
               if(curmpoint>=mpoints)
                  curmpoint=0;
               checkMount(true);
               rpttimer=0;
            }
         }
         else
            checkMount(false);
         XFlush(d_display);
         usleep(50000);
      }
   }

   while(mpoints>0){
      mpoints--;
      free(mpoint[mpoints]);
   }
   while(icons>0){
      icons--;
      XFreePixmap(d_display, pm_icon[icons]);
   }
   XFreeGC(d_display, gc_gc);
   XUnloadFont(d_display, f_ntxtfont);
   XUnloadFont(d_display, f_utxtfont);
   XFreePixmap(d_display, pm_main);
   XFreePixmap(d_display, pm_tile);
   XFreePixmap(d_display, pm_disp);
   XFreePixmap(d_display, pm_mask);
   freeXWin();
   return 0;
}

void initXWin(int argc, char **argv){
   winsize=astep ? ASTEPSIZE : NORMSIZE;

   if((d_display=XOpenDisplay(display))==NULL){
      fprintf(stderr,"%s : Unable to open X display '%s'.\n", NAME, XDisplayName(display));
      exit(1);
   }
   _XA_GNUSTEP_WM_FUNC=XInternAtom(d_display, "_GNUSTEP_WM_FUNCTION", false);
   deleteWin=XInternAtom(d_display, "WM_DELETE_WINDOW", false);

   w_root=DefaultRootWindow(d_display);

   XWMHints wmhints;
   XSizeHints shints;
   shints.x=0;
   shints.y=0;
   shints.flags=0;
   bool pos=(XWMGeometry(d_display, DefaultScreen(d_display), position, NULL, 0, &shints, &shints.x, &shints.y,
      &shints.width, &shints.height, &shints.win_gravity) & (XValue | YValue));
   shints.min_width=winsize;
   shints.min_height=winsize;
   shints.max_width=winsize;
   shints.max_height=winsize;
   shints.base_width=winsize;
   shints.base_height=winsize;
   shints.flags=PMinSize | PMaxSize | PBaseSize;

   createWin(&w_main, shints.x, shints.y);

   if(wmaker || astep || pos)
      shints.flags |= USPosition;
   if(wmaker){
      wmhints.initial_state=WithdrawnState;
      wmhints.flags=WindowGroupHint | StateHint | IconWindowHint;
      createWin(&w_icon, shints.x, shints.y);
      w_activewin=w_icon;
      wmhints.icon_window=w_icon;
   }
   else{
      wmhints.initial_state=NormalState;
      wmhints.flags=WindowGroupHint | StateHint;
      w_activewin=w_main;
   }
   wmhints.window_group=w_main;
   XSetWMHints(d_display, w_main, &wmhints);
   XSetWMNormalHints(d_display, w_main, &shints);
   XSetCommand(d_display, w_main, argv, argc);
   XStoreName(d_display, w_main, NAME);
   XSetIconName(d_display, w_main, NAME);
   XSetWMProtocols(d_display, w_activewin, &deleteWin, 1);
}

void freeXWin(){
   XDestroyWindow(d_display, w_main);
   if(wmaker)
      XDestroyWindow(d_display, w_icon);
   XCloseDisplay(d_display);
}

void createWin(Window *win, int x, int y){
   XClassHint classHint;
   *win=XCreateSimpleWindow(d_display, w_root, x, y, winsize, winsize, 0, 0, 0);
   classHint.res_name=NAME;
   classHint.res_class=CLASS;
   XSetClassHint(d_display, *win, &classHint);
}

unsigned long getColor(char *colorname){
   XColor color;
   XWindowAttributes winattr;
   XGetWindowAttributes(d_display, w_root, &winattr);
   color.pixel=0;
   XParseColor(d_display, winattr.colormap, colorname, &color);
   color.flags=DoRed | DoGreen | DoBlue;
   XAllocColor(d_display, winattr.colormap, &color);
   return color.pixel;
}

unsigned long mixColor(char *colorname1, int prop1, char *colorname2, int prop2){
   XColor color, color1, color2;
   XWindowAttributes winattr;
   XGetWindowAttributes(d_display, w_root, &winattr);
   XParseColor(d_display, winattr.colormap, colorname1, &color1);
   XParseColor(d_display, winattr.colormap, colorname2, &color2);
   color.pixel=0;
   color.red=(color1.red*prop1+color2.red*prop2)/(prop1+prop2);
   color.green=(color1.green*prop1+color2.green*prop2)/(prop1+prop2);
   color.blue=(color1.blue*prop1+color2.blue*prop2)/(prop1+prop2);
   color.flags=DoRed | DoGreen | DoBlue;
   XAllocColor(d_display, winattr.colormap, &color);
   return color.pixel;
}

void scanArgs(int argc, char **argv){
   for(int i=1;i<argc;i++){
      if(strcmp(argv[i], "-h")==0 || strcmp(argv[i], "-help")==0 || strcmp(argv[i], "--help")==0){
         fprintf(stderr, "wmmount - The WindowMaker universal mount point\n05/09/98  Release 1.0 Beta1\n");
         fprintf(stderr, "Copyright (C) 1998  Sam Hawker <shawkie@geocities.com>\n");
         fprintf(stderr, "This software comes with ABSOLUTELY NO WARRANTY\n");
         fprintf(stderr, "This software is free software, and you are welcome to redistribute it\n");
         fprintf(stderr, "under certain conditions\n");
         fprintf(stderr, "See the README file for a more complete notice.\n\n");
         fprintf(stderr, "usage:\n\n   %s [options]\n\noptions:\n\n",argv[0]);
         fprintf(stderr, "   -h | -help | --help    display this help screen\n");
         fprintf(stderr, "   -w                     use WithdrawnState    (for WindowMaker)\n");
         fprintf(stderr, "   -s                     shaped window\n");
         fprintf(stderr, "   -a                     use smaller window    (for AfterStep Wharf)\n");
         fprintf(stderr, "   -position position     set window position   (see X manual pages)\n");
         fprintf(stderr, "   -display display       select target display (see X manual pages)\n");
         exit(0);
      }
      if(strcmp(argv[i], "-w")==0)
         wmaker=!wmaker;
      if(strcmp(argv[i], "-s")==0)
         ushape=!ushape;
      if(strcmp(argv[i], "-a")==0)
         astep=!astep;
      if(strcmp(argv[i], "-position")==0){
         if(i<argc-1){
            i++;
            sprintf(position, "%s", argv[i]);
         }
         continue;
      }
      if(strcmp(argv[i],"-display")==0){
         if(i<argc-1){
            i++;
            sprintf(display, "%s", argv[i]);
         }
         continue;
      }
   }
}

void readFile(){
   FILE *rcfile;
   char rcfilen[256];
   char buf[256];
   int done;
   sprintf(rcfilen, "%s/.wmmount", getenv("HOME"));
   if((rcfile=fopen(rcfilen, "r"))==NULL){
      fprintf(stderr, "%s : Unable to read file '%s'.\n", NAME, rcfilen);
      if((rcfile=fopen(SYSRCFILE, "r"))==NULL){
         fprintf(stderr, "%s : Unable to read file '%s'.\n", NAME, SYSRCFILE);
         return;
      }
   }
   XpmAttributes xpmattr;
   xpmattr.exactColors=false;
   xpmattr.closeness=40000;
   xpmattr.valuemask=XpmExactColors | XpmCloseness;
   do{
      fgets(buf, 250, rcfile);
      if((done=feof(rcfile))==0){
         buf[strlen(buf)-1]=0;
         if(strncmp(buf, "mountcmd=", strlen("mountcmd="))==0)
            sprintf(mountcmd, "%s", buf+strlen("mountcmd="));
         if(strncmp(buf, "umountcmd=", strlen("mountcmd="))==0)
            sprintf(umountcmd, "%s", buf+strlen("umountcmd="));
         if(strncmp(buf, "opencmd=", strlen("opencmd="))==0)
            sprintf(opencmd, "%s", buf+strlen("opencmd="));
         if(strncmp(buf, "backcolor=", strlen("backcolor="))==0)
            sprintf(backcolor, "%s", buf+strlen("backcolor="));
         if(strncmp(buf, "ntxtfont=", strlen("ntxtfont="))==0)
            sprintf(ntxtfont,"%s",buf+strlen("ntxtfont="));
         if(strncmp(buf, "ntxtcolor=", strlen("ntxtcolor="))==0)
            sprintf(ntxtcolor, "%s", buf+strlen("ntxtcolor="));
         if(strncmp(buf, "utxtfont=", strlen("utxtfont="))==0)
            sprintf(utxtfont, "%s", buf+strlen("utxtfont="));
         if(strncmp(buf, "utxtcolor=", strlen("utxtcolor="))==0)
            sprintf(utxtcolor, "%s", buf+strlen("utxtcolor="));
         if(strncmp(buf, "icon ", strlen("icon "))==0){
            XpmReadFileToPixmap(d_display, w_root, buf+strlen("icon "), &pm_icon[icons], NULL, &xpmattr);
            icons++;
         }
         if(strncmp(buf, "mountname ", strlen("mountname "))==0){
            mpoint[mpoints]=(struct MPInfo *)malloc(sizeof(struct MPInfo));
            sprintf(mpoint[mpoints]->name, "%s", buf+strlen("mountname "));
            fscanf(rcfile, " mountpoint %s", mpoint[mpoints]->mpoint);
            fscanf(rcfile, " iconnumber %i", &mpoint[mpoints]->icon);
            fscanf(rcfile, " usagedisplay %s", buf);
            mpoint[mpoints]->udisplay=0;
            if(strcasecmp(buf, "Free")==0)
               mpoint[mpoints]->udisplay=1;
            if(strcasecmp(buf, "Used")==0)
               mpoint[mpoints]->udisplay=2;
            if(strcasecmp(buf, "%Capacity")==0)
               mpoint[mpoints]->udisplay=3;
            mpoints++;
         }
      }
   }  while(done==0);
   fclose(rcfile);
}

void checkMount(bool forced){
   int nmounted=false;
   long nusage;

   dev_t prvdev,dirdev;
   if(strcmp(mpoint[curmpoint]->mpoint, "/")==0)
      // I think I can assume that / is mounted.
      nmounted=true;
   else{
      // Check that the directory above the mount point is on a different device.
      struct stat st;

      stat(mpoint[curmpoint]->mpoint, &st);
      dirdev=st.st_dev;

      char mpstr[256];
      strcpy(mpstr, mpoint[curmpoint]->mpoint);
      if(*(strrchr(mpstr, '\0')-1)=='/')
	 *(strrchr(mpstr, '\0')-1)='\0';        // Trash any terminating '/' character
      *(strrchr(mpstr, '/')+1)='\0';
      stat(mpstr, &st);
      prvdev=st.st_dev;

      if(memcmp(&prvdev, &dirdev, sizeof(dev_t))!=0)
         nmounted=true;
   }
   struct statfs sfs;
   if(nmounted && mpoint[curmpoint]->udisplay!=0){
      statfs(mpoint[curmpoint]->mpoint, &sfs);
      if((sfs.f_blocks-sfs.f_bfree+sfs.f_bavail)==0)
	 nusage=-1;
      else
	 nusage=(sfs.f_blocks-sfs.f_bfree);
   }
   else
      nusage=-1;

   if(nusage!=curusage){
      if(nusage==-1)
         strcpy(curusagestr, "");
      else{
         float value;
	 if(mpoint[curmpoint]->udisplay==1)
            value=sfs.f_bavail;
         if(mpoint[curmpoint]->udisplay==2)
            value=sfs.f_blocks-sfs.f_bfree;
         if(mpoint[curmpoint]->udisplay==3){
            value=(sfs.f_blocks-sfs.f_bfree)*100.0/(sfs.f_blocks-sfs.f_bfree+sfs.f_bavail);
            sprintf(curusagestr, "%.01f%%", value);
         }
         else{
            value*=(float)sfs.f_bsize/1024.0;
            if(value<999.5)
               sprintf(curusagestr, "%.0fkB", value);
            else{
               value/=1024.0;
               if(value>=0 && value<9.5)
                  sprintf(curusagestr, "%1.02fMB", value);
               if(value>=9.5 && value<99.5)
                  sprintf(curusagestr, "%2.01fMB", value);
               if(value>=99.5 && value<999.5)
                  sprintf(curusagestr, "%3.0fMB", value);
               if(value>=999.5){
                  value/=1024.0;
                  sprintf(curusagestr, "%.02fGB", value);
               }
            }
         }
      }
   }

   if(nmounted)
      btnstate|=BTNMNT;
   else
      btnstate&=~BTNMNT;

   if((nmounted!=curmounted) || (nmounted && nusage!=curusage && mpoint[curmpoint]->udisplay!=0) || forced){
      if(nmounted!=curmounted || forced){
         curmounted=nmounted;
         drawBtns(BTNMNT);
         if(mpoint[curmpoint]->udisplay!=0 || forced){
            curusage=nusage;
            update();
         }
      }
      else{
         if(nmounted && nusage!=curusage && mpoint[curmpoint]->udisplay!=0){
            curusage=nusage;
            update();
         }
      }
      repaint();
   }
}

void pressEvent(XButtonEvent *xev){
   int x=xev->x-(winsize/2-32);
   int y=xev->y-(winsize/2-32);
   char cmd[256];
   if(x>=46 && y>=47 && x<=58 && y<=57){
      curmpoint++;
      if(curmpoint>=mpoints)
         curmpoint=0;
      btnstate|=BTNNEXT;
      rpttimer=0;
      drawBtns(BTNNEXT);
      checkMount(true);
      return;
   }
   if(x>=33 && y>=47 && x<=45 && y<=57){
      curmpoint--;
      if(curmpoint<0)
         curmpoint=mpoints-1;
      btnstate|=BTNPREV;
      rpttimer=0;
      drawBtns(BTNPREV);
      checkMount(true);
      return;
   }
   if(x>=5 && y>=47 && x<=32 && y<=57){
      if(curmounted)
         sprintf(cmd, umountcmd, mpoint[curmpoint]->mpoint);
      else
         sprintf(cmd, mountcmd, mpoint[curmpoint]->mpoint);
      newSystem(cmd);
      return;
   }
   if(x>=4 && y>=12 && x<=59 && y<=42){
      if(curmounted){
         // First or second click of double-click?
         if(dclktimer>0){
            sprintf(cmd, opencmd, mpoint[curmpoint]->mpoint);
            newSystem(cmd, mpoint[curmpoint]->mpoint);
            dclktimer=0;
         }
         else
            dclktimer=DCLKINTERVAL;
      }
   }
}

void releaseEvent(XButtonEvent *xev){
   btnstate&=~(BTNPREV | BTNNEXT);
   drawBtns(BTNPREV | BTNNEXT);
   repaint();
}

void repaint(){
   XCopyArea(d_display, pm_disp, w_activewin, gc_gc, 0, 0, 64, 64, winsize/2-32, winsize/2-32);
   XEvent xev;
   while(XCheckTypedEvent(d_display, Expose, &xev));
}

void update(){
   // Needs to be called if the current mountpoint or usage change

   // Copy part of background
   XCopyArea(d_display, pm_main, pm_disp, gc_gc, 4, 4, 56, 39, 4, 4);

   // Copy icon
   XCopyArea(d_display, pm_icon[mpoint[curmpoint]->icon], pm_disp, gc_gc, 0, 0, 32, 10, 25, 30);

   // Display usage
   if(curmounted && mpoint[curmpoint]->udisplay!=0){
      XSetFont(d_display, gc_gc, f_utxtfont);
      XSetForeground(d_display, gc_gc, color[2]);
      XDrawString(d_display, pm_disp, gc_gc, utxtx, utxty, (char *)curusagestr, strlen(curusagestr));
   }

   // Draw mountpoint name text
   XSetFont(d_display, gc_gc, f_ntxtfont);
   XSetForeground(d_display, gc_gc, color[1]);
   XDrawString(d_display, pm_disp, gc_gc, ntxtx, ntxty, (char *)mpoint[curmpoint]->name,
      strlen(mpoint[curmpoint]->name));
}

void drawBtns(int btns){
   if(btns & BTNPREV)
      drawBtn(33, 47, 13, 11, (btnstate & BTNPREV));
   if(btns & BTNNEXT)
      drawBtn(46, 47, 13, 11, (btnstate & BTNNEXT));
   if(btns & BTNMNT)
      drawBtn(5, 47, 28, 11, (btnstate & BTNMNT));
}

void drawBtn(int x, int y, int w, int h, bool down){
   if(!down)
      XCopyArea(d_display, pm_main, pm_disp, gc_gc, x, y, w, h, x, y);
   else{
      XCopyArea(d_display, pm_main, pm_disp, gc_gc, x, y, 1, h-1, x+w-1, y+1);
      XCopyArea(d_display, pm_main, pm_disp, gc_gc, x+w-1, y+1, 1, h-1, x, y);
      XCopyArea(d_display, pm_main, pm_disp, gc_gc, x,y, w-1, 1, x+1, y+h-1);
      XCopyArea(d_display, pm_main, pm_disp, gc_gc, x+1, y+h-1, w-1,1, x, y);
   }
}

void newSystem(char *command, char *path=NULL){
   // I am told that using system(3) is bad programming so I use this instead.
   // I also notice system(3) does odd things to footprint of wmmount in memory.

   int pid=fork();
   int status;
   if(pid==0){
      if(path!=NULL)
         chdir(path);
      char *argv[4]={"sh","-c",command,0};
      execv("/bin/sh", argv);
      exit(127);
   }
   do{
      if(waitpid(pid,&status,0)!=-1)
         return;
   } while(1);
}
