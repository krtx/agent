#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <vector>
#include <bitset>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
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
#define ITEM_MAX 1000

class Client {
 public:
 Client(std::string log_file, std::string host, unsigned short port = 5000)
   :log_file(log_file), host(host), port(port) {
    // ログファイルがあるかチェック
    std::ifstream ifs(log_file.c_str());
    first_day = !ifs ? true : false;
    std::stringstream tmp; tmp << "minoru" << (rand() % 200); cname = tmp.str();
    std::cout << "client name is " << cname << std::endl;
  }

  ~Client() { close(s); }

  void start (std::string ev_file) {
    start_connection();
    if (!first_day) load_data();
    load_eval(ev_file.c_str());
    tendering.set();
    run();
    dump();
  }

 private:
  std::string log_file;
  bool first_day;

  // ソケット用の変数
  int s;
  struct hostent *servhost;
  struct sockaddr_in server;
  char buf[BUF_LEN];
  int read_size;
  std::string host;
  unsigned short port;

  int cid; // client id
  size_t n, m; // 商品数、クライアント数
  std::string cname; // 自分のクライアント名
  std::vector<std::string> cnames; // クライアント名
  std::vector<std::vector<int> > prices;
  std::vector<std::vector<std::string> > tenders;
  std::vector<std::vector<SVM> > svms;

  std::vector<int> evals;
  std::bitset<ITEM_MAX> tendering;

  void load_eval(std::string filename) {
    evals.resize((int)pow(2, n) - 1);
    std::ifstream ifs(filename.c_str());
    std::string tmp;
    while (ifs >> tmp) {
      int val = atoi(tmp.substr(tmp.find(':') + 1).c_str());
      tmp = tmp.substr(0, tmp.find(':'));
      int idx = 0;
      for (size_t i = 0, j = 1; i < tmp.length(); i++, j *= 2)
        if (tmp[i] == '1') idx += j;
      evals[idx - 1] = val;
    }

    for (size_t i = 0; i < evals.size(); i++)
      printf("%d%c", evals[i], i == evals.size() - 1 ? '\n' : ' ');
  }

  /*
  std::string decide() {
    std::bitset<ITEM_MAX> si(0);
    for (size_t i = 0; i < n; i++) si[i] = rand() % 2;
    tendering &= si;
    std::string ret = tendering.to_string();
    reverse(ret.begin(), ret.end());
    return ret.substr(0, n);
  }
  */

  std::string decide() {
    std::vector<int> cur_price = prices.back();
    int max_benefit = -1;
    std::bitset<ITEM_MAX> si(0);
    std::vector<std::bitset<ITEM_MAX> > mxss;
    do {
      // どの商品に入札するかを決めて、コストを予測する
      si = std::bitset<ITEM_MAX>(si.to_ulong() + 1);
      if ((si & tendering) != si) continue;

      int ben, ev = evals[si.to_ulong() - 1], cost = 0;

      if (first_day) {
        for (size_t i = 0; i < n; i++) if (si[i]) cost += cur_price[i];
      } else {
        Vector<double> prc(n); for (size_t i = 0; i < n; i++) prc[i] = cur_price[i];
        std::vector<std::bitset<ITEM_MAX> > tends(m, 0);
        if (!tenders.empty()) {
          for (size_t i = 0; i < m; i++)
            for (size_t j = 0; j < n; j++)
              tends[i][j] = tenders.back()[i][j] == '1' ? 1 : 0;
        } else {
          for (size_t i = 0; i < n; i++) tends[i].set();
          tends[cid - 1] = si;
        }
        // 入札のシミュレーション
        while (true) {
          std::bitset<ITEM_MAX> comp(0);
          std::bitset<ITEM_MAX> st = si;
          cost = 0;
          printf("prices: [");
          for (size_t i = 0; i < n; i++) printf("%d%s", (int)prc[i], i == n - 1 ? "] -> " : " ");
          for (size_t i = 0; i < m; i++) {
            if ((int)i == cid - 1) continue;
            printf("%zu:", i);
            for (size_t j = 0; j < n; j++) {
              if (svms[i][j].discriminant(prc) > 0.0 && tends[i][j]) {
                printf("1");
                if (st[j]) comp[j] = 1; else st[j] = 1;
              }
              else {
                tends[i][j] = 0;
                printf("0");
              }
            }
            printf(" ");
          }
          puts("");
          if (comp.to_ulong() == 0 || (si & comp).to_ulong() == 0) {
            for (size_t i = 0; i < n; i++) if (si[i]) cost += (int)prc[i];
            break;
          }
          else {
            for (size_t i = 0; i < n; i++) {
              if (comp[i]) prc[i] += 1.0;
              if (si[i]) cost += (int)prc[i];
            }
            if (cost > ev) break;
          }
        }
      }
      ben = ev - cost;
      
      std::string tmp = si.to_string(); reverse(tmp.begin(), tmp.end()); tmp = tmp.substr(0, n);
      printf("%s ## ev: %d, cost: %d, benefit: %d\n", tmp.c_str(), ev, cost, ben);

      if (ben > max_benefit) { max_benefit = ben; mxss.clear(); mxss.push_back(si); }
      else if (ben == max_benefit) { max_benefit = ben; mxss.push_back(si); }
    } while (si.count() < n);
    
    // どうやっても利益が出ないとき
    if (max_benefit < 0) {
      tendering.reset();
      return std::string(n, '0');
    }
    
    tendering = mxss[rand() % mxss.size()];
    std::string ret = tendering.to_string();
    reverse(ret.begin(), ret.end());
    return ret.substr(0, n);
  }

