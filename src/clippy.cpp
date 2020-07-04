#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <map>
#include <memory>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

void exit_with_backtrace(std::string errstr);
void logmsg(const char* format, ...);
void notify_user(const std::string summary,const std::string body);
void notify_init();

Display *dpy{nullptr};

const std::string CLIPBOARD_S = "CLIPBOARD";
const std::string INCR_S = "INCR";
const std::string TARGETS_S = "TARGETS";
const std::string TIMESTAMP_S = "TIMESTAMP";
const std::string UTF8_STRING_S = "UTF8_STRING";

Atom CLIPBOARD_A, INCR_A, TARGETS_A, TIMESTAMP_A, UTF8_STRING_A;

std::vector<Atom> pendingSelcQueries;
std::map<std::string, std::vector<char>> targetsMap;

void send_none(XSelectionRequestEvent *sev, std::unique_ptr<XSelectionEvent> ssev)
{
    char *an;

    an = XGetAtomName(dpy, sev->target);
    logmsg("Denying request of type '%s'\n", an);
    if (an)
        XFree(an);

    ssev->property = None;
    XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)ssev.get());
}

void send_bytes(XSelectionRequestEvent *sev, std::unique_ptr<XSelectionEvent> ssev, Atom targetType)
{
    const char* value;
    char *property, *target;
    std::string target_S, notification;
    std::ostringstream notificationStream;

    property = XGetAtomName(dpy, sev->property);
    target = XGetAtomName(dpy, targetType);
    target_S = std::string(target);

    notificationStream << "Sending data to window "<< (void const *)sev->requestor<<
                    " targetType "<<target<<" property "<<property;
    notification = notificationStream.str();
    notify_user("Clippy", notification);
    logmsg("%s\n", notification.c_str());

    if (property)
        XFree(property);
    if (target)
        XFree(target);

    value = &targetsMap[target_S][0];

    XChangeProperty(dpy, sev->requestor, sev->property, targetType, 8, PropModeReplace,
                    (unsigned char *)value, targetsMap[target_S].size());

    ssev->property = sev->property;

    XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)ssev.get());
}

void send_targets(XSelectionRequestEvent *sev, std::unique_ptr<XSelectionEvent> ssev)
{
    char *propertey;
    Atom* atomArray = new Atom[targetsMap.size()];

    propertey = XGetAtomName(dpy, sev->property);
    logmsg("Sending TARGETS to window 0x%lx, property '%s'\n", sev->requestor, propertey);
    if (propertey)
        XFree(propertey);

    int i=0;
    for(auto& it : targetsMap)
    {
        atomArray[i++] = XInternAtom(dpy, it.first.c_str(), False);
    }

    XChangeProperty(dpy, sev->requestor, sev->property, 4, 32, PropModeReplace,
                    (unsigned char*)atomArray, targetsMap.size());


    ssev->property = sev->property;

    XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)ssev.get());

    delete atomArray;
}

void send_timestamp(XSelectionRequestEvent *sev, std::unique_ptr<XSelectionEvent> ssev)
{
    char *an;

    an = XGetAtomName(dpy, sev->property);
    logmsg("Sending TIMESTAMP to window 0x%lx, property '%s'\n", sev->requestor, an);
    if (an)
        XFree(an);

    // this should be handled properly
    // look at XServerClipboard::SendTimestampResponse in chromium for reference
    Time time = 0;
    XChangeProperty(dpy, sev->requestor, sev->property, XA_INTEGER, 32, PropModeReplace,
                    (unsigned char*)time, 1);


    ssev->property = sev->property;

    XSendEvent(dpy, sev->requestor, True, NoEventMask, (XEvent *)ssev.get());
}

void processSelectionNotify(Window w, Atom p)
{
    if(pendingSelcQueries.size() == 0)
    {
            Atom type, *targets;
            int di;
            unsigned long i, nitems, dul;
            unsigned char *prop_ret = NULL;
            char *an = NULL;

            /* Read the first 1024 atoms from this list of atoms. We don't
            * expect the selection owner to be able to convert to more than
            * 1024 different targets. :-) */
            XGetWindowProperty(dpy, w, p, 0, 1024 * sizeof (Atom), False, XA_ATOM,
                            &type, &di, &nitems, &dul, &prop_ret);

            logmsg("Targets response :\n");
            targets = (Atom *)prop_ret;
            for (i = 0; i < nitems; i++)
            {
                an = XGetAtomName(dpy, targets[i]);

                if (targets[i] == TIMESTAMP_A)
                {
                    targetsMap[TIMESTAMP_S] = {};
                }
                else if (targets[i] != TARGETS_A)
                {
                    pendingSelcQueries.push_back(targets[i]);
                }
                logmsg("    '%s'\n", an);
                if (an)
                    XFree(an);
            }
            XFree(prop_ret);
            targetsMap[TARGETS_S] = {};
    }
    else
    {
        Atom da, incr, type;
        int di;
        unsigned long size, dul;
        unsigned char *prop_ret = NULL;

        char *key;
        key = XGetAtomName(dpy, pendingSelcQueries[0]);
        std::string keystr(key);

        /* Dummy call to get type and size. */
        XGetWindowProperty(dpy, w, p, 0, 0, False, AnyPropertyType,
                        &type, &di, &dul, &size, &prop_ret);

        XFree(prop_ret);
        targetsMap[key].resize(size);

        pendingSelcQueries.erase(pendingSelcQueries.begin());
        if (type == INCR_A)
        {
            logmsg("Data too large and INCR mechanism not implemented\n");
            return;
        }

        XGetWindowProperty(dpy, w, p, 0, size, False, AnyPropertyType,
                        &da, &di, &dul, &size, &prop_ret);

        memcpy(&targetsMap[key][0], prop_ret, targetsMap[key].size());

        logmsg("read key %s value %s\n", keystr.c_str(), &targetsMap[key][0]);
        XFree(prop_ret);
    }   
}


