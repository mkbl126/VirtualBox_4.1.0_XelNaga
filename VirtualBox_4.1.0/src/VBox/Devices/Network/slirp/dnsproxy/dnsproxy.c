/* $Id: dnsproxy.c 37746 2011-07-04 06:07:37Z vboxsync $ */
/*
 * Copyright (c) 2003,2004,2005 Armin Wolfermann
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef VBOX
#include <config.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#else
#include "slirp.h"
#endif

#ifndef VBOX
#define GLOBALS 1
#include "dnsproxy.h"

#define RD(x) (*(x + 2) & 0x01)
#define MAX_BUFSPACE 512

static unsigned short queryid = 0;
#define QUERYID queryid++

static struct sockaddr_in authoritative_addr;
static struct sockaddr_in recursive_addr;
static int sock_query;
static int sock_answer;
static int dnsproxy_sig;

extern int event_gotsig;
extern int (*event_sigcb)(void);

#ifdef DEBUG
char *malloc_options = "AGZ";
#endif

/* signal_handler -- Native signal handler. Set external flag for libevent
 * and store type of signal. Real signal handling is done in signal_event.
 */

RETSIGTYPE
signal_handler(int sig)
{
    event_gotsig = 1;
    dnsproxy_sig = sig;
}

/* signal_event -- Called by libevent to deliver a signal.
 */

int
signal_event(void)
{
    fatal("exiting on signal %d", dnsproxy_sig);
    return 0;
}

#else

# define RD(x) (*(x + 2) & 0x01)
# define MAX_BUFSPACE 512

# define QUERYID queryid++

#endif
/* timeout -- Called by the event loop when a query times out. Removes the
 * query from the queue.
 */
/* ARGSUSED */
#ifndef VBOX
static void
timeout(int fd, short event, void *arg)
{
    /* here we should check if we reached the end of the DNS server list */
    hash_remove_request(pData, (struct request *)arg);
    free((struct request *)arg);
    ++removed_queries;
}
#else /* VBOX */
static void
timeout(PNATState pData, struct socket *so, void *arg)
{
    struct request *req = (struct request *)arg;
    struct dns_entry *de;
    de = TAILQ_PREV(req->dns_server, dns_list_head, de_list);
    /* here we should check if we reached the end of the DNS server list */
    if (de == NULL)
    {
        hash_remove_request(pData, req);
        RTMemFree(req);
        ++removed_queries;
    }
    else
    {
        struct ip *ip;
        struct udphdr *udp;
        int iphlen;
        struct socket *so1 = socreate();
        struct mbuf *m = NULL;
        char *data;
        if (so1 == NULL)
        {
            LogRel(("NAT: can't create DNS socket\n"));
            return;
        }
        if(udp_attach(pData, so1, 0) == -1)
        {
            LogRel(("NAT: can't attach udp socket\n"));
            sofree(pData, so1);
            return;
        }
        m = slirpDnsMbufAlloc(pData);
        if (m == NULL)
        {
            LogRel(("NAT: Can't allocate mbuf\n"));
            udp_detach(pData, so1);
            return;
        }
        /* mbuf initialization */
        m->m_data += if_maxlinkhdr;
        ip = mtod(m, struct ip *);
        udp = (struct udphdr *)&ip[1]; /* ip attributes */
        data = (char *)&udp[1];
        iphlen = sizeof(struct ip);
        m->m_len += sizeof(struct ip);
        m->m_len += sizeof(struct udphdr);
        m->m_len += req->nbyte;
        ip->ip_src.s_addr = so->so_laddr.s_addr;
        ip->ip_dst.s_addr = RT_H2N_U32(RT_N2H_U32(pData->special_addr.s_addr) | CTL_DNS);
        udp->uh_dport = ntohs(53);
        udp->uh_sport = so->so_lport;
        memcpy(data, req->byte, req->nbyte); /* coping initial req */

        so1->so_laddr = so->so_laddr;
        so1->so_lport = so->so_lport;
        so1->so_faddr = so->so_faddr;
        so1->so_fport = so->so_fport;
        req->dns_server = de;
        so1->so_timeout_arg = req;
        so1->so_timeout = timeout;
        dnsproxy_query(pData, so1, m, iphlen);
    }
}
#endif /* VBOX */

/* do_query -- Called by the event loop when a packet arrives at our
 * listening socket. Read the packet, create a new query, append it to the
 * queue and send it to the correct server.
 *
 * Slirp: this routine should be called from udp_input
 * socket is Slirp's construction (here we should set expiration time for socket)
 * mbuf points on ip header to easy fetch information about source and destination.
 * iphlen - len of ip header
 */

