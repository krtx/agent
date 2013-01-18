#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <unistd.h>
#include "QuadProg++.hh"
#include "svm.h"


#define BUF_LEN 256

class Client {
public:
  Client(std::string log_file, std::string host, unsigned short port = 5000)
    :log_file(log_file), host(host), port(port) {
    start_connection();
    load_data();
    run();
  }

  ~Client() { close(s); }

  void start_connection () {
    try {
      servhost = gethostbyname(host.c_str());
      if (servhost == NULL){
        fprintf(stderr, "[%s] から IP アドレスへの変換に失敗しました。\n", host.c_str());
        throw;
      }

      bzero(&server, sizeof(server));

      server.sin_family = AF_INET;
      bcopy(servhost->h_addr, &server.sin_addr, servhost->h_length);
      server.sin_port = htons(port);

      if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "ソケットの生成に失敗しました。\n");
        throw;
      }
      if (connect(s, (struct sockaddr *)&server, sizeof(server)) == -1){
        fprintf(stderr, "connect に失敗しました。\n");
        throw;
      }
      
      // "PLEASE INPUT YOUR NAME"
      if ((read_size = read(s, buf, BUF_LEN)) > 0)
        write(1, buf, read_size);

      // input name
      fgets(buf, BUF_LEN, stdin);
      write(s, buf, strlen(buf));

      // IDとクライアント名の対応
      // "a1:name1 a2:name2 ..."
      if ((read_size = read(s, buf, BUF_LEN)) > 0)
        write(1, buf, read_size);
      buf[read_size] = '\0';
      std::stringstream ss(buf);
      std::string tmp;
      while (ss >> tmp)
        cnames.push_back(tmp.substr(tmp.find(':') + 1));

      // "Your ID: x", x はクライアントID
      if ((read_size = read(s, buf, BUF_LEN)) > 0)
        write(1, buf, read_size);
      buf[read_size] = '\0';
      sscanf(buf, "%*[^0-9]%d", &cid);

      // "n,m", n は商品数 m はクライアント数
      if ((read_size = read(s, buf, BUF_LEN)) > 0)
        write(1, buf, read_size);
      buf[read_size] = '\0';
      sscanf(buf, "%zu%*[^0-9]%zu", &n, &m);

    } catch (const std::exception &ex) {
      std::cerr << "error occured" << std::endl;
    }
  }

  void load_data() {
    std::ifstream ifs(log_file.c_str());
    char buf[BUF_LEN];
    Matrix<double> x;
    std::vector<Vector<double> > y;
    while (ifs.getline(buf, BUF_LEN)) {
    }
  }

  void run() {
    while (1) {
      puts("-------------------------------------");
      std::string dec = decide() + "\n";
      write(s, dec.c_str(), n + 1);

      // current prices and tenders
      // "g1:1 g2:1 g3:1 a1:100 a2:010 a3:110 a4:100"
      if ((read_size = read(s, buf, BUF_LEN)) > 0)
        write(1, buf, read_size);
      buf[read_size] = '\0';
      std::vector<int> ps;
      std::stringstream ss(buf);
      for (size_t i = 0; i < n; i++) {
        std::string tmp; ss >> tmp;
        ps.push_back(atoi(tmp.substr(tmp.find(':') + 1).c_str()));
      }
      prices.push_back(ps);
      std::vector<std::string> ts;
      for (size_t i = 0; i < m; i++) {
        std::string tmp; ss >> tmp;
        ts.push_back(tmp.substr(tmp.find(':') + 1));
      }
      tenders.push_back(ts);
      
      // consequent prices (ignored)
      if ((read_size = read(s, buf, BUF_LEN)) > 0)
        write(1, buf, read_size);
      buf[read_size] = '\0';
      if (std::string(buf).find("end") != std::string::npos) break;
    }

    // "winner g1:2 g2:1 "
    read_size = read(s, buf, BUF_LEN);
    buf[read_size] = '\0';
    bool win[n]; memset(win, 0, sizeof(win));
    std::stringstream ss(buf);
    std::string tmp; ss >> tmp;
    for (size_t i = 0; i < n; i++) {
      ss >> tmp;
      if (atoi(tmp.substr(tmp.find(':') + 1).c_str()) == cid)
        win[i] = true;
    }
    // "price g1:3 g2:2"
    read_size = read(s, buf, BUF_LEN);
    buf[read_size] = '\0';
    int total_cost = 0;
    ss.str(""); ss.clear(); ss.str(buf); ss >> tmp;
    for (size_t i = 0; i < n; i++) {
      ss >> tmp;
      if (win[i])
        total_cost += atoi(tmp.substr(tmp.find(':') + 1).c_str());
    }
    printf("TOTAL COST = %d\n", total_cost);

  }

  void dump() {
    std::ofstream ofs(log_file.c_str(), std::ios::out | std::ios::app);
    for (size_t i = 0; i < prices.size(); i++) {
      for (size_t j = 0; j < n; j++) ofs << prices[i][j] << " ";
      for (size_t j = 0; j < m; j++) ofs << tenders[i][j] << " ";
      ofs << std::endl;
    }
  }
private:
  std::string log_file;

  // variables for socket
  int s;
  struct hostent *servhost;
  struct sockaddr_in server;
  char buf[BUF_LEN];
  int read_size;
  std::string host;
  unsigned short port;

  int cid; // client id
  size_t n, m; // num of items, num of clients
  std::vector<std::string> cnames; // client names
  std::vector<std::vector<int> > prices;
  std::vector<std::vector<std::string> > tenders;
  std::vector<std::vector<SVM> > svms;
  
  std::string decide() {
    // tender to item 1
    std::string ret = "1" + std::string(n - 1, '0');
    return ret;
  }
};  

int main(int argc, char *argv[])
{
  char *host;
  unsigned short port = 5000;

  if (argc < 3) {
    std::cout << "usage: client [hostname] [port]\n";
    exit(1);
  }

  host = (char *)argv[1];
  port = atoi(argv[2]);

  Client cl("log_file", host, port);
  //cl.run();

  return 0;
}

