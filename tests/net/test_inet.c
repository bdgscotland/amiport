/*
 * test_inet.c — Tests for amiport bsdsocket-shim (inet functions + fd tracking)
 *
 * These test the pure-C parts of the bsdsocket shim that don't require
 * an actual TCP/IP stack: inet address conversion, byte order macros,
 * socket fd tracking, and graceful degradation.
 */

#include "test_framework.h"
#include <amiport-net/socket.h>
#include <amiport-net/netinet/in.h>
#include <amiport-net/arpa/inet.h>
#include <amiport-net/netdb.h>
#include <stdio.h>
#include <string.h>

static const char *verstag = "$VER: test_inet 1.0 (20.03.2026)";
long __stack = 32768;

/* --- inet_addr tests --- */

TEST(inet_addr_valid)
{
    unsigned long addr;

    addr = amiport_inet_addr("127.0.0.1");
    ASSERT(addr != INADDR_NONE);
    ASSERT_EQ(addr, htonl(0x7F000001UL));
}

TEST(inet_addr_zeros)
{
    unsigned long addr;

    addr = amiport_inet_addr("0.0.0.0");
    ASSERT_EQ(addr, INADDR_ANY);
}

TEST(inet_addr_broadcast)
{
    unsigned long addr;

    addr = amiport_inet_addr("255.255.255.255");
    ASSERT_EQ(addr, INADDR_BROADCAST);
}

TEST(inet_addr_invalid)
{
    ASSERT_EQ(amiport_inet_addr("not.an.ip"), INADDR_NONE);
    ASSERT_EQ(amiport_inet_addr("256.0.0.1"), INADDR_NONE);
    ASSERT_EQ(amiport_inet_addr(""), INADDR_NONE);
    ASSERT_EQ(amiport_inet_addr(NULL), INADDR_NONE);
}

/* --- inet_ntoa tests --- */

TEST(inet_ntoa_loopback)
{
    struct in_addr addr;
    char *result;

    addr.s_addr = htonl(0x7F000001UL);
    result = amiport_inet_ntoa(addr);
    ASSERT(result != NULL);
    ASSERT_EQ(strcmp(result, "127.0.0.1"), 0);
}

TEST(inet_ntoa_zeros)
{
    struct in_addr addr;
    char *result;

    addr.s_addr = INADDR_ANY;
    result = amiport_inet_ntoa(addr);
    ASSERT(result != NULL);
    ASSERT_EQ(strcmp(result, "0.0.0.0"), 0);
}

/* --- inet_aton tests --- */

TEST(inet_aton_valid)
{
    struct in_addr addr;
    int result;

    result = amiport_inet_aton("192.168.1.1", &addr);
    ASSERT_EQ(result, 1);
    ASSERT_EQ(addr.s_addr, htonl(0xC0A80101UL));
}

TEST(inet_aton_invalid)
{
    struct in_addr addr;

    ASSERT_EQ(amiport_inet_aton("invalid", &addr), 0);
    ASSERT_EQ(amiport_inet_aton(NULL, &addr), 0);
}

TEST(inet_aton_broadcast)
{
    struct in_addr addr;
    int result;

    result = amiport_inet_aton("255.255.255.255", &addr);
    ASSERT_EQ(result, 1);
    ASSERT_EQ(addr.s_addr, INADDR_BROADCAST);
}

/* --- Byte order tests (68k is big-endian, so these are no-ops) --- */

TEST(byte_order_noop)
{
    /* On big-endian 68k, htons/htonl should be identity */
    ASSERT_EQ(htons(0x1234), 0x1234);
    ASSERT_EQ(htonl(0x12345678UL), 0x12345678UL);
    ASSERT_EQ(ntohs(0x1234), 0x1234);
    ASSERT_EQ(ntohl(0x12345678UL), 0x12345678UL);
}

/* --- sockaddr_in tests --- */

TEST(sockaddr_in_layout)
{
    struct sockaddr_in sa;

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(8080);
    sa.sin_addr.s_addr = htonl(0x7F000001UL);

    ASSERT_EQ(sa.sin_family, AF_INET);
    ASSERT_EQ(sa.sin_port, 8080); /* nop on big-endian */
}

/* --- Socket fd tracking tests --- */

/* These are declared in sockfd.c */
extern void amiport_sockfd_init(void);
extern void amiport_sockfd_cleanup(void);
extern void amiport_sockfd_register(int fd);
extern void amiport_sockfd_unregister(int fd);

