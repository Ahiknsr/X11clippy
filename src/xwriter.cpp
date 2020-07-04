#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <string.h>
#include <time.h>

void testmemory()
{
    u_int64_t *test = new u_int64_t[10];
    for(int i=0;i<9;i++)
    {
        test[i] = 0x1234567891234567;
    }
    test[1]+=1;
    unsigned char *temp1 = (unsigned char*)test;
    printf("%02X %02X %02X %02X\n", temp1[0], temp1[1], temp1[2], temp1[3]);
    printf("%02X %02X %02X %02X\n", temp1[8+0], temp1[8+1], temp1[8+2], temp1[8+3]);
}


void
send_no(Display *dpy, XSelectionRequestEvent *sev)
{
    XSelectionEvent ssev;
    char *an;

    an = XGetAtomName(dpy, sev->target);
    printf("Denying request of type '%s'\n", an);
    if (an)
        XFree(an);

    /* All of these should match the values of the request. */
    ssev.type = SelectionNotify;
    ssev.requestor = sev->requestor;
    ssev.selection = sev->selection;
    ssev.target = sev->target;
    ssev.property = None;  /* signifies "nope" */
    ssev.time = sev->time;

    XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

void
send_utf8(Display *dpy, XSelectionRequestEvent *sev, Atom utf8)
{
    XSelectionEvent ssev;
    time_t now_tm;
    char *now, *an;

    now_tm = time(NULL);
    now = ctime(&now_tm);

    char* now_new = new char[6];
    for(int i=0;i<6;i++)
    {
        now_new[i] = 0x00+i;
    }

    an = XGetAtomName(dpy, sev->property);
    printf("Sending data to window 0x%lx, property '%s'\n", sev->requestor, an);
    if (an)
        XFree(an);

    // change the property
    int ret = XChangeProperty(dpy, sev->requestor, sev->property, utf8, 8, 
                    PropModeReplace /*discard old data*/,
                    (unsigned char *)now_new, 6);
    printf("ret value %d\n", ret);
    ssev.type = SelectionNotify;
    ssev.requestor = sev->requestor;
    ssev.selection = sev->selection;
    ssev.target = sev->target;
    ssev.property = sev->property;
    ssev.time = sev->time;

    // send the event to requestor
    XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)&ssev);
}

int main()
{
    Display *dpy;
    Window owner, root;
    int screen;
    Atom sel, utf8;
    XEvent ev;
    XSelectionRequestEvent *sev;

    dpy = XOpenDisplay(NULL);
    if (!dpy)
    {
        fprintf(stderr, "Could not open X display\n");
        return 1;
    }

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    /* We need a window to receive messages from other clients. */
    owner = XCreateSimpleWindow(dpy, root, -10, -10, 1, 1, 0, 0, 0);

    sel = XInternAtom(dpy, "CLIPBOARD", False);
    utf8 = XInternAtom(dpy, "UTF89_STRING", False);

    printf("claiming clipboard ownership\n");
    XSetSelectionOwner(dpy, sel, owner, CurrentTime);

    for (;;)
    {
        XNextEvent(dpy, &ev);
        switch (ev.type)
        {
            case SelectionClear:
                printf("Lost selection ownership\n");
                return 1;
                break;
            case SelectionRequest:
                sev = (XSelectionRequestEvent*)&ev.xselectionrequest;
                printf("clipboard request: 0x%lx\n", sev->requestor);
                /* Property is set to None by "obsolete" clients. */
                if (sev->target != utf8 || sev->property == None)
                    send_no(dpy, sev);
                else
                    send_utf8(dpy, sev, utf8);
                break;
        }
    }

    return 0;
}