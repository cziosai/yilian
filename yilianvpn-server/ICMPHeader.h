#ifndef ICMPHEADER_H
#define ICMPHEADER_H
#include <iostream>
#include "CommonMethods.h"
#include "IPHeader.h"

/*
//icmp报文(能到达目的地,响应-请求包)
struct icmp8
{
    u_char icmp_type; //type of message(报文类型)
    u_char icmp_code; //type sub code(报文类型子码)
    u_short icmp_cksum;
    u_short icmp_id;
    u_short icmp_seq;
    char icmp_data[ 1];
};
*/
class ICMPHeader {
public:
    //uint8_t  type;
    static const short offset_type = 0;
    //uint8_t code;
    static const short offset_code = 1;
    //uint16_t check_sum;
    static const short offset_check_sum = 2;
    char* mData;
    int mOffset;

    ICMPHeader(char* data, int offset) {
        mData = data;
        mOffset = offset;
    }

    unsigned char getType() {
        return mData[mOffset + offset_type] & 0xFF;
    }

    void setType(char value) {
        mData[mOffset + offset_type] = value;
    }

    unsigned char getCode() {
        return mData[mOffset + offset_code] & 0xFF;
    }

    void setCode(char value) {
        mData[mOffset + offset_code] = value;
    }

    unsigned short getCrc() {
        return CommonMethods::readShort(mData, mOffset + offset_check_sum) & 0xFFFF;
    }

    void setCrc(short value) {
        CommonMethods::writeShort(mData, mOffset + offset_check_sum, value);
    }

    bool ComputeICMPChecksum(IPHeader &ipHeader) {
        ipHeader.ComputeIPChecksum();
        long ipData_len = ipHeader.getDataLength();
        if (ipData_len < 0) {
            return false;
        }
        setCrc((short) 0);
        unsigned long sum = 0;
        // 将所有16位字相加
        int num = mOffset;
        while (ipData_len > 1) {
            sum += CommonMethods::readShort(mData, num)  & 0xFFFF;
            num += 2;
            ipData_len -= 2;
        }

        // 处理最后一个奇数字节
        if (ipData_len > 0) {
            sum += (mData[num] & 0xFF) << 8;
        }

        // 处理进位（回卷），直到高16位为0
        while ((sum >> 16) != 0) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        setCrc((short) ~sum);
        // 返回反码
        return true;
    }

    std::string toString() {
        std::stringstream ss;
		ss  << "type="
		    << getType()
		    << "code="
		    << getCode()
		    << "crc="
		    << getCrc();
		return ss.str();
    }
}; 

#endif

