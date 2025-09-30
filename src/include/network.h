#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Network interface types
#define NET_TYPE_ETHERNET   1
#define NET_TYPE_LOOPBACK   2

// Protocol families
#define AF_INET     2
#define AF_INET6    10

// Socket types
#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

// IP protocols
#define IPPROTO_TCP  6
#define IPPROTO_UDP  17
#define IPPROTO_ICMP 1

// Network interface structure
struct network_interface {
    char name[16];              // Interface name (eth0, lo, etc.)
    uint8_t mac_addr[6];        // MAC address
    uint32_t ip_addr;           // IPv4 address (network byte order)
    uint32_t netmask;           // Network mask
    uint32_t gateway;           // Default gateway
    uint16_t mtu;               // Maximum transmission unit
    uint32_t flags;             // Interface flags
    void *driver_data;          // Driver-specific data
    struct net_device_ops *ops; // Device operations
};

// Network device operations
struct net_device_ops {
    int (*open)(struct network_interface *netif);
    int (*close)(struct network_interface *netif);
    int (*transmit)(struct network_interface *netif, void *packet, size_t len);
    int (*set_mac)(struct network_interface *netif, uint8_t *mac);
    int (*get_stats)(struct network_interface *netif, struct net_stats *stats);
};

// Network statistics
struct net_stats {
    uint64_t rx_packets;        // Received packets
    uint64_t tx_packets;        // Transmitted packets
    uint64_t rx_bytes;          // Received bytes
    uint64_t tx_bytes;          // Transmitted bytes
    uint32_t rx_errors;         // Receive errors
    uint32_t tx_errors;         // Transmit errors
    uint32_t rx_dropped;        // Dropped on receive
    uint32_t tx_dropped;        // Dropped on transmit
};

// Socket address structures
struct sockaddr {
    uint16_t sa_family;         // Address family
    char sa_data[14];           // Address data
};

struct sockaddr_in {
    uint16_t sin_family;        // AF_INET
    uint16_t sin_port;          // Port number (network byte order)
    uint32_t sin_addr;          // IP address (network byte order)
    uint8_t sin_zero[8];        // Padding
};

// Socket structure (kernel internal)
struct socket {
    int domain;                 // Address family
    int type;                   // Socket type
    int protocol;               // Protocol
    uint32_t local_ip;          // Local IP address
    uint16_t local_port;        // Local port
    uint32_t remote_ip;         // Remote IP address
    uint16_t remote_port;       // Remote port
    void *protocol_data;        // Protocol-specific data
    struct socket_ops *ops;     // Socket operations
};

// Socket operations
struct socket_ops {
    int (*bind)(struct socket *sock, const struct sockaddr *addr, socklen_t addrlen);
    int (*connect)(struct socket *sock, const struct sockaddr *addr, socklen_t addrlen);
    int (*listen)(struct socket *sock, int backlog);
    int (*accept)(struct socket *sock, struct socket **newsock, struct sockaddr *addr, socklen_t *addrlen);
    ssize_t (*send)(struct socket *sock, const void *buf, size_t len, int flags);
    ssize_t (*recv)(struct socket *sock, void *buf, size_t len, int flags);
    int (*close)(struct socket *sock);
};

// Network packet buffer
struct net_packet {
    void *data;                 // Packet data
    size_t len;                 // Packet length
    size_t capacity;            // Buffer capacity
    struct network_interface *netif; // Source/destination interface
    uint8_t protocol;           // Protocol type
    uint32_t timestamp;         // Packet timestamp
};

// Network initialization and management
int network_init(void);
void network_cleanup(void);

// Interface management
int netif_register(struct network_interface *netif);
int netif_unregister(struct network_interface *netif);
struct network_interface *netif_find(const char *name);
int netif_up(struct network_interface *netif);
int netif_down(struct network_interface *netif);

// Packet handling
int net_packet_alloc(struct net_packet **packet, size_t size);
void net_packet_free(struct net_packet *packet);
int net_packet_send(struct network_interface *netif, struct net_packet *packet);

// Socket API (system calls)
int sys_socket(int domain, int type, int protocol);
int sys_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int sys_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int sys_listen(int sockfd, int backlog);
int sys_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t sys_send(int sockfd, const void *buf, size_t len, int flags);
ssize_t sys_recv(int sockfd, void *buf, size_t len, int flags);
int sys_close_socket(int sockfd);

// Network utilities
uint32_t inet_addr(const char *cp);
char *inet_ntoa(uint32_t addr);
uint16_t ntohs(uint16_t netshort);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint32_t htonl(uint32_t hostlong);

// Network configuration
int netif_set_ip(struct network_interface *netif, uint32_t ip, uint32_t netmask, uint32_t gateway);
int netif_get_ip(struct network_interface *netif, uint32_t *ip, uint32_t *netmask, uint32_t *gateway);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_H */