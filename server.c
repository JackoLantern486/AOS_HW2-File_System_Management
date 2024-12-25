#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_FILES 100
#define MAX_CLIENTS 50

// 定義 Capability 結構
struct Capability {
    char filename[256];
    int client_id;  // 客戶端 ID
    char group[100];
    int owner_permission;
    int group_permission;
    int other_permission;
    int file_id;

};

// 全局 Capability List
struct Capability capability_list[MAX_FILES];
int capability_count = 0;
// 互斥鎖
pthread_rwlock_t file_rwlock[MAX_FILES];  // 讀寫鎖

// 是否有權限
int has_permission(int client_id, char *group, const char *filename, int read, int write) {

    for (int i = 0; i < capability_count; i++) {
        if (strcmp(capability_list[i].filename, filename) == 0) {
            struct Capability *cap = &capability_list[i];

            if (read == 1){
                if (cap ->client_id == client_id) { //client 為 owner
                    if (cap -> owner_permission == 1 || cap -> owner_permission == 3) {
                        printf("1.1");
                        return 1; // 有權限
                    }else{
                        printf("1.0");
                        return 0; // 沒有權限
                    }
                }else if(strcmp(cap->group, group) == 0){ //client 與 owner 同 group
                    if (cap -> group_permission == 1 || cap -> group_permission == 3) {
                        return 1; // 有權限
                    }else{
                        return 0; // 沒有權限
                    }
                }else{ // client 為 others
                    if (cap -> other_permission == 1 || cap -> other_permission == 3) {
                        return 1; // 有權限
                    }else{
                        return 0; // 沒有權限
                    }
                }
            // 檢查write權限
            }else if(write == 1){
                if (cap ->client_id == client_id) { //client 為 owner
                    if (cap -> owner_permission == 2 || cap -> owner_permission == 3) {
                        return 1; // 有權限
                    }else{
                        return 0; // 沒有權限
                    }
                }else if(strcmp(cap->group, group) == 0){ //client 與 owner 同 group
                    if (cap -> group_permission == 2 || cap -> group_permission == 3) {
                        return 1; // 有權限
                    }else{
                        return 0; // 沒有權限
                    }
                }else{ // client 為 others
                    if (cap -> other_permission == 2 || cap -> other_permission == 3) {
                        return 1; // 有權限
                    }else{
                        return 0; // 沒有權限
                    }
                }
            }
        }
    }
    return -1;  // 檔案不存在
}


