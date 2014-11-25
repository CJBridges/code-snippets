#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct ipinfo {
    const char *ip;
    unsigned short port;
} ipinfo_t;

void usage() {
    printf( "echo-server [<IP> <port>]\n" );
}

static char *default_ip = "0.0.0.0";

void *start_listening( void *arg );

int launch_listener_thread( pthread_t *listener_thread,
                            const char *ip,
                            unsigned short port );

int main( int argc, char *argv[] ) {
    const char *ip = NULL;
    unsigned short port = 0;
    pthread_t listener_thread;
    int rc = 0;

    if ( argc != 1 && argc != 3 ) {
        usage( );
        return 1;
    } 

    if ( argc == 3 ) {
        ip = argv[1];
        port = atoi( argv[2] );
    } else {
        ip = default_ip;
        port = 4444;
    }

    printf( "IP: %s port:%d\n", ip, port );

    rc = launch_listener_thread( &listener_thread, ip, port );
    if ( rc != 0 ) {
        printf( "Listener thread failed to launch.  rc=%d", rc );
        return rc; 
    }

    pthread_join( listener_thread, NULL );
    printf( "Listener thread finished.  Exiting.\n" );
    return rc;
}

int launch_listener_thread( pthread_t *listener_thread,
                                   const char *ip,
                                   unsigned short port ) {
    int rc = 0;
    ipinfo_t *my_ip_info = (ipinfo_t *)malloc( sizeof( ipinfo_t ));
    if ( my_ip_info == NULL ) {
        printf( "Didn't allocate my_ip_info\n" );
        rc = 1;
        goto end;
    }

    my_ip_info->ip = strdup(ip);
    if ( my_ip_info == NULL ) {
        printf( "Failed to allocate my_ip_info->ip\n" );
        rc = 1;
        goto end;
    }
    my_ip_info->port = port;

    rc = pthread_create( listener_thread, NULL, start_listening, my_ip_info );
    if ( rc != 0 ) {
        printf( "Error: pthread_create rc=%d\n", rc );
        goto end;
    }

end:
    if ( rc != 0 ) {
        if ( my_ip_info->ip != NULL )
            free( (void *)(my_ip_info->ip ) );
        if ( my_ip_info != NULL )
            free( (void *)my_ip_info );
    }
    return rc;
}

void *start_listening( void *arg ) {
    int rc;
    ipinfo_t *my_ip_info = ( ipinfo_t * ) arg;
    printf( "Starting listener thread with ip=%s port=%d\n",
            my_ip_info->ip, my_ip_info->port );
    int sockfd;
    struct sockaddr_in socketaddr;
    char buffer[1024];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if ( rc < 0 ) {
        printf( "Failed to allocate a socket.  rc = %d", rc );
        goto end;
    }

    bzero( &socketaddr, sizeof (struct sockaddr_in) );
    socketaddr.sin_family = AF_INET;
    socketaddr.sin_port = htons(my_ip_info->port);
    socketaddr.sin_addr.s_addr = htonl( INADDR_ANY );

    rc = bind( sockfd, (struct sockaddr *)(&socketaddr), sizeof( struct sockaddr_in ) );
    if ( rc != 0 ) {
        printf( "Failed to bind.  rc = %d", rc );
        goto end;
    }

    rc = listen( sockfd, 30 );
    if ( rc != 0 ) {
        printf("Error setting up listening on sockfd.  rc = %d\n", rc);
        goto end;
    }

    while ( 1 ) {
        int clientfd;
        struct sockaddr_in client_addr;
        unsigned int addrlen=sizeof(client_addr);

        do {
            clientfd = accept( sockfd, (struct sockaddr *)(&client_addr), &addrlen );
        } while ( clientfd < 0 && errno == EINTR );
        if ( clientfd < 0 ) {
            rc = errno;
            printf( "Failed to accept.  errno=%d", rc );
            goto end;
        }

        rc = recv( clientfd, buffer, 1024, 0 );
        if ( rc < 0 ) {
            rc = errno;
            printf( "Failed to receive.  errno=%d", rc );
            goto end;
        }

        rc = send( clientfd, buffer, rc /* len of data received */, 0 );
        if ( rc < 0 ) {
            rc = errno;
            printf( "Failed to send.  errno=%d", rc );
            goto end;
        }

        do {
            rc = close( clientfd );
        } while ( rc != 0 && errno == EINTR );
        if ( rc < 0 ) {
            rc = errno;
            printf( "Failed to close connection.  errno=%d", rc );
            goto end;
        }
    }
end:
    // Clean up socket in sockfd
    // Clean up socket in clientfd 
    return NULL;
}
