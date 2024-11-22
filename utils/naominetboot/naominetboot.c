/* KallistiOS ##version##

   naomibintool.c
   Copyright (C) 2020 Lawrence Sebald

*/

/* Functionality adapted from the Triforce Netfirm Toolbox, which was put into
   the public domain by debugmode.

   This program only implements the bare minimum functionality to upload a
   program to a NAOMI NetDIMM and doesn't try to do any more than that. */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define NETDIMM_PORT 10703

/* This function is taken (with minor modifications) from:
   KallistiOS ##version##

   kernel/net/net_crc.c
   Copyright (C) 2009, 2010, 2012 Lawrence Sebald

*/
uint32_t crc32le(uint32_t crc, const uint8_t *data, int size) {
    int i;
    uint32_t rv = ~crc;

    for(i = 0; i < size; ++i) {
        rv ^= data[i];
        rv = (0xEDB88320 & (-(rv & 1))) ^ (rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^ (rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^ (rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^ (rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^ (rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^ (rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^ (rv >> 1);
        rv = (0xEDB88320 & (-(rv & 1))) ^ (rv >> 1);
    }

    return ~rv;
}

ssize_t send_restart_cmd(int s) {
    uint8_t cmd[4] = { 0x00, 0x00, 0x00, 0x0A };
    return send(s, cmd, 4, 0);
}

ssize_t send_prog_info(int s, uint32_t crc, uint32_t len) {
    uint8_t cmd[16] = { 0x0C, 0x00, 0x00, 0x19 };

    cmd[4] = (uint8_t)(crc & 0xFF);
    cmd[5] = (uint8_t)((crc >> 8) & 0xFF);
    cmd[6] = (uint8_t)((crc >> 16) & 0xFF);
    cmd[7] = (uint8_t)((crc >> 24) & 0xFF);
    cmd[8] = (uint8_t)(len & 0xFF);
    cmd[9] = (uint8_t)((len >> 8) & 0xFF);
    cmd[10] = (uint8_t)((len >> 16) & 0xFF);
    cmd[11] = (uint8_t)((len >> 24) & 0xFF);
    cmd[12] = cmd[13] = cmd[14] = cmd[15] = 0;

    return send(s, cmd, 16, 0);
}

ssize_t set_key(int s, const uint8_t key[8]) {
    uint8_t cmd[12] = { 0x08, 0x00, 0x00, 0x7F };

    memcpy(&cmd[4], key, 8);
    return send(s, cmd, 12, 0);
}

ssize_t set_null_key(int s) {
    uint8_t key[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    return set_key(s, key);
}

ssize_t upload_data(int s, uint32_t addr, uint32_t len, const uint8_t data[]) {
    uint8_t cmd[14];

    len += 0x0A;
    cmd[0] = (uint8_t)(len & 0xFF);
    cmd[1] = (uint8_t)((len >> 8) & 0xFF);
    cmd[2] = 0x80;
    cmd[3] = 0x04;
    cmd[4] = cmd[5] = cmd[6] = cmd[7] = 0;
    cmd[8] = (uint8_t)(addr & 0xFF);
    cmd[9] = (uint8_t)((addr >> 8) & 0xFF);
    cmd[10] = (uint8_t)((addr >> 16) & 0xFF);
    cmd[11] = (uint8_t)((addr >> 24) & 0xFF);
    cmd[12] = cmd[13] = 0;

    if(send(s, cmd, 14, 0) < 14) {
        perror("Upload Failed");
    }

    return send(s, data, len - 0x0A, 0);
}

ssize_t finalize_upload(int s, uint32_t addr) {
    uint8_t cmd[22];

    cmd[0] = 0x12;
    cmd[1] = 0;
    cmd[2] = 0x81;
    cmd[3] = 0x04;
    cmd[4] = cmd[5] = cmd[6] = cmd[7] = 0;
    cmd[8] = (uint8_t)(addr & 0xFF);
    cmd[9] = (uint8_t)((addr >> 8) & 0xFF);
    cmd[10] = (uint8_t)((addr >> 16) & 0xFF);
    cmd[11] = (uint8_t)((addr >> 24) & 0xFF);
    cmd[12] = cmd[13] = 0;
    cmd[14] = '1';
    cmd[15] = '2';
    cmd[16] = '3';
    cmd[17] = '4';
    cmd[18] = '5';
    cmd[19] = '6';
    cmd[20] = '7';
    cmd[21] = '8';

    return send(s, cmd, 22, 0);
}

ssize_t set_time_limit(int s, uint32_t limit) {
    uint8_t cmd[8] = { 0x04, 0x00, 0x00, 0x17 };

    cmd[4] = (uint8_t)(limit & 0xFF);
    cmd[5] = (uint8_t)((limit >> 8) & 0xFF);
    cmd[6] = (uint8_t)((limit >> 16) & 0xFF);
    cmd[7] = (uint8_t)((limit >> 24) & 0xFF);

    return send(s, cmd, 8, 0);
}

ssize_t set_host_mode(int s, uint8_t v_and, uint8_t v_or) {
    uint8_t cmd[8] = { 0x04, 0x00, 0x00, 0x07 };

    cmd[4] = (uint8_t)(v_or);
    cmd[5] = (uint8_t)(v_and);
    cmd[6] = cmd[7] = 0;

    if(send(s, cmd, 8, 0) != 8) {
        perror("set host mode");
        return -1;
    }

    if(recv(s, cmd, 8, 0) != 8) {
        perror("set host mode");
        return -1;
    }

#ifdef DEBUG
    printf("Set Host Mode: %02x %02x %02x %02x %02x %02x %02x %02x\n",
           cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);
#endif

    return 8;
}

static uint8_t buf[0x8000];

int connect_and_send(struct sockaddr_in *a, const char *fn, int ka) {
    FILE *fp;
    int s;
    uint32_t crc = 0, size = 0, addr = 0, crc2 = 0;

    if(!(fp = fopen(fn, "rb"))) {
        perror("Error opening file");
        return -1;
    }

    if((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("Create socket");
        fclose(fp);
        return -2;
    }

    if(connect(s, (struct sockaddr *)a, sizeof(struct sockaddr_in)) < 0) {
        perror("Connect socket");
        close(s);
        fclose(fp);
        return -3;
    }

    if(set_host_mode(s, 0, 1) < 0) {
        fprintf(stderr, "Error setting host mode\n");
        close(s);
        fclose(fp);
        return -4;
    }

    if(set_null_key(s) < 0) {
        fprintf(stderr, "Error setting null key\n");
        close(s);
        fclose(fp);
        return -5;
    }

    /* Send file */
    while(!feof(fp)) {
        size = fread(buf, 1, 0x8000, fp);
        if(size != 0) {
            printf("%08x\n", addr);
            if(upload_data(s, addr, size, buf) < 0) {
                fprintf(stderr, "Error uploading data\n");
                close(s);
                fclose(fp);
                return -6;
            }

            crc = crc32le(crc, buf, size);

            addr += size;
        }
    }

    printf("%08x\n", addr);
    fclose(fp);
    crc = ~crc;

    if(finalize_upload(s, addr) < 0) {
        fprintf(stderr, "Error finalizing upload\n");
        close(s);
        return -7;
    }

    if(send_prog_info(s, crc, addr) < 0) {
        fprintf(stderr, "Error sending program information\n");
        close(s);
        return -8;
    }

    if(send_restart_cmd(s) < 0) {
        fprintf(stderr, "Error sending restart command\n");
        close(s);
        return -9;
    }

    if(set_time_limit(s, 10 * 60 * 1000) < 0) {
        fprintf(stderr, "Error setting time limit\n");
        close(s);
        return -10;
    }

    printf("Entering Keep Alive Loop. CTRL + C will end the program.\n");
    /* If the keep alive flag is set, send a packet to the NetDIMM every few
       seconds to keep it awake without a security PIC. */
    if(ka) {
        sleep(20);
        for(;;) {
            set_time_limit(s, 10 * 60 * 1000);
            sleep(5);
        }
    }

    close(s);
    return 0;
}

void usage(const char *progname) {
    printf("Usage: %s -t ip -x prog\n\n", progname);
    printf("Arguments:\n"
           "  -t ip    - Specify the IP of the NAOMI.\n"
           "  -x prog  - Load and execute the NAOMI rom file 'prog'.\n"
           "  -a       - Attempt to keep the NAOMI awake without a PIC.\n");
}

int main(int argc, char *argv[]) {
    int c;
    struct sockaddr_in naomi_addr;
    char *progfn = NULL;
    int keepalive = 0;

    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);

    /* Ignore SIGPIPEs */
    sa.sa_handler = SIG_IGN;

    if(sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    memset(&naomi_addr, 0, sizeof(struct sockaddr_in));

    if(argc < 5) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
   
    /* Parse arguments. */
    while((c = getopt(argc, argv, ":t:x:a")) != -1) {
        switch(c) {
            case 't':
                if(inet_pton(AF_INET, optarg, &naomi_addr.sin_addr) != 1) {
                    fprintf(stderr, "Invalid IP address specified.\n");
                    goto err;
                }

                naomi_addr.sin_family = AF_INET;
                naomi_addr.sin_port = htons(NETDIMM_PORT);

                inet_ntop(AF_INET, &naomi_addr.sin_addr, (char *)buf, INET6_ADDRSTRLEN);
                break;

            case 'x':
                if(progfn) {
                    fprintf(stderr, "Ignoring duplicate -x argument.\n");
                    break;
                }

                if(!(progfn = strdup(optarg))) {
                    perror("strdup");
                    goto err;
                }
                break;

            case 'a':
                keepalive = 1;
                break;

            case '?':
                fprintf(stderr, "Unrecognized option: '-%c'\n", optopt);
                goto err;

            case ':':
                fprintf(stderr, "Option -%c requires an argument\n", optopt);
                goto err;
        }
    }

    /* Make sure we got all the requisite arguments */
    if(!naomi_addr.sin_family) {
        fprintf(stderr, "You must specify the IP address of the NAOMI.\n");
        goto err;
    }
    else if(!progfn) {
        fprintf(stderr, "You must specify a binary to upload.\n");
        goto err;
    }

    c = connect_and_send(&naomi_addr, progfn, keepalive);
    free(progfn);
    return c < 0 ? EXIT_FAILURE: 0;

err:
    if(progfn)
        free(progfn);

    return EXIT_FAILURE;
}