int main()
{
    char* atomName;
    std::string atomName_S;
    XEvent ev;

    Window owner, root, target_window;
    int screen;
    std::unique_ptr<XSelectionEvent> ssev;
    Atom target_property;
    
    notify_init();

    dpy = XOpenDisplay(NULL);
    if (!dpy)
    {
        exit_with_backtrace("Could not open X display\n");
    }

    screen = DefaultScreen(dpy);
    root = RootWindow(dpy, screen);
    
    target_window = XCreateSimpleWindow(dpy, root, -10, -10, 1, 1, 0, 0, 0);

    CLIPBOARD_A = XInternAtom(dpy, CLIPBOARD_S.c_str(), False);
    UTF8_STRING_A = XInternAtom(dpy, UTF8_STRING_S.c_str(), False);
    TIMESTAMP_A = XInternAtom(dpy, TIMESTAMP_S.c_str(), False);
    TARGETS_A = XInternAtom(dpy, TARGETS_S.c_str(), False);
    INCR_A = XInternAtom(dpy, INCR_S.c_str(), False);
    target_property = XInternAtom(dpy, "TARGET_PROPERTY", False);

    // Query the targets of current ClipBoard
    XConvertSelection(dpy, CLIPBOARD_A, TARGETS_A, target_property, target_window,
                      CurrentTime);

    for (;;)
    {
        XNextEvent(dpy, &ev);
        XSelectionEvent *sev;
        XSelectionRequestEvent *sreqev;
        switch (ev.type)
        {
            case SelectionNotify:
                sev = (XSelectionEvent*)&ev.xselection;
                if (sev->property == None)
                {
                    if(pendingSelcQueries.size() > 0)
                    {
                        char *currReqFormat = XGetAtomName(dpy, pendingSelcQueries[0]);
                        logmsg("requested convertion could not be performed key %s\n", currReqFormat);
                        if (currReqFormat)
                            XFree(currReqFormat);
                        pendingSelcQueries.erase(pendingSelcQueries.begin());
                    }
                }
                else
                {
                    processSelectionNotify(target_window, target_property);
                }

                // We have copied all the targets from old clipbaord
                // claim clipboard ownership now
                if(pendingSelcQueries.size() == 0)
                {
                    XSetSelectionOwner(dpy, CLIPBOARD_A, target_window, CurrentTime);
                }
                else
                {
                    XConvertSelection(dpy, CLIPBOARD_A, pendingSelcQueries.front(), 
                                      target_property,target_window,CurrentTime);
                }
                break;
            case SelectionClear:
                logmsg("lost clipboard ownership..reclaiming\n");
                sleep(3);
                pendingSelcQueries.clear();
                targetsMap.clear();
                XConvertSelection(dpy, CLIPBOARD_A, TARGETS_A, 
                                    target_property, target_window, CurrentTime);
                break;
            case SelectionRequest:
                sreqev = (XSelectionRequestEvent*)&ev.xselectionrequest;
 
                ssev = std::make_unique<XSelectionEvent>();

                /* All of these should match the values of the request. */
                ssev->type = SelectionNotify;
                ssev->requestor = sreqev->requestor;
                ssev->selection = sreqev->selection;
                ssev->target = sreqev->target;
                ssev->property = None;  /* proper value will be filled in later */
                ssev->time = sreqev->time;

                logmsg("Requestor for paste: 0x%lx\n", sreqev->requestor);
                atomName = XGetAtomName(dpy, sreqev->target);
                atomName_S = std::string(atomName);
                /* Property is set to None by "obsolete" clients. */
                if (sreqev->property == None)
                {
                    send_none(sreqev, std::move(ssev));
                }
                else if (sreqev->target == TARGETS_A && targetsMap.size())
                {
                    send_targets(sreqev, std::move(ssev));
                }
                else if (sreqev->target == TIMESTAMP_A)
                {
                    send_timestamp(sreqev, std::move(ssev));
                }
                else if (targetsMap.find(atomName_S) != targetsMap.end())
                {
                    send_bytes(sreqev, std::move(ssev), sreqev->target);
                }
                else
                {
                    send_none(sreqev, std::move(ssev)); 
                }
                break;
            default:
                exit_with_backtrace("unhandled event type");
                break;
        }
    }
}