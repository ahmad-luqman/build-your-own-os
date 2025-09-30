/*
 * Simple HTTP Server Example for MiniOS
 * Demonstrates basic networking and web server capabilities
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define HTTP_PORT 8080
#define MAX_CONNECTIONS 10
#define BUFFER_SIZE 4096

// HTTP response templates
static const char *http_200_header = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "\r\n";

static const char *http_404_response =
    "HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "\r\n"
    "<html><body><h1>404 Not Found</h1></body></html>\r\n";

static const char *welcome_page =
    "<html>\n"
    "<head><title>MiniOS Web Server</title></head>\n"
    "<body>\n"
    "<h1>Welcome to MiniOS!</h1>\n"
    "<p>This is a simple web server running on MiniOS.</p>\n"
    "<ul>\n"
    "<li><a href=\"/status\">System Status</a></li>\n"
    "<li><a href=\"/processes\">Running Processes</a></li>\n"
    "<li><a href=\"/memory\">Memory Information</a></li>\n"
    "</ul>\n"
    "</body>\n"
    "</html>\n";

// Parse HTTP request
struct http_request {
    char method[16];
    char path[256];
    char version[16];
};

int parse_http_request(const char *request, struct http_request *req) {
    // Simple parsing: "GET /path HTTP/1.1"
    if (sscanf(request, "%15s %255s %15s", req->method, req->path, req->version) != 3) {
        return -1;
    }
    return 0;
}

// Generate system status page
char *generate_status_page(void) {
    static char page[2048];
    
    snprintf(page, sizeof(page),
        "<html>\n"
        "<head><title>MiniOS System Status</title></head>\n"
        "<body>\n"
        "<h1>System Status</h1>\n"
        "<p><strong>Operating System:</strong> MiniOS v1.0.0</p>\n"
        "<p><strong>Architecture:</strong> %s</p>\n"
        "<p><strong>Uptime:</strong> %d seconds</p>\n"
        "<p><strong>Free Memory:</strong> %d KB</p>\n"
        "<p><a href=\"/\">Back to Home</a></p>\n"
        "</body>\n"
        "</html>\n",
#ifdef __aarch64__
        "ARM64",
#else
        "x86-64",
#endif
        (int)(timer_get_ticks() / 1000),  // Uptime in seconds
        (int)(get_free_memory() / 1024)   // Free memory in KB
    );
    
    return page;
}

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    printf("MiniOS HTTP Server starting on port %d...\n", HTTP_PORT);
    
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket creation failed");
        return 1;
    }
    
    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(HTTP_PORT);
    
    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_socket);
        return 1;
    }
    
    // Listen for connections
    if (listen(server_socket, MAX_CONNECTIONS) < 0) {
        perror("listen failed");
        close(server_socket);
        return 1;
    }
    
    printf("Server listening on http://0.0.0.0:%d\n", HTTP_PORT);
    printf("Press Ctrl+C to stop the server.\n\n");
    
    // Main server loop
    while (1) {
        // Accept connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            continue;
        }
        
        printf("Connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        // Handle request (simplified)
        char buffer[BUFFER_SIZE];
        recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        // Send response  
        send(client_socket, http_200_header, strlen(http_200_header), 0);
        send(client_socket, welcome_page, strlen(welcome_page), 0);
        
        close(client_socket);
    }
    
    close(server_socket);
    return 0;
}