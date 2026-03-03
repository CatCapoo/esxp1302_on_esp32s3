/*
 * test_w5500_udp.c  –  W5500 Ethernet + UDP bringup test for STM32F407
 *
 * Test sequence:
 *   1. Initialize W5500 (SPI2, PA2=RESET, PA3=NSS)
 *   2. Print network configuration (MAC, IP, etc.)
 *   3. Open a UDP socket on local_port (default 5005)
 *   4. Send a "hello" packet to the remote server
 *   5. Enter echo loop: receive data, print it, echo back with prefix
 *
 * PC-side test:
 *   python scripts/udp_test.py [--port 5005] [--target 192.168.10.15]
 *
 * Prerequisites:
 *   - W5500 connected to SPI2 (PC2/PC3/PB10), NSS=PA3, RESET=PA2
 *   - Ethernet cable connected, PC on same subnet (192.168.10.x)
 */

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "wizchip_conf.h"
#include "socket.h"
#include "w5500.h"
#include "cmsis_os.h"

/* ------------------------------------------------------------------ */
/*  Configuration                                                      */
/* ------------------------------------------------------------------ */
#define W5500_UDP_SOCK       0          /* W5500 socket number (0-7)   */
#define W5500_LOCAL_PORT     5005       /* local UDP port              */

/* Remote server (PC) – only used for the initial "hello" packet */
static uint8_t  remote_ip[4]   = {192, 168, 10, 1};
static uint16_t remote_port    = 5005;

/* ------------------------------------------------------------------ */
/*  Helpers                                                            */
/* ------------------------------------------------------------------ */

static void print_network_info(void)
{
    wiz_NetInfo info;
    ctlnetwork(CN_GET_NETINFO, (void *)&info);
    printf("[W5500] MAC : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
           info.mac[0], info.mac[1], info.mac[2],
           info.mac[3], info.mac[4], info.mac[5]);
    printf("[W5500] IP  : %d.%d.%d.%d\r\n",
           info.ip[0], info.ip[1], info.ip[2], info.ip[3]);
    printf("[W5500] SN  : %d.%d.%d.%d\r\n",
           info.sn[0], info.sn[1], info.sn[2], info.sn[3]);
    printf("[W5500] GW  : %d.%d.%d.%d\r\n",
           info.gw[0], info.gw[1], info.gw[2], info.gw[3]);
    printf("[W5500] DHCP: %s\r\n",
           info.dhcp == NETINFO_STATIC ? "Static" : "DHCP");
}

/* ------------------------------------------------------------------ */
/*  Main test entry point                                              */
/* ------------------------------------------------------------------ */

void test_w5500_udp(void)
{
    uint8_t  rx_buf[256];
    char     tx_buf[300];
    uint8_t  peer_ip[4];
    uint16_t peer_port;
    int32_t  rx_len;
    int      tx_len;

    printf("\r\n========================================\r\n");
    printf(" TEST: W5500 Ethernet + UDP\r\n");
    printf("========================================\r\n");

    /* 1. Init W5500 chip (reset, SPI callbacks, network config) */
    printf("[W5500] Initializing...\r\n");
    W5500_ChipInit();
    printf("[W5500] Init OK\r\n");

    /* 2. Print network info */
    print_network_info();

    /* 3. Open UDP socket */
    printf("[W5500] Opening UDP socket %d on port %d\r\n",
           W5500_UDP_SOCK, W5500_LOCAL_PORT);

    int8_t ret = socket(W5500_UDP_SOCK, Sn_MR_UDP, W5500_LOCAL_PORT, 0);
    if (ret != W5500_UDP_SOCK) {
        printf("[W5500] ERROR: socket() returned %d\r\n", ret);
        return;
    }

    /* 4. Send initial "hello" to remote server */
    {
        const char *hello = "Hello from STM32F407 W5500!";
        int32_t n = sendto(W5500_UDP_SOCK,
                           (uint8_t *)hello, strlen(hello),
                           remote_ip, remote_port);
        printf("[W5500] Sent hello (%ld bytes) to %d.%d.%d.%d:%d\r\n",
               (long)n,
               remote_ip[0], remote_ip[1], remote_ip[2], remote_ip[3],
               remote_port);
    }

    /* 5. Echo loop */
    printf("[W5500] Entering echo loop...\r\n");

    uint32_t pkt_cnt = 0;
    while (1) {
        /* Check for received data */
        uint16_t rsr = getSn_RX_RSR(W5500_UDP_SOCK);
        if (rsr > 0) {
            rx_len = recvfrom(W5500_UDP_SOCK, rx_buf, sizeof(rx_buf) - 1,
                              peer_ip, &peer_port);
            if (rx_len > 0) {
                pkt_cnt++;
                rx_buf[rx_len] = '\0';
                printf("[W5500] #%lu Recv %ld bytes from %d.%d.%d.%d:%d => %s\r\n",
                       (unsigned long)pkt_cnt, (long)rx_len,
                       peer_ip[0], peer_ip[1], peer_ip[2], peer_ip[3],
                       peer_port, rx_buf);

                /* Echo back with prefix */
                tx_len = snprintf(tx_buf, sizeof(tx_buf),
                                  "Echo #%lu: %s",
                                  (unsigned long)pkt_cnt, rx_buf);
                sendto(W5500_UDP_SOCK,
                       (uint8_t *)tx_buf, tx_len,
                       peer_ip, peer_port);
            }
        }
        osDelay(10);   /* yield to other tasks */
    }
}
