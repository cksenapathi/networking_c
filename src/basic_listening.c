#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

int main(){
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, "3490", &hints, &servinfo)) != 0){
         fprintf(stderr, "gai error: %s\n", gai_strerror(status));
         return -1;
    }

    fprintf(stdout, "Note sure what number to expect %f\n\n", (float) sizeof(hints)/(float)sizeof(servinfo));

    struct addrinfo *curr_addrinfo = servinfo;

    while(curr_addrinfo != NULL){
        fprintf(stdout, "ai_canonname: %s\n", curr_addrinfo->ai_canonname);
        curr_addrinfo = curr_addrinfo->ai_next;
    }

    freeaddrinfo(servinfo);

    return 0;
}
