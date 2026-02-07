#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include "iostream"
#include <stdlib.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include "wrap.h"
using namespace std;

#define SERV_PORT 8080
// client
// 	socket()	# 创建socket
// 	connect()	# 与服务器建立连接
// 	write()	# 写数据到socket
// 	read() 	# 读取数据
// 	显示读取结果
// 	close() # 关闭连接

bool is_valid_line(const string& line){
    size_t start = line.find_first_not_of(" \t\n\r");
    if(start == string::npos){
        return false;
    }
    if(line.find("//", start) == start){
        return false;
    }
    return true;
}

int get_content(char* buf, int idx){
    ifstream file("./wrap.c");

    if(!file.is_open()){
        cerr << "Failed to open file"<<endl;
        return -1;
    }
    string line;
    int valid_line_count = 0;
    while(getline(file, line)){
        if(is_valid_line(line)){
            if(valid_line_count == idx){
                strncpy(buf, line.c_str(), BUFSIZ - 1); // strncpy会自动在后面加上\0
                buf[BUFSIZ - 1] = '\0';
                file.close();
                return line.size();
            }
            valid_line_count ++;
        }
    }   
    file.close();
    return 0;
}

int main(int argc, char const *argv[]){
    int client_fd = -1;
    struct sockaddr_in serv_addr;
    char buf[BUFSIZ];       //write buffer 
    char recv_buf[BUFSIZ];  //read buffer

    // create sockte 
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(client_fd == -1)
        sys_err("create socket failue!");
    
    // connect server
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &serv_addr.sin_addr.s_addr);
    int conn_res = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(conn_res == -1) 
        sys_err("connect server failure!");

    // send data!
    for(int i = 0; i < 30; i++){
        int content_len = get_content(buf, i);
        if(content_len <= 0) break;
        // else cout<< buf << endl;
        write(client_fd, buf,strlen(buf));
        int read_len = read(client_fd, recv_buf, sizeof(recv_buf) - 1);
        if(read_len <= 0) {
            if(read_len == 0) {
                cout << "Server closed connection" << endl;
            } else {
                perror("read error");
            }
            break;
        }
        recv_buf[read_len] = '\0';
        cout << recv_buf << '\n'; // 必须要刷新缓冲区 才能在屏幕当中显示数据    
    }
    close(client_fd);
}