/* ARGSUSED */
#ifndef VBOX
static void
do_query(int fd, short event, void *arg)
#else
void
dnsproxy_query(PNATState pData, struct socket *so, struct mbuf *m, int iphlen)
#endif
{
#ifndef VBOX
    char buf[MAX_BUFSPACE];
    unsigned int fromlen = sizeof(fromaddr);
    struct timeval tv;
#else
    struct ip *ip;
    char *buf;
    int retransmit;
    struct udphdr *udp;
#endif
    struct sockaddr_in addr;
    struct request *req = NULL;
#ifndef VBOX
    struct sockaddr_in fromaddr;
#else
    struct sockaddr_in fromaddr = { 0, };
#endif
    int byte = 0;

    ++all_queries;

#ifndef VBOX
    /* Reschedule event */
    event_add((struct event *)arg, NULL);

    /* read packet from socket */
    if ((byte = recvfrom(fd, buf, sizeof(buf), 0,
                (struct sockaddr *)&fromaddr, &fromlen)) == -1) {
        LogRel(("recvfrom failed: %s\n", strerror(errno)));
        ++dropped_queries;
        return;
    }
#else
    ip = mtod(m, struct ip *);
    udp = (struct udphdr *)(m->m_data + iphlen);

    fromaddr.sin_addr.s_addr = ip->ip_src.s_addr;
    fromaddr.sin_port = udp->uh_sport;
    fromaddr.sin_family = AF_INET;

    iphlen += sizeof (struct udphdr);
    byte = m->m_len - iphlen;  /* size of IP header + udp header size */
    /* the validness of ip and udp header has been already checked so we shouldn't care if */
    buf = m->m_data + iphlen;
#endif

    /* check for minimum dns packet length */
    if (byte < 12) {
        LogRel(("query too short from %s\n", inet_ntoa(fromaddr.sin_addr)));
        ++dropped_queries;
        return;
    }

#ifndef VBOX
    /* allocate new request */
    if ((req = calloc(1, sizeof(struct request))) == NULL) {
        LogRel(("calloc failed\n"));
        ++dropped_queries;
        return;
    }

    req->id = QUERYID;
    memcpy(&req->client, &fromaddr, sizeof(struct sockaddr_in));
    memcpy(&req->clientid, &buf[0], 2);
#else
    /* allocate new request */
    req = so->so_timeout_arg; /* in slirp we might re-send the query*/
    if (req == NULL)
    {
        if ((req = RTMemAllocZ(sizeof(struct request) + byte)) == NULL) {
            LogRel(("calloc failed\n"));
            ++dropped_queries;
            return;
        }
    }

    /* fill the request structure */
    if (so->so_timeout_arg == NULL)
    {
        req->id = QUERYID;
        memcpy(&req->client, &fromaddr, sizeof(struct sockaddr_in));
        memcpy(&req->clientid, &buf[0], 2);
        req->dns_server = TAILQ_LAST(&pData->pDnsList, dns_list_head);
        if (req->dns_server == NULL)
        {
            static int fail_counter = 0;
            RTMemFree(req);
            if (fail_counter == 0)
                LogRel(("NAT/dnsproxy: Empty DNS entry (suppressed 100 times)\n"));
            else
                fail_counter = (fail_counter == 100 ? 0 : fail_counter + 1);
            return;

        }
        retransmit = 0;
        so->so_timeout = timeout;
        so->so_timeout_arg = req;
        req->nbyte = byte;
        memcpy(req->byte, buf, byte); /* copying original request */
    }
    else
    {
        retransmit = 1;
    }
#endif

#ifndef VBOX
    /* where is this query coming from? */
    if (is_internal(pData, fromaddr.sin_addr)) {
        req->recursion = RD(buf);
        DPRINTF(("Internal query RD=%d\n", req->recursion));
    } else {
        /* no recursion for foreigners */
        req->recursion = 0;
        DPRINTF(("External query RD=%d\n", RD(buf)));
    }

    /* insert it into the hash table */
    hash_add_request(pData, req);
#else
    req->recursion = 0;
    DPRINTF(("External query RD=%d\n", RD(buf)));
    if (retransmit == 0)
        hash_add_request(pData, req);
#endif

    /* overwrite the original query id */
    memcpy(&buf[0], &req->id, 2);

#ifndef VBOX
    if (req->recursion) {

        /* recursive queries timeout in 90s */
        event_set(&req->timeout, -1, 0, timeout, req);
        tv.tv_sec=recursive_timeout; tv.tv_usec=0;
        event_add(&req->timeout, &tv);

        /* send it to our recursive server */
        if ((byte = sendto(sock_answer, buf, (unsigned int)byte, 0,
                    (struct sockaddr *)&recursive_addr,
                    sizeof(struct sockaddr_in))) == -1) {
            LogRel(("sendto failed: %s\n", strerror(errno)));
            ++dropped_queries;
            return;
        }

        ++recursive_queries;

    } else {

        /* authoritative queries timeout in 10s */
        event_set(&req->timeout, -1, 0, timeout, req);
        tv.tv_sec=authoritative_timeout; tv.tv_usec=0;
        event_add(&req->timeout, &tv);

        /* send it to our authoritative server */
        if ((byte = sendto(sock_answer, buf, (unsigned int)byte, 0,
                    (struct sockaddr *)&authoritative_addr,
                    sizeof(struct sockaddr_in))) == -1) {
            LogRel(("sendto failed: %s\n", strerror(errno)));
            ++dropped_queries;
            return;
        }

#else
        so->so_expire = curtime + recursive_timeout * 1000; /* let's slirp to care about expiration */
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr  = req->dns_server->de_addr.s_addr;
        addr.sin_port = htons(53);
        so->so_expire = curtime + recursive_timeout * 1000; /* let's slirp to care about expiration */
        /* send it to our authoritative server */
        Log2(("NAT: request will be sent to %RTnaipv4 on %R[natsock]\n", addr.sin_addr, so));
        if ((byte = sendto(so->s, buf, (unsigned int)byte, 0,
                    (struct sockaddr *)&addr,
                    sizeof(struct sockaddr_in))) == -1) {
            LogRel(("sendto failed: %s\n", strerror(errno)));
            ++dropped_queries;
            return;
        }
        so->so_state = SS_ISFCONNECTED; /* now it's selected */
        Log2(("NAT: request was sent to %RTnaipv4 on %R[natsock]\n", addr.sin_addr, so));
#endif
        ++authoritative_queries;
#ifndef VBOX
    }
#endif
}