TEST(sockfd_tracking)
{
    amiport_sockfd_init();

    /* Initially nothing is a socket */
    ASSERT_EQ(amiport_is_socket(0), 0);
    ASSERT_EQ(amiport_is_socket(5), 0);

    /* Register a socket fd */
    amiport_sockfd_register(5);
    ASSERT_EQ(amiport_is_socket(5), 1);
    ASSERT_EQ(amiport_is_socket(4), 0);
    ASSERT_EQ(amiport_is_socket(6), 0);

    /* Register another */
    amiport_sockfd_register(10);
    ASSERT_EQ(amiport_is_socket(10), 1);
    ASSERT_EQ(amiport_is_socket(5), 1);

    /* Unregister */
    amiport_sockfd_unregister(5);
    ASSERT_EQ(amiport_is_socket(5), 0);
    ASSERT_EQ(amiport_is_socket(10), 1);

    amiport_sockfd_cleanup();
}

TEST(sockfd_out_of_range)
{
    amiport_sockfd_init();

    /* Out of range should not crash */
    ASSERT_EQ(amiport_is_socket(-1), 0);
    ASSERT_EQ(amiport_is_socket(FD_SETSIZE + 1), 0);

    amiport_sockfd_cleanup();
}

/* --- fd_set tests --- */

TEST(net_fd_set_operations)
{
    amiport_net_fd_set fds;

    AMIPORT_NET_FD_ZERO(&fds);

    ASSERT(!AMIPORT_NET_FD_ISSET(0, &fds));
    ASSERT(!AMIPORT_NET_FD_ISSET(31, &fds));

    AMIPORT_NET_FD_SET(7, &fds);
    ASSERT(AMIPORT_NET_FD_ISSET(7, &fds));
    ASSERT(!AMIPORT_NET_FD_ISSET(6, &fds));

    AMIPORT_NET_FD_CLR(7, &fds);
    ASSERT(!AMIPORT_NET_FD_ISSET(7, &fds));
}

/* --- Constants tests --- */

TEST(socket_constants)
{
    /* Verify key constants are defined with expected values */
    ASSERT_EQ(AF_INET, 2);
    ASSERT_EQ(SOCK_STREAM, 1);
    ASSERT_EQ(SOCK_DGRAM, 2);
    ASSERT_EQ(IPPROTO_TCP, 6);
    ASSERT_EQ(IPPROTO_UDP, 17);
    ASSERT_EQ(SHUT_RD, 0);
    ASSERT_EQ(SHUT_WR, 1);
    ASSERT_EQ(SHUT_RDWR, 2);
}

/* --- inet_ntop tests (IPv4 only, pure C) --- */

TEST(inet_ntop_loopback)
{
    struct in_addr addr;
    char buf[16];
    const char *result;

    addr.s_addr = htonl(0x7F000001UL);
    result = amiport_inet_ntop(AF_INET, &addr, buf, (int)sizeof(buf));
    ASSERT(result != NULL);
    ASSERT_EQ(strcmp(buf, "127.0.0.1"), 0);
}

TEST(inet_ntop_zeros)
{
    struct in_addr addr;
    char buf[16];
    const char *result;

    addr.s_addr = INADDR_ANY;
    result = amiport_inet_ntop(AF_INET, &addr, buf, (int)sizeof(buf));
    ASSERT(result != NULL);
    ASSERT_EQ(strcmp(buf, "0.0.0.0"), 0);
}

TEST(inet_ntop_buffer_too_small)
{
    struct in_addr addr;
    char buf[4];

    addr.s_addr = htonl(0x7F000001UL);
    ASSERT(amiport_inet_ntop(AF_INET, &addr, buf, (int)sizeof(buf)) == NULL);
}

TEST(inet_ntop_bad_family)
{
    struct in_addr addr;
    char buf[16];

    addr.s_addr = 0;
    ASSERT(amiport_inet_ntop(99, &addr, buf, (int)sizeof(buf)) == NULL);
}

/* --- inet_pton tests (IPv4 only, pure C) --- */

TEST(inet_pton_valid)
{
    struct in_addr addr;
    int result;

    result = amiport_inet_pton(AF_INET, "10.0.0.1", &addr);
    ASSERT_EQ(result, 1);
    ASSERT_EQ(addr.s_addr, htonl(0x0A000001UL));
}

TEST(inet_pton_invalid)
{
    struct in_addr addr;

    ASSERT_EQ(amiport_inet_pton(AF_INET, "not_an_ip", &addr), 0);
}

TEST(inet_pton_bad_family)
{
    struct in_addr addr;

    ASSERT_EQ(amiport_inet_pton(99, "10.0.0.1", &addr), -1);
}