void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    free(arg);

    char buffer[BUFFER_SIZE];
    int client_id = client_fd; // 使用 client_fd 作為 client_id
    char username[50] = {0};
    char usergroup[50] = {0};

    // 接收用戶名和群組資訊
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1); // 接收用戶名和群組
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        if (sscanf(buffer, "USER %49s GROUP %49s", username, usergroup) == 2) {
            printf("Client connected: Username = %s, Group = %s\n", username, usergroup);
            write(client_fd, "Welcome! User and group registered.\n", 37);
        } else {
            write(client_fd, "Invalid user/group format.\nEND\n", 32);
            close(client_fd);
            return NULL;
        }
    } else {
        printf("Failed to receive user/group info.\n");
        close(client_fd);
        return NULL;
    }


    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1); // 從客戶端接收數據

        if (bytes_read <= 0) {
            printf("Client disconnected: FD = %d\n", client_fd);
            break; // 客戶端斷開連接或讀取錯誤
        }

        buffer[bytes_read] = '\0'; // 確保字符串結尾
        printf("Received command: %s\n", buffer);

        char filename[256], permissions[6];
        char mode;

        
        if (sscanf(buffer, "create %255s %6s", filename, permissions) == 2) { // create
 
             if (access(filename, F_OK) == 0) {     //文件已存在
                write(client_fd, "Error: File already exists\n", 28);

            }else{
                strcpy(capability_list[capability_count].filename, filename);
                capability_list[capability_count].client_id = client_id;
                strcpy(capability_list[capability_count].group,  usergroup);
                capability_list[capability_count].file_id = capability_count;

                // permission : 0 :none, 1 :read only, 2 :write only, 3 : both
                capability_list[capability_count].owner_permission =
                    ((permissions[0] == 'r') ? 1 : 0) + ((permissions[1] == 'w') ? 2 : 0);
                capability_list[capability_count].group_permission =
                    ((permissions[2] == 'r') ? 1 : 0) + ((permissions[3] == 'w') ? 2 : 0);
                capability_list[capability_count].other_permission =
                    ((permissions[4] == 'r') ? 1 : 0) + ((permissions[5] == 'w') ? 2 : 0);

                int fd = open(filename, O_CREAT | O_WRONLY, 0666); 
                if (fd == -1) {
                    write(client_fd, "Error: Unable to create file\nEND\n", 34);
                }else{
                    close(fd); // 創建成功後立即關閉文件描述符
                    capability_list->file_id = capability_count;
                    capability_count++;
                    write(client_fd, "File created successfully\nEND\n", 31);
                }
            }   
        } else if (sscanf(buffer, "read %255s", filename) == 1) { //read 
            int permission = has_permission(client_id, usergroup, filename, 1, 0);
            struct Capability *cap;

            for (int i = 0; i < capability_count; i++) {
                if (strcmp(capability_list[i].filename, filename) == 0) {
                    cap = &capability_list[i];
                    cap->file_id = i;
                    break;
                }
            }
                
            if (permission == -1) {
                // 文件不存在
                write(client_fd, "Error: File does not exist\nEND\n", 32);
            }else if (permission == 0) {
                // 沒有讀取權限
                write(client_fd, "Permission denied\nEND\n", 23);
            } else {
                // 有讀取權限，執行讀取文件操作
                pthread_rwlock_rdlock(&file_rwlock[cap->file_id % MAX_FILES]);

                FILE *file = fopen(filename, "r");
                if (file) {
                    while (fgets(buffer, sizeof(buffer), file)) {
                        write(client_fd, buffer, strlen(buffer));
                        printf("%s",buffer);
                    }
                    fclose(file);
                    write(client_fd, "\nEND\n", 6); // 標記讀取結束
                } else {
                    write(client_fd, "Error: Unable to open file\nEND\n", 32);
                }
                    pthread_rwlock_unlock(&file_rwlock[cap->file_id % MAX_FILES]);
            }
        } else if (sscanf(buffer, "write %255s %c", filename, &mode) == 2) { //write
            int permission = has_permission(client_id, usergroup, filename, 0, 1);
            struct Capability *cap;

            for (int i = 0; i < capability_count; i++) {
                if (strcmp(capability_list[i].filename, filename) == 0) {
                    cap = &capability_list[i];
                    cap->file_id = i;
                    break;
                }
            }

            if (permission == -1) {
                // 文件不存在
                write(client_fd, "Error: File does not exist\nEND\n", 32);
            }else if (permission == 0) {
                // 沒有讀取權限
                write(client_fd, "Permission denied\nEND\n", 23);
            } else {
                write(client_fd, "Please wait...\nEND\n", 20);
                pthread_rwlock_wrlock(&file_rwlock[cap->file_id % MAX_FILES]);


                const char *open_mode = (mode == 'o') ? "w" : "a";
                FILE *file = fopen(filename, open_mode);

                if (file) {
                    while (1) {
                        memset(buffer, 0, sizeof(buffer));
                        ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);

                        if (n <= 0 || strstr(buffer, "!q")) {
                            break; // 結束寫入
                        }

                        fputs(buffer, file);
                    }
                    fclose(file);
                    // write(client_fd, "Write successful\nEND\n", 22);
                } else {
                    write(client_fd, "Error: Unable to open file\nEND\n", 32);
                }
                pthread_rwlock_unlock(&file_rwlock[cap->file_id % MAX_FILES]);
            }
        } else if (sscanf(buffer, "mode %255s %6s", filename, permissions) == 2) { //mode
            // 修改文件權限
            struct Capability *cap = NULL;
                for (int i = 0; i < capability_count; i++) { //尋找檔案
                    if (strcmp(capability_list[i].filename, filename) == 0) {
                        cap = &capability_list[i];
                        break;
                    }
                }
            if(!cap){
                write(client_fd, "Error: File does not exist\nEND\n", 32);
            }else if (client_id != cap->client_id){
                // 沒有改mode權限
                write(client_fd, "Permission denied\nEND\n", 23);
            }else{
                cap->owner_permission = ((permissions[0] == 'r') ? 1 : 0) + ((permissions[1] == 'w') ? 2 : 0);
                cap->group_permission = ((permissions[2] == 'r') ? 1 : 0) + ((permissions[3] == 'w') ? 2 : 0);
                cap->other_permission = ((permissions[4] == 'r') ? 1 : 0) + ((permissions[5] == 'w') ? 2 : 0);
                write(client_fd, "Permissions updated\nEND\n", 25);
            }

        }else {
            write(client_fd, "Unknown command\nEND\n", 21);
            continue;
        }
    }
    close(client_fd);
    return NULL;
}


int main() {
    int server_fd;
    struct sockaddr_in server_addr;
    // 初始化互斥鎖
    for (int i = 0; i < MAX_FILES; i++) {
    pthread_rwlock_init(&file_rwlock[i], NULL);
    }

    // 建立 TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET; //IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // 讓任何IP連線
    server_addr.sin_port = htons(PORT); // 設定伺服器監聽的端口

    // 綁定 socket 與伺服器的地址
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // 監聽client連線
    if (listen(server_fd, 5) == -1) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        int *client_fd = malloc(sizeof(int)); // 為每個客戶端連線分配記憶體
        struct sockaddr_in client_addr; // 用來存放客戶端的地址資訊
        socklen_t addr_len = sizeof(client_addr); // 客戶端地址長度

        // 接受客戶端的連線請求
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (*client_fd == -1) {
            perror("Accept failed");
            free(client_fd);
            continue; // 繼續接受下一個連線
        }

        // 取得客戶端的 IP 和端口並顯示
        char *client_ip = inet_ntoa(client_addr.sin_addr); // 轉換 IP 為可讀格式
        int client_port = ntohs(client_addr.sin_port); // 轉換端口為主機字節順序
        printf("Client connected: IP = %s, Port = %d\n", client_ip, client_port);

        // 創建新的執行緒來處理該客戶端的請求
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, client_fd);
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}
