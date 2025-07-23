#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUFFER_SIZE 4096
#define MAX_PATH 256

void send_response(int client_fd, const char* status, const char* content_type, const char* body) {
    char header[1024];
    snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        status, content_type, strlen(body));
    
    write(client_fd, header, strlen(header));
    write(client_fd, body, strlen(body));
}

void send_file(int client_fd, const char* filepath) {
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        send_response(client_fd, "404 Not Found", "text/html", "<h1>404 Not Found</h1>");
        return;
    }
    
    struct stat st;
    fstat(fd, &st);
    
    const char* content_type = "text/html";
    if (strstr(filepath, ".zip")) content_type = "application/zip";
    else if (strstr(filepath, ".css")) content_type = "text/css";
    else if (strstr(filepath, ".js")) content_type = "application/javascript";
    
    char header[1024];
    if (strstr(filepath, ".zip")) {
        snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Disposition: attachment; filename=\"%s\"\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n\r\n",
            content_type, strrchr(filepath, '/') ? strrchr(filepath, '/') + 1 : filepath, st.st_size);
    } else {
        snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n\r\n",
            content_type, st.st_size);
    }
    
    write(client_fd, header, strlen(header));
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
        write(client_fd, buffer, bytes);
    }
    
    close(fd);
}

int create_zip(const char* project_name) {
    char zip_path[MAX_PATH];
    char src_path[MAX_PATH];
    char cmd[MAX_PATH * 2];
    
    snprintf(zip_path, sizeof(zip_path), "www/%s/%s.zip", project_name, project_name);
    snprintf(src_path, sizeof(src_path), "www/%s/src", project_name);
    
    // Check if src directory exists
    struct stat st;
    if (stat(src_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return -1;
    }
    
    // Create zip using system zip command
    snprintf(cmd, sizeof(cmd), "cd www/%s && zip -r %s.zip src/ >/dev/null 2>&1", project_name, project_name);
    
    if (system(cmd) == 0) {
        return 0;
    }
    return -1;
}

void generate_index() {
    FILE* index_file = fopen("index.html", "w");
    if (!index_file) return;
    
            fprintf(index_file,
        "<!DOCTYPE html>\n"
        "<html><head>\n"
        "<title>CYBERSPACE KIOSK</title>\n"
        "<style>\n"
        "body{font-family:Courier,monospace;margin:20px;background:#f5f5dc !important;color:#000080;}\n"
        "h1{text-align:center;color:#800080;font-size:24px;background:#e0e0e0;padding:10px;border:3px inset #c0c0c0;margin-bottom:20px}\n"
        "marquee{background:#ffff00;color:#ff0000;font-weight:bold;border:2px solid #000;margin:10px 0}\n"
        ".project{background:#e8e8e8;border:2px outset #c0c0c0;margin:15px 0;padding:15px;font-size:12px}\n"
        ".project h3{color:#000080;font-size:14px;margin:0 0 5px 0;text-decoration:underline}\n"
        ".project p{margin:3px 0;color:#000}\n"
        ".author{color:#008000;font-weight:bold}\n"
        ".download-btn{background:#c0c0c0;color:#000;border:2px outset #c0c0c0;padding:5px 15px;text-decoration:none;font-family:Courier,monospace;font-size:11px;margin-top:8px;display:inline-block}\n"
        ".download-btn:active{border:2px inset #c0c0c0}\n"
        ".footer{text-align:center;margin-top:30px;font-size:10px;color:#666}\n"
        "blink{animation:blink 1s infinite}@keyframes blink{0%,50%{opacity:1}51%,100%{opacity:0}}\n"
        "</style>\n"
        "</head><body>\n"
        "<h1>*** CYBERSPACE KIOSK v1.0 ***</h1>\n"
        "<marquee>WELCOME TO THE INFORMATION SUPERHIGHWAY! DOWNLOAD SOFTWARE HERE! FOR PENNIES!</marquee>\n");
    
    DIR* www_dir = opendir("www");
    if (www_dir) {
        struct dirent* entry;
        while ((entry = readdir(www_dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            
            char showfile_path[MAX_PATH];
            snprintf(showfile_path, sizeof(showfile_path), "www/%s/ShowFile", entry->d_name);
            
            FILE* showfile = fopen(showfile_path, "r");
            if (showfile) {
                char name[256] = "", desc[512] = "", author[256] = "", icon[256] = "";
                char line[512];
                
                while (fgets(line, sizeof(line), showfile)) {
                    line[strcspn(line, "\n")] = 0; // remove newline
                    if (strncmp(line, "name:", 5) == 0) strcpy(name, line + 5);
                    else if (strncmp(line, "description:", 12) == 0) strcpy(desc, line + 12);
                    else if (strncmp(line, "author:", 7) == 0) strcpy(author, line + 7);
                    else if (strncmp(line, "icon:", 5) == 0) strcpy(icon, line + 5);
                }
                fclose(showfile);
                
                fprintf(index_file,
                    "<div class='project'>\n"
                    "<h3>%s</h3>\n"
                    "<p><b>Description:</b> %s</p>\n"
                    "<p class='author'>Author: %s</p>\n"
                    "<a href='/download/%s' class='download-btn'>INSTALL</a>\n"
                    "</div>\n",
                    name[0] ? name : entry->d_name,
                    desc[0] ? desc : "No description available",
                    author[0] ? author : "Unknown",
                    entry->d_name);
            }
        }
        closedir(www_dir);
    }
    
    fprintf(index_file, 
        "<div class='footer'>\n"
        "<blink>*** POWERED BY KIOSK.C v1.0 ***</blink><br>\n"
        "Best viewed with Netscape Navigator 4.0 or Internet Explorer 3.0<br>\n"
        "&copy; 1999 @1casie\n"
        "</div>\n"
        "</body></html>\n");
    fclose(index_file);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int port = 8080;
    
    // Try ports 8080-8090
    for (port = 8080; port <= 8090; port++) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == 0) {
            break;
        }
        close(server_fd);
    }
    
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        return 1;
    }
    
    printf("Cyberspace Kiosk running on port %d\n", port);
    
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) continue;
        
        pid_t pid = fork();
        if (pid == 0) {
            // Child process handles the request
            close(server_fd); // Child doesn't need the server socket
            
            generate_index(); // Refresh on each request
            
            char buffer[BUFFER_SIZE] = {0};
            read(client_fd, buffer, BUFFER_SIZE);
            
            char method[16], path[256];
            sscanf(buffer, "%s %s", method, path);
            
            if (strcmp(path, "/") == 0) {
                send_file(client_fd, "index.html");
            } else if (strncmp(path, "/download/", 10) == 0) {
                char project_name[256];
                strcpy(project_name, path + 10);
                
                // Create zip file
                if (create_zip(project_name) == 0) {
                    char zip_path[MAX_PATH];
                    snprintf(zip_path, sizeof(zip_path), "www/%s/%s.zip", project_name, project_name);
                    send_file(client_fd, zip_path);
                    // Clean up zip file after sending
                    unlink(zip_path);
                } else {
                    send_response(client_fd, "404 Not Found", "text/html", "<h1>Source not found</h1>");
                }
            } else if (strncmp(path, "/www/", 5) == 0) {
                char filepath[MAX_PATH];
                snprintf(filepath, sizeof(filepath), "%s", path + 1); // remove leading /
                send_file(client_fd, filepath);
            } else {
                send_response(client_fd, "404 Not Found", "text/html", "<h1>404 Not Found</h1>");
            }
            
            close(client_fd);
            exit(0); // Child process exits after handling request
        } else if (pid > 0) {
            // Parent process
            close(client_fd); // Parent doesn't need the client socket
            // Clean up zombie processes
            waitpid(-1, NULL, WNOHANG);
        } else {
            // Fork failed
            close(client_fd);
        }
    }
    
    close(server_fd);
    return 0;
}