TEST(inet_pton_roundtrip)
{
    struct in_addr addr;
    char buf[16];
    int pton_result;
    const char *ntop_result;

    pton_result = amiport_inet_pton(AF_INET, "192.168.1.100", &addr);
    ASSERT_EQ(pton_result, 1);

    ntop_result = amiport_inet_ntop(AF_INET, &addr, buf, (int)sizeof(buf));
    ASSERT(ntop_result != NULL);
    ASSERT_EQ(strcmp(buf, "192.168.1.100"), 0);
}

/* --- getaddrinfo tests (numeric addresses only — no DNS on vamos) --- */

TEST(getaddrinfo_numeric)
{
    struct amiport_addrinfo hints;
    struct amiport_addrinfo *res;
    struct sockaddr_in *sa;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICHOST;

    rc = amiport_getaddrinfo("127.0.0.1", "22", &hints, &res);
    ASSERT_EQ(rc, 0);
    ASSERT(res != NULL);
    ASSERT_EQ(res->ai_family, AF_INET);
    ASSERT_EQ(res->ai_socktype, SOCK_STREAM);

    sa = (struct sockaddr_in *)res->ai_addr;
    ASSERT_EQ(sa->sin_port, htons(22));
    ASSERT_EQ(sa->sin_addr.s_addr, htonl(0x7F000001UL));

    amiport_freeaddrinfo(res);
}

TEST(getaddrinfo_passive)
{
    struct amiport_addrinfo hints;
    struct amiport_addrinfo *res;
    struct sockaddr_in *sa;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    rc = amiport_getaddrinfo(NULL, "8080", &hints, &res);
    ASSERT_EQ(rc, 0);
    ASSERT(res != NULL);

    sa = (struct sockaddr_in *)res->ai_addr;
    ASSERT_EQ(sa->sin_port, htons(8080));
    ASSERT_EQ(sa->sin_addr.s_addr, INADDR_ANY);

    amiport_freeaddrinfo(res);
}

TEST(getaddrinfo_numerichost_rejects_name)
{
    struct amiport_addrinfo hints;
    struct amiport_addrinfo *res;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_NUMERICHOST;

    rc = amiport_getaddrinfo("example.com", "80", &hints, &res);
    ASSERT_EQ(rc, EAI_NONAME);
}

TEST(getaddrinfo_bad_family)
{
    struct amiport_addrinfo hints;
    struct amiport_addrinfo *res;
    int rc;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = 99;

    rc = amiport_getaddrinfo("127.0.0.1", "22", &hints, &res);
    ASSERT_EQ(rc, EAI_FAMILY);
}

TEST(gai_strerror_messages)
{
    ASSERT(amiport_gai_strerror(0) != NULL);
    ASSERT(amiport_gai_strerror(EAI_NONAME) != NULL);
    ASSERT(strcmp(amiport_gai_strerror(0), "Success") == 0);
}

/* --- fcntl tests (socket-side only, non-blocking flag) --- */

TEST(fcntl_non_socket_fails)
{
    int rc;

    rc = amiport_fcntl(0, F_GETFL, 0);
    ASSERT_EQ(rc, -1);
}

int main(void)
{
    (void)verstag;
    printf("=== BSD Socket Shim Tests ===\n");

    RUN_TEST(inet_addr_valid);
    RUN_TEST(inet_addr_zeros);
    RUN_TEST(inet_addr_broadcast);
    RUN_TEST(inet_addr_invalid);
    RUN_TEST(inet_ntoa_loopback);
    RUN_TEST(inet_ntoa_zeros);
    RUN_TEST(inet_aton_valid);
    RUN_TEST(inet_aton_invalid);
    RUN_TEST(inet_aton_broadcast);
    RUN_TEST(byte_order_noop);
    RUN_TEST(sockaddr_in_layout);
    RUN_TEST(sockfd_tracking);
    RUN_TEST(sockfd_out_of_range);
    RUN_TEST(net_fd_set_operations);
    RUN_TEST(socket_constants);

    /* New tests for Phase 0 shim extensions */
    RUN_TEST(inet_ntop_loopback);
    RUN_TEST(inet_ntop_zeros);
    RUN_TEST(inet_ntop_buffer_too_small);
    RUN_TEST(inet_ntop_bad_family);
    RUN_TEST(inet_pton_valid);
    RUN_TEST(inet_pton_invalid);
    RUN_TEST(inet_pton_bad_family);
    RUN_TEST(inet_pton_roundtrip);
    RUN_TEST(getaddrinfo_numeric);
    RUN_TEST(getaddrinfo_passive);
    RUN_TEST(getaddrinfo_numerichost_rejects_name);
    RUN_TEST(getaddrinfo_bad_family);
    RUN_TEST(gai_strerror_messages);
    RUN_TEST(fcntl_non_socket_fails);

    return test_summary();
}