/* do_answer -- Process a packet coming from our authoritative or recursive
 * server. Find the corresponding query and send answer back to querying
 * host.
 *
 * Slirp: we call this from the routine from socrecvfrom routine handling UDP responses.
 * So at the moment of call response already has been readed and packed into the mbuf
 */

/* ARGSUSED */
#ifndef VBOX
static void
do_answer(int fd, short event, void *arg)
#else
void
dnsproxy_answer(PNATState pData, struct socket *so, struct mbuf *m)
#endif
{
#ifndef VBOX
    char buf[MAX_BUFSPACE];
    int byte = 0;
    struct request *query = NULL;

    /* Reschedule event */
    event_add((struct event *)arg, NULL);

    /* read packet from socket */
    if ((byte = recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL)) == -1) {
        LogRel(("recvfrom failed: %s\n", strerror(errno)));
        ++dropped_answers;
        return;
    }
#else
    char *buf;
    int byte;
    struct request *query = NULL;
    byte = m->m_len;
    buf = mtod(m, char *);
#endif

    /* check for minimum dns packet length */
    if (byte < 12) {
        LogRel(("answer too short\n"));
        ++dropped_answers;
        return;
    }

    /* find corresponding query */
#ifdef VBOX
    if ((query = hash_find_request(pData, *((unsigned short *)buf))) == NULL) {
        ++late_answers;
        /* Probably, this request wasn't serviced by
         * dnsproxy so we won't care about it here*/
        so->so_expire = curtime + SO_EXPIREFAST;
        Log2(("NAT: query wasn't found\n"));
        return;
    }
    so->so_timeout = NULL;
    so->so_timeout_arg = NULL;
#else
    if ((query = hash_find_request(pData, *((unsigned short *)&buf))) == NULL) {
        ++late_answers;
        return;
    }
    event_del(&query->timeout);
#endif
    hash_remove_request(pData, query);

    /* restore original query id */
    memcpy(&buf[0], &query->clientid, 2);

#ifndef VBOX
    /* Slirp: will send mbuf to guest by itself */
    /* send answer back to querying host */
    if (sendto(sock_query, buf, (unsigned int)byte, 0,
                (struct sockaddr *)&query->client,
                sizeof(struct sockaddr_in)) == -1) {
        LogRel(("sendto failed: %s\n", strerror(errno)));
        ++dropped_answers;
    } else
        ++answered_queries;

    free(query);
#else
    ++answered_queries;

    RTMemFree(query);
#endif
}

/* main -- dnsproxy main function
 */
