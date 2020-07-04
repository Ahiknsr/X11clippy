#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

void print_clipboard(Display *dpy, Window w, Atom p)
{
    Atom da, incr, type;
    int di;
    unsigned long size, dul;
    unsigned char *prop_ret = NULL;

    /* Dummy call to get type and size. */
    XGetWindowProperty(dpy, w, p, 0, 0, False, AnyPropertyType,
                       &type, &di, &dul, &size, &prop_ret);
    XFree(prop_ret);

    incr = XInternAtom(dpy, "INCR", False);
    if (type == incr)
    {
        fprintf(stderr, "Data too large and INCR mechanism not implemented\n");
        return;
    }

    XGetWindowProperty(dpy, w, p, 0, size, False, AnyPropertyType,
                       &da, &di, &dul, &dul, &prop_ret);
    for(int i=0;i<size;i++)
        printf("%c",*(prop_ret+i));
    fflush(stdout);
    XFree(prop_ret);
}

int main(int argc, char** argv)
{
    Display *dpy;
    Window owner, target_window, root;
    int screen;
    Atom sel, target_property, target_type;
    XEvent ev;
    XSelectionEvent *sev;

    dpy = XOpenDisplay(NULL);
    if (!dpy)
    {
        fprintf(stderr, "Could not open X display\n");
        return 1;
    }

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);

    sel = XInternAtom(dpy, "CLIPBOARD", False);

    owner = XGetSelectionOwner(dpy, sel);
    if (owner == None)
    {
        printf("'CLIPBOARD' has no owner\n");
        return 1;
    }

    /* The selection owner will store the data in a property on this
     * window: */
    target_window = XCreateSimpleWindow(dpy, root, -10, -10, 1, 1, 0, 0, 0);

    /* That's the property used by the owner. Note that it's completely
     * arbitrary. */
    target_property = XInternAtom(dpy, "PENGUIN", False);

    target_type = XInternAtom(dpy, argc>=2 ? argv[1] : "UTF8_STRING", False);
 
    /* Request conversion to UTF-8 or custome type. Not all owners will be able to
     * fulfill that request. */
    XConvertSelection(dpy, sel, target_type, target_property, target_window,
                      CurrentTime);

    for (;;)
    {
        XNextEvent(dpy, &ev);
        switch (ev.type)
        {
            case SelectionNotify:
                sev = (XSelectionEvent*)&ev.xselection;
                if (sev->property == None)
                {
                    fprintf(stderr, "Conversion could not be performed.\n");
                    return 1;
                }
                else
                {
                    print_clipboard(dpy, target_window, target_property);
                    return 0;
                }
                break;
        }
    }

    return 0;
}