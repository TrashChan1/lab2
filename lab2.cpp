//
//modified by: David Carter
//date: Jan 21, 2025
//
//original author: Gordon Griesel
//date:            2025
//purpose:         OpenGL sample program
//
//This program needs some refactoring.
//We will do this in class together.
//

#include <iostream>
using namespace std;
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>

#define CALL_ON_BOXES(method) for (auto& box : g.boxes){ box.method(); }

//some structures
class Global;
class Box;

class Box {
   private:
      void redden()
      {
	 redness += 100;
	 if (redness > 255) redness = 255;
	 if (redness < 0) redness = 0;
      }

   public:
      float w;
      float dir[2];
      float pos[2];
      float redness;
      Box()
      {
	 w = 20.0;
	 dir[0] = 5.0f;
	 dir[1] = 12.0f;
	 pos[0] = 0.0f+w;
	 pos[1] = 100.f;
	 redness = 0;
      }
      void physics(int xres, int yres)
      {
	 pos[0] += dir[0];
	 pos[1] += dir[1];
	 if (pos[0] >= (xres-w)) {
	    pos[0] = (xres-w);
	    dir[0] = -dir[0];
	    redden();
	 }
	 if (pos[0] <= w) {
	    pos[0] = w;
	    dir[0] = -dir[0];
	    redden();
	 }
	 if (pos[1] <= w) {
	    pos[1] = w;
	    dir[1] = -dir[1];
	    redden();
	 }
	 if (pos[1] >= yres-w) {
	    pos[1] = yres - w;
	    dir[1] = -dir[1];
	    redden();
	 }
      }

      void cooling()
      {
	 redness -= 1;
	 if (redness < 0)
	    redness = 0;
      }

      void render_box()
      {
	 glPushMatrix();
	 glColor3ub(redness, 0, 255 - redness);
	 glTranslatef(pos[0], pos[1], 0.0f);
	 glBegin(GL_QUADS);
	 glVertex2f(-w, -w);
	 glVertex2f(-w,  w);
	 glVertex2f( w,  w);
	 glVertex2f( w, -w);
	 glEnd();
	 glPopMatrix();
      }

      void decelerate()
      {
	 dir[0] *= 0.8;
	 dir[1] *= 0.8;
      }

      void accelerate()
      {
	 dir[0] *= 1.2;
	 dir[1] *= 1.2;
      }

};

class Global {
   public:
      int xres, yres;
      Box boxes[2];
     //float w;
     //float dir[2];
     //float pos[2];
     //float redness;
      Global()
      {
	 xres = 400;
	 yres = 200;
	 boxes[0].w = 20.0;
	 boxes[0].dir[0] = 10.0f;
	 boxes[0].dir[1] = 4.0f;
	 boxes[0].pos[0] = 0.0f+boxes[0].w;
	 boxes[0].pos[1] = yres/2.0f; 
	 boxes[0].redness = 0;
      }

} g;

class X11_wrapper {
   private:
      Display *dpy;
      Window win;
      GLXContext glc;
   public:
      ~X11_wrapper();
      X11_wrapper();
      void set_title();
      bool getXPending();
      XEvent getXNextEvent();
      void swapBuffers();
      void reshape_window(int width, int height);
      void check_resize(XEvent *e);
      void check_mouse(XEvent *e);
      int check_keys(XEvent *e);
} x11;

//Function prototypes
void init_opengl(void);
void physics_all(void);
void render(void);


int main()
{
   init_opengl();
   int done = 0;
   //main game loop
   while (!done) {
      //look for external events such as keyboard, mouse.
      while (x11.getXPending()) {
	 XEvent e = x11.getXNextEvent();
	 x11.check_resize(&e);
	 x11.check_mouse(&e);
	 done = x11.check_keys(&e);
      }
      physics_all();
      render();
      x11.swapBuffers();
      usleep(200);
   }
   return 0;
}