#ifndef VBOX
int
main(int argc, char *argv[])
{
    int ch;
    struct passwd *pw = NULL;
    struct sockaddr_in addr;
    struct event evq, eva;
    const char *config = "/etc/dnsproxy.conf";
    int daemonize = 0;

    /* Process commandline arguments */
    while ((ch = getopt(argc, argv, "c:dhV")) != -1) {
        switch (ch) {
        case 'c':
            config = optarg;
            break;
        case 'd':
            daemonize = 1;
            break;
        case 'V':
            fprintf(stderr, PACKAGE_STRING "\n");
            exit(0);
        /* FALLTHROUGH */
        case 'h':
        default:
            fprintf(stderr,
            "usage: dnsproxy [-c file] [-dhV]\n"        \
            "\t-c file  Read configuration from file\n" \
            "\t-d       Detach and run as a daemon\n"   \
            "\t-h       This help text\n"           \
            "\t-V       Show version information\n");
            exit(1);
        }
    }

    /* Parse configuration and check required parameters */
    if (!parse(config))
        fatal("unable to parse configuration");

    if (!authoritative || !recursive)
        fatal("No authoritative or recursive server defined");

    if (!listenat)
        listenat = strdup("0.0.0.0");

    /* Create and bind query socket */
    if ((sock_query = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        fatal("unable to create socket: %s", strerror(errno));

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_addr.s_addr = inet_addr(listenat);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    if (bind(sock_query, (struct sockaddr *)&addr, sizeof(addr)) != 0)
        fatal("unable to bind socket: %s", strerror(errno));

    /* Create and bind answer socket */
    if ((sock_answer = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        fatal("unable to create socket: %s", strerror(errno));

    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;

    if (bind(sock_answer, (struct sockaddr *)&addr, sizeof(addr)) != 0)
        fatal("unable to bind socket: %s", strerror(errno));

    /* Fill sockaddr_in structs for both servers */
    memset(&authoritative_addr, 0, sizeof(struct sockaddr_in));
    authoritative_addr.sin_addr.s_addr = inet_addr(authoritative);
    authoritative_addr.sin_port = htons(authoritative_port);
    authoritative_addr.sin_family = AF_INET;

    memset(&recursive_addr, 0, sizeof(struct sockaddr_in));
    recursive_addr.sin_addr.s_addr = inet_addr(recursive);
    recursive_addr.sin_port = htons(recursive_port);
    recursive_addr.sin_family = AF_INET;

    /* Daemonize if requested and switch to syslog */
    if (daemonize) {
        if (daemon(0, 0) == -1)
            fatal("unable to daemonize");
        log_syslog("dnsproxy");
    }

    /* Find less privileged user */
    if (user) {
        pw = getpwnam(user);
        if (!pw)
            fatal("unable to find user %s", user);
    }

    /* Do a chroot if requested */
    if (chrootdir) {
        if (chdir(chrootdir) || chroot(chrootdir))
            fatal("unable to chroot to %s", chrootdir);
        chdir("/");
    }

    /* Drop privileges */
    if (user) {
        if (setgroups(1, &pw->pw_gid) < 0)
            fatal("setgroups: %s", strerror(errno));
#if defined(HAVE_SETRESGID)
        if (setresgid(pw->pw_gid, pw->pw_gid, pw->pw_gid) < 0)
            fatal("setresgid: %s", strerror(errno));
#elif defined(HAVE_SETREGID)
        if (setregid(pw->pw_gid, pw->pw_gid) < 0)
            fatal("setregid: %s", strerror(errno));
#else
        if (setegid(pw->pw_gid) < 0)
            fatal("setegid: %s", strerror(errno));
        if (setgid(pw->pw_gid) < 0)
            fatal("setgid: %s", strerror(errno));
#endif
#if defined(HAVE_SETRESUID)
        if (setresuid(pw->pw_uid, pw->pw_uid, pw->pw_uid) < 0)
            fatal("setresuid: %s", strerror(errno));
#elif defined(HAVE_SETREUID)
        if (setreuid(pw->pw_uid, pw->pw_uid) < 0)
            fatal("setreuid: %s", strerror(errno));
#else
        if (seteuid(pw->pw_uid) < 0)
            fatal("seteuid: %s", strerror(errno));
        if (setuid(pw->pw_uid) < 0)
            fatal("setuid: %s", strerror(errno));
#endif
    }

    /* Init event handling */
    event_init();

    event_set(&evq, sock_query, EV_READ, do_query, &evq);
    event_add(&evq, NULL);

    event_set(&eva, sock_answer, EV_READ, do_answer, &eva);
    event_add(&eva, NULL);

    /* Zero counters and start statistics timer */
    statistics_start();

    /* Take care of signals */
    if (signal(SIGINT, signal_handler) == SIG_ERR)
        fatal("unable to mask signal SIGINT: %s", strerror(errno));

    if (signal(SIGTERM, signal_handler) == SIG_ERR)
        fatal("unable to mask signal SIGTERM: %s", strerror(errno));

    if (signal(SIGHUP, SIG_IGN) == SIG_ERR)
        fatal("unable to mask signal SIGHUP: %s", strerror(errno));

    event_sigcb = signal_event;

    /* Start libevent main loop */
    event_dispatch();

    return 0;

}
#else
int
dnsproxy_init(PNATState pData)
{
    /* globals initialization */
    authoritative_port = 53;
    authoritative_timeout = 10;
    recursive_port = 53;
    recursive_timeout = 2;
    stats_timeout = 3600;
    dns_port = 53;
    return 0;
}
#endif