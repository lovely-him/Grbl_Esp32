#include "Jk_tcp.h"
namespace WebUI {
    WiFiServer* Jk_Server::_jk_tcp = nullptr;
    WiFiClient Jk_Server::_jk_Client;
    bool Jk_Server::yi_lian_jie=false;
    int32_t Jk_Server::ji_shu=0;
}
