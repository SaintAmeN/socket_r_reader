#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <signal.h>


#define PORT 12300
#define HOST "10.1.1.20"

volatile sig_atomic_t stop;

void inthand(int signum) {
    stop = 1;
}

static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";


static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

/**
 * Base 64 decoder.
 * @param encoded_string - message to decode
 * @return decoded message.
 */
std::string base64_decode(std::string const &encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_];
        in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    return ret;
}

/**
 * Thread method. Called to write data in different thread (not to block socket by itself).
 * @param data - data with socket address.
 * @return nothing.
 */
void *get_message(void *data) {
    int *sock = (int*) data;
    //frame sent to server
    char register_frame[] = {102, 72, 115, 105, 89, 50, 57, 116, 98, 87, 70, 117, 90, 67, 73, 54, 73, 109, 70, 107, 90,
                             70, 57, 106, 98, 71, 108, 108, 98, 110, 81, 105, 76, 67, 74, 107, 90, 88, 82, 108, 89, 51,
                             82, 112, 98, 50, 53, 122, 73, 106, 112, 98, 88, 88, 48, 61, 51, 19, 8, 83, 99, 13};
    char hello[] = {102, 72, 115, 105, 89, 50, 57, 116, 98, 87, 70, 117, 90, 67, 73, 54, 73, 110, 66, 118, 98, 109, 99,
                    105, 76, 67, 74, 107, 90, 88, 82, 108, 89, 51, 82, 112, 98, 50, 53, 122, 73, 106, 112, 98, 88, 88,
                    48, 61, 51, 19, 8, 83, 99, 13};

    send(*sock, register_frame, sizeof(register_frame), 0);
    while (1) {
        send(*sock, hello, sizeof(hello), 0);
        sleep(3);
    }
}


int main(int argc, char const *argv[]) {

    // Create buffer to which data will be passed after read.
    char buffer[50000] = {0};

    // create socket
    int sock = 0, valread;
    // create socket describing structs
    struct sockaddr_in serv_addr;

    // init heartbeat thread
    pthread_t heartbeatThread;

    // connect
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, HOST, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return 0;
    }

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return 0;
    }

    // create heartbeat thread. pass connected socket
    pthread_create(&heartbeatThread, NULL, &get_message, (void*) &sock);

    stop = 0;
    while (!stop) {
        // read message.
        valread = read(sock, buffer, 1024);
        if(valread == 0){
            std::cout<< "Connection broke.";
            break;
        }
        std::string decodedMsg = base64_decode(buffer);
        std::cout << decodedMsg << std::endl;

    }
    printf("exiting\n");
    return 0;
}
