#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 8080   // 必須與伺服器的端口一致
#define BUFFER_SIZE 1024

void handle_server_response(int client_fd) {
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read <= 0) {
            printf("Server disconnected.\n");
            return;
        }

        printf("%s", buffer); // 打印伺服器回應

        // 假設伺服器用 "\nEND\n" 表示回應結束
        if (strstr(buffer, "\nEND\n")) {
            break;
        }
    }
}

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE] = {0};
    char username[50], group[50];

     // 取得用戶名稱跟所屬群組
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0; // 移除换行符

    printf("Enter your group: ");
    fgets(group, sizeof(group), stdin);
    group[strcspn(group, "\n")] = 0; // 移除换行符

    // 1. 創建 socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. 設定伺服器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // 3. 轉換伺服器的 IP 地址（127.0.0.1 表示本機）
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // 4. 與伺服器建立連線
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to server.\n");

    // 5. 發送名稱跟所屬群組
    snprintf(buffer, BUFFER_SIZE, "USER %s GROUP %s\n", username, group);
    write(client_fd, buffer, strlen(buffer));

    // 接收確認
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read > 0) {
        printf("Server: %s\n", buffer);
    }

    // 5. 與伺服器通訊
    while (1) {
        // 從用戶端輸入要執行的命令
        printf("Enter command: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0; // 移除換行符

        // 檢查退出指令
        if (strcmp(buffer, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }

        // 檢查命令格式
        if (strncmp(buffer, "create", 6) == 0) {
            char filename[256], permissions[6];
            if (sscanf(buffer, "create %255s %9s", filename, permissions) != 2) {
                printf("Invalid create command format. Use: create <filename> <permissions>\n");
                continue;
            }

        } else if (strncmp(buffer, "write", 5) == 0) {
            char filename[256], mode;
            char content[BUFFER_SIZE];
            write(client_fd, buffer, strlen(buffer));
            if (sscanf(buffer, "write %255s %c", filename, &mode) != 2) {
                printf("Invalid write command format. Use: write <filename> <o/a>\n");
                handle_server_response(client_fd);
                break;
            }else{
                printf("Enter content to write to the file (end with !q):\n");
                while (1) {
                    // 讀取用戶輸入
                    fgets(buffer, sizeof(buffer), stdin);

                    // 如果輸入是終止符，結束寫入
                    if (strncmp(buffer, "!q", 2) == 0) {
                    break;
                    }

                // 發送輸入的內容到服務器
                write(client_fd, buffer, strlen(buffer));
                }
            }
        } else if (strncmp(buffer, "mode", 4) == 0) {
            char filename[256], permissions[10];
            if (sscanf(buffer, "mode %255s %6s", filename, permissions) != 2) {
                printf("Invalid mode command format. Use: mode <filename> <permissions>\n");
                continue;
            }
        } else if (strncmp(buffer, "read", 4) == 0) {
            char filename[256];
            if (sscanf(buffer, "read %255s", filename) != 1) {
                printf("Invalid read command format. Use: read <filename>\n");
                continue;
            }
        }

        // 發送命令到伺服器
        write(client_fd, buffer, strlen(buffer));

        // 處理伺服器回應
        handle_server_response(client_fd);
    }

    // 6. 關閉連線
    close(client_fd);

    return 0;
}