X11_wrapper::~X11_wrapper()
{
   XDestroyWindow(dpy, win);
   XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
   GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
   int w = g.xres, h = g.yres;
   dpy = XOpenDisplay(NULL);
   if (dpy == NULL) {
      cout << "\n\tcannot connect to X server\n" << endl;
      exit(EXIT_FAILURE);
   }
   Window root = DefaultRootWindow(dpy);
   XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
   if (vi == NULL) {
      cout << "\n\tno appropriate visual found\n" << endl;
      exit(EXIT_FAILURE);
   } 
   Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
   XSetWindowAttributes swa;
   swa.colormap = cmap;
   swa.event_mask =
      ExposureMask | KeyPressMask | KeyReleaseMask |
      ButtonPress | ButtonReleaseMask |
      PointerMotionMask |
      StructureNotifyMask | SubstructureNotifyMask;
   win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
	 InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
   set_title();
   glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
   glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
   //Set the window title bar.
   XMapWindow(dpy, win);
   XStoreName(dpy, win, "3350 Lab-2");
}

bool X11_wrapper::getXPending()
{
   //See if there are pending events.
   return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
   //Get a pending event.
   XEvent e;
   XNextEvent(dpy, &e);
   return e;
}

void X11_wrapper::swapBuffers()
{
   glXSwapBuffers(dpy, win);
}

void X11_wrapper::reshape_window(int width, int height)
{
   //Window has been resized.
   g.xres = width;
   g.yres = height;
   //
   glViewport(0, 0, (GLint)width, (GLint)height);
   glMatrixMode(GL_PROJECTION); glLoadIdentity();
   glMatrixMode(GL_MODELVIEW); glLoadIdentity();
   glOrtho(0, g.xres, 0, g.yres, -1, 1);
}

void X11_wrapper::check_resize(XEvent *e)
{
   //The ConfigureNotify is sent by the
   //server if the window is resized.
   if (e->type != ConfigureNotify)
      return;
   XConfigureEvent xce = e->xconfigure;
   if (xce.width != g.xres || xce.height != g.yres) {
      //Window size did change.
      reshape_window(xce.width, xce.height);
   }
}
//-----------------------------------------------------------------------------

void X11_wrapper::check_mouse(XEvent *e)
{
   static int savex = 0;
   static int savey = 0;

   //Weed out non-mouse events
   if (e->type != ButtonRelease &&
	 e->type != ButtonPress &&
	 e->type != MotionNotify) {
      //This is not a mouse event that we care about.
      return;
   }
   //
   if (e->type == ButtonRelease) {
      return;
   }
   if (e->type == ButtonPress) {
      if (e->xbutton.button==1) {
	 //Left button was pressed.
	 //int y = g.yres - e->xbutton.y;
	 return;
      }
      if (e->xbutton.button==3) {
	 //Right button was pressed.
	 return;
      }
   }
   if (e->type == MotionNotify) {
      //The mouse moved!
      if (savex != e->xbutton.x || savey != e->xbutton.y) {
	 savex = e->xbutton.x;
	 savey = e->xbutton.y;
	 //Code placed here will execute whenever the mouse moves.


      }
   }
}

int X11_wrapper::check_keys(XEvent *e)
{
   if (e->type != KeyPress && e->type != KeyRelease)
      return 0;
   int key = XLookupKeysym(&e->xkey, 0);
   if (e->type == KeyPress) {
      switch (key) {
	 case XK_a:
	    //the 'a' key was pressed
	    CALL_ON_BOXES(decelerate)
	    break;
	 case XK_d:
	    CALL_ON_BOXES(accelerate)
	    break;
	 case XK_Escape:
	    //Escape key was pressed
	    return 1;
      }
   }
   return 0;
}

void init_opengl(void)
{
   //OpenGL initialization
   glViewport(0, 0, g.xres, g.yres);
   //Initialize matrices
   glMatrixMode(GL_PROJECTION); glLoadIdentity();
   glMatrixMode(GL_MODELVIEW); glLoadIdentity();
   //Set 2D mode (no perspective)
   glOrtho(0, g.xres, 0, g.yres, -1, 1);
   //Set the screen background color
   glClearColor(0.1, 0.1, 0.1, 1.0);
}

void physics_all()
{
   for (auto& box : g.boxes){
      box.physics(g.xres, g.yres);
   }
}

void render()
{
   //clear the window
   glClear(GL_COLOR_BUFFER_BIT);
   CALL_ON_BOXES(cooling)
   //draw the box

   if (2 * g.boxes[0].w > g.xres || 2 * g.boxes[0].w + 50 > g.yres)
      return;
   //render_boxes_all();
   CALL_ON_BOXES(render_box)
}