  void start_connection () {
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
    /*
    fgets(buf, BUF_LEN, stdin);
    write(s, buf, strlen(buf));
    */
    write(s, cname.c_str(), cname.length());
    write(s, "\n", 1);

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
  }

  // 蓄積された入札データを読み込み、SVMを作成する
  void load_data() {
    std::ifstream ifs(log_file.c_str());
    std::string buf;
    size_t n, m; ifs >> n >> m;
    std::vector<std::vector<double> > _x;
    std::vector<std::vector<double> > _y(n*m);
    while (ifs) {
      std::vector<double> tmp;
      for (size_t i = 0; i < n; i++) {
        double t; ifs >> t; tmp.push_back(t);
      }
      _x.push_back(tmp); tmp.clear();
      for (size_t i = 0; i < m; i++) {
        std::string s; ifs >> s;
        for (size_t j = 0; j < n; j++) {
          if (s[j] == '1') _y[i * n + j].push_back(1);
          else _y[i * n + j].push_back(0);
        }
      }
    }

    Matrix<double> x(_x.size() - 1, _x[0].size());
    for (size_t i = 0; i < _x.size() - 1; i++)
      for (size_t j = 0; j < _x[0].size(); j++) x[i][j] = _x[i][j];
    std::vector<std::vector<Vector<double> > > y(m);
    for (size_t i = 0; i < m; i++) {
      y[i] = std::vector<Vector<double> >(n);
      for (size_t j = 0; j < n; j++) {
        y[i][j] = Vector<double>(_y[0].size() - 1);
        for (size_t k = 0; k < _y[0].size() - 1; k++)
          y[i][j][k] = _y[i * n + j][k];
      }
    }

    puts("creating SVMs...");
    Kernel *k = new Gaussian(20.0);
    svms = std::vector<std::vector<SVM> >(m);
    for (size_t i = 0; i < m; i++) 
      for (size_t j = 0; j < n; j++) {
        printf("svm %zu,%zu\n", i, j);
        svms[i].push_back(SVM(x, y[i][j], k, 10));
      }
  }

  void run() {
    prices.push_back(std::vector<int>(n, 0));
    while (1) {
      printf("------------------- %zu ----------------------\n", prices.size());
      std::string dec = decide();
      printf("DECISION: %s\n", dec.c_str());
      write(s, (dec + "\n").c_str(), n + 1);

      // current prices and tenders (current prices are ignored)
      // "g1:1 g2:1 g3:1 a1:100 a2:010 a3:110 a4:100"
      if ((read_size = read(s, buf, BUF_LEN)) > 0)
        write(1, buf, read_size);
      buf[read_size] = '\0';
      std::stringstream ss(buf);
      for (size_t i = 0; i < n; i++) { std::string tmp; ss >> tmp; }
      std::vector<std::string> ts;
      for (size_t i = 0; i < m; i++) {
        std::string tmp; ss >> tmp; ts.push_back(tmp.substr(tmp.find(':') + 1));
      }
      tenders.push_back(ts);
      
      // consequent prices
      if ((read_size = read(s, buf, BUF_LEN)) > 0) write(1, buf, read_size);
      buf[read_size] = '\0';
      if (std::string(buf).find("end") != std::string::npos) break;
      ss.str(""); ss.clear(); ss.str(buf);
      std::vector<int> ps;
      for (size_t i = 0; i < n; i++) {
        std::string tmp; ss >> tmp;
        ps.push_back(atoi(tmp.substr(tmp.find(':') + 1).c_str()));
      }
      prices.push_back(ps);
    }

    // caluculate the cost
    // "winner g1:2 g2:1 "
    read_size = read(s, buf, BUF_LEN);
    buf[read_size] = '\0';
    bool win[n]; memset(win, 0, sizeof(win));
    std::bitset<ITEM_MAX> ws(0);
    std::stringstream ss(buf);
    std::string tmp; ss >> tmp;
    for (size_t i = 0; i < n; i++) {
      ss >> tmp;
      if (atoi(tmp.substr(tmp.find(':') + 1).c_str()) == cid) {
        win[i] = true; ws[i] = 1;
      }
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
    printf("BENEFIT = %d(EVAL) - %d(COST) = %d\n", evals[ws.to_ulong() - 1], total_cost, evals[ws.to_ulong() - 1] - total_cost);
  }

  void dump() {
    std::ofstream ofs(log_file.c_str(), std::ios::out | std::ios::app);
    if (first_day) ofs << n << " " << m << std::endl;
    for (size_t i = 0; i < prices.size(); i++) {
      for (size_t j = 0; j < n; j++) ofs << prices[i][j] << " ";
      for (size_t j = 0; j < m; j++) ofs << tenders[i][j] << " ";
      ofs << std::endl;
    }
  }
};  

#endif
