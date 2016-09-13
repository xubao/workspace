#include <alloca.h>
#include <endian.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <systemd/sd-daemon.h>
#include <systemd/sd-event.h>

static int signal_handler(sd_event_source *es, const struct signalfd_siginfo *si, void *userdata) {
        if (si->ssi_signo == SIGINT) {
            printf ("... got SIGINT\n");
        } else if (si->ssi_signo == SIGQUIT) {
            printf ("... got SIGQUIT\n");
            exit(EXIT_SUCCESS);
        } else {
            printf("... got unexpected signal\n");
        }
}

static int io_handler(sd_event_source *es, int fd, uint32_t revents, void *userdata) {
        void *buffer;
        ssize_t n;
        int sz;

        /* UDP enforces a somewhat reasonable maximum datagram size of 64K, we can just allocate the buffer on the stack */
        if (ioctl(fd, FIONREAD, &sz) < 0)
                return -errno;
        buffer = alloca(sz);

        n = recv(fd, buffer, sz, 0);
        if (n < 0) {
                if (errno == EAGAIN)
                        return 0;

                return -errno;
        }

        if (n == 5 && memcmp(buffer, "EXIT\n", 5) == 0) {
                /* Request a clean exit */
                sd_event_exit(sd_event_source_get_event(es), 0);
                return 0;
        }

        fwrite(buffer, 1, n, stdout);
        fflush(stdout);
        return 0;
}

int main(int argc, char *argv[]) {
        sd_event_source *event_source = NULL;
        sd_event *event = NULL;
        int fd = -1, r;
        sigset_t ss;

        r = sd_event_default(&event);
        if (r < 0)
                goto finish;

        if (sigemptyset(&ss) < 0 ||
            sigaddset(&ss, SIGTERM) < 0 ||
            sigaddset(&ss, SIGINT) < 0 ||
            sigaddset(&ss, SIGQUIT) < 0 ) {
                r = -errno;
                goto finish;
        }

        /* Block SIGTERM first, so that the event loop can handle it */
        if (sigprocmask(SIG_BLOCK, &ss, NULL) < 0) {
                r = -errno;
                goto finish;
        }
            

        r = sd_event_add_signal(event, &event_source, SIGINT, signal_handler, NULL);
        if (r < 0)
                goto finish;

        r = sd_event_add_signal(event, &event_source, SIGQUIT, signal_handler, NULL);
        if (r < 0)
                goto finish;
        /* Let's make use of the default handler and "floating" reference features of sd_event_add_signal() */
        /*
        r = sd_event_add_signal(event, NULL, SIGTERM, NULL, NULL);
        if (r < 0)
                goto finish;
        r = sd_event_add_signal(event, NULL, SIGINT, NULL, NULL);
        if (r < 0)
                goto finish;
        */

        /* Enable automatic service watchdog support */
        /*
        r = sd_event_set_watchdog(event, true);
        if (r < 0)
                goto finish;       

        r = sd_event_add_io(event, &event_source, fd, EPOLLIN, io_handler, NULL);
        if (r < 0)
                goto finish;
        */
        (void) sd_notifyf(false,
                          "READY=1\n"
                          "STATUS=Daemon startup completed, processing events.");

        r = sd_event_loop(event);

finish:
        event_source = sd_event_source_unref(event_source);
        event = sd_event_unref(event);

        /*
        if (fd >= 0)
                (void) close(fd);
        */

        if (r < 0)
                fprintf(stderr, "Failure: %s\n", strerror(-r));

        return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
