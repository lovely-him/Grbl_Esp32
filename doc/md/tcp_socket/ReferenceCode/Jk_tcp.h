#ifndef JK_TCP_H
#define JK_TCP_H
#include "WiFiServer.h"
#include "WiFiClient.h"
#include "grbl.h"
#define __duan_kou 8081
namespace WebUI {
enum class Fan_Hui : uint8_t {
    OK        = 1,//正常
    CUO_WU    = 2,//错误
    CHAO_SHI  = 3,//超时
};
class Jk_Server {
    public:
        Jk_Server()
        {
 
        }

        bool   begin()
        {
            grbl_send(CLIENT_ALL, "jk_chu_shi_hua\n");
            _jk_tcp=new WiFiServer(__duan_kou);
            grbl_send(CLIENT_ALL, "jk_chu_shi_hua2\n");
            _jk_tcp->begin();
            return false;
        }
        void   end()
        {

        }
        void   handle()
        {
            if(!_jk_tcp)
            {
                // grbl_send(CLIENT_ALL, "jk_ji_mao\n");
                return;
            }
            if(kai_shi)
            {
                if(millis()-uptime>10000)
                {
                    uptime=millis();
                    file.close();
                    set_sd_state(SDState::Idle);
                    kai_shi=false;
                    grbl_sendf(CLIENT_ALL, "chao_shi!!!\n");
                    fan_hui(bao_xu_hao,Fan_Hui::CHAO_SHI);
                    _jk_Client.stop();
                }
            }
            if(yi_lian_jie==false)
            {
                if (_jk_tcp->hasClient())
                {
                    _jk_Client = _jk_tcp->available();
                    grbl_send(CLIENT_ALL, "jk_lian_jie\n");
                    yi_lian_jie=true;
                }
            }else
            {
                if(_jk_Client.connected())
                {
                    int ke_yong=_jk_Client.available();//可用数据个数
                    if (ke_yong>0) 
                    {
                        uint8_t shu[1024*2];
                        int du_qu=_jk_Client.read(shu,ke_yong>1024*2?1024*2:ke_yong);
                        if(du_qu>0)
                        {
                            for(int i=0;i<du_qu;i++)
                            {
                                jie_shou(shu[i]);
                            }
                        }
                    }
                }else
                {
                    yi_lian_jie=false;
                    ji_shu=0;
                    jie_shou_ji_shu=0;
                    file.close();
                    kai_shi=false;
                    set_sd_state(SDState::Idle);
                    grbl_send(CLIENT_ALL, "jk_duan_kai!!!\n");
                    _jk_Client.stop();
                }
            }
        }
        void jie_shou(int shu)
        {
            if(jie_shou_ji_shu==0)
            {
                if(shu==0xab)
                    buffer[jie_shou_ji_shu++]=shu;
            }else if(jie_shou_ji_shu==1)
            {
                buffer[jie_shou_ji_shu++]=shu;
            }else if(jie_shou_ji_shu==2)
            {
                buffer[jie_shou_ji_shu++]=shu;
                chang=shu<<8;
            }else if(jie_shou_ji_shu==3)
            {
                buffer[jie_shou_ji_shu++]=shu;
                chang=chang|shu;
                if(chang>1024*32+10)
                {
                    jie_shou_ji_shu=0;//出错, 重新接收
                    file.close();
                    set_sd_state(SDState::Idle);
                    kai_shi=false;
                }
            }else if(jie_shou_ji_shu<chang)
            {
                buffer[jie_shou_ji_shu++]=shu;
                if(jie_shou_ji_shu==chang)
                {
                    chu_li();
                    jie_shou_ji_shu=0;
                }
            }
        }
        void chu_li()
        {
            uint16_t he=0;
            for(int i=0;i<chang-2;i++)
            {
                he=he+buffer[i];
            }
            uint16_t he2=buffer[chang-2]<<8|buffer[chang-1];
            if(he==he2)
            {
                
                String wen_jian_ming="/";
                switch(buffer[1])
                {
                    case 1:
                        for (int i = 4; i < 4 + chang-6; i++) {//取出文件名, 从第4个字符开始的字符到20个字符是文件名
                            wen_jian_ming += String((char)buffer[i]);
                        }
                        state = get_sd_state(true);
                        if (state != SDState::Idle) {
                            if (state == SDState::NotPresent) {
                                grbl_send(CLIENT_ALL, "No SD Card\n");
                                break;
                            } else {
                                grbl_send(CLIENT_ALL, "SD Card Busy\n");
                                break;
                            }
                        }
                        //打开sd卡并创建nc文件
                        // 创建文件
                        file = SD.open(wen_jian_ming, FILE_WRITE);
                        if (!file) {
                            break;
                        }
                        set_sd_state(SDState::Busy);
                        // file=SD.open(wen_jian_ming, FILE_WRITE);
                        bao_xu_hao=(buffer[4]<<8|buffer[5]) + 1 ;
                        kai_shi=true;
                        uptime=millis();
                        fan_hui(bao_xu_hao,Fan_Hui::OK);
                        break;
                    case 2:
                        // grbl_sendf(CLIENT_ALL, "!!!1:%d\n",millis());
                        // set_sd_state(SDState::Busy);
                        he=file.write(buffer+6,chang-8);
                        if(he!=chang-8)
                        {
                            grbl_sendf(CLIENT_ALL, "xie_ru_shi_bai fan_hui:%d,yao_xie_ru:%d\n",he,chang-8);
                            file.close();
                            set_sd_state(SDState::Idle);
                            kai_shi=false;
                            fan_hui(bao_xu_hao,Fan_Hui::CUO_WU);
                            _jk_Client.stop();
                            break;
                        }
                        // grbl_sendf(CLIENT_ALL, "!!!a:%d,chang:%d\n",he,chang-8);
                        bao_xu_hao = (buffer[4] << 8 | buffer[5]) + 1;
                        uptime=millis();
                        fan_hui(bao_xu_hao,Fan_Hui::OK);
                        // set_sd_state(SDState::Idle);
                        // grbl_sendf(CLIENT_ALL, "!!!3:%d\n",millis());
                        break; 
                    case 3:
                        file.close();
                        set_sd_state(SDState::Idle);
                        kai_shi=false;
                        grbl_sendf(CLIENT_ALL, "!!!jie_shou_wan_cheng\n");
                        break;
                    default:
                    // 当 command 不匹配任何 case 时执行的代码
                    break;
                }
            }else
            {
                grbl_sendf(CLIENT_ALL, "jiao_yan_shi_bai:%d,%d!!!\n",he,he2);
                file.close();
                set_sd_state(SDState::Idle);
                kai_shi=false;
                fan_hui(bao_xu_hao,Fan_Hui::CUO_WU);
                _jk_Client.stop();
            }
            
        }
        void fan_hui(int16_t xu_hao,Fan_Hui ming_ling)
        {
            uint8_t buffer[10];
            buffer[0]=0xab;
            buffer[1]=(char)ming_ling;
            buffer[2]=0x00;
            buffer[3]=0x08;
            buffer[4]=xu_hao>>8;
            buffer[5]=xu_hao;
            int he=0;
            for(int i=0;i<6;i++)
            {
                he=he+buffer[i];
            }
            buffer[6]=he>>8;
            buffer[7]=he;
            if(_jk_Client.connected())
                _jk_Client.write(buffer,8);
        }
        ~Jk_Server()
        {

        }

    private:
        static WiFiServer* _jk_tcp;
        static WiFiClient  _jk_Client;
        static bool yi_lian_jie;
        static int32_t ji_shu;
        uint8_t buffer[1024*32+10];
        int jie_shou_ji_shu=0;
        uint16_t chang=0;
        uint16_t bao_xu_hao=0;
        unsigned long uptime=0;
        File file;
        File root;
        SDState state;
        bool kai_shi=false;
    };

}

#endif
