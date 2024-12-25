# 選擇編譯器
CC = gcc

# CFLAGS：設置編譯選項
# -Wall 開啟所有警告。
# -Wextra 額外警告。
# -pthread
CFLAGS = -Wall -Wextra -pthread

FILES_DIR = files

# 目標檔案名稱
SERVER = server
CLIENT = client

# 來源檔案
SERVER_SRC = server.c
CLIENT_SRC = client.c

# 預設目標（執行 make 時執行）
all: $(SERVER) $(CLIENT)

# 編譯伺服器
$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC)
$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)

# 清除生成的檔案
# 檢查有無生成新檔案再刪除
clean:
	rm -f $(SERVER) $(CLIENT) 
	[ -d $(FILES_DIR) ] && rm -rf $(FILES_DIR) 
