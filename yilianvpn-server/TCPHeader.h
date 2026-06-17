#ifndef TCPHEADER_H
#define TCPHEADER_H
#include "CommonMethods.h"
#include "IPHeader.h"

class TCPHeader
{
	/**
	 * ＴＣＰ报头格式
	 * ０                                                           １５ １６
	 * ３１
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜               源端口号（ｓｏｕｒｃｅ　ｐｏｒｔ）           　｜       　目的端口号（ｄｅｓｔｉｎａｔｉｏｎ　ｐｏｒｔ）           ｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜　　　　　　　　　　　　　　　　　　　　　　　　顺序号（ｓｅｑｕｅｎｃｅ　ｎｕｍｂｅｒ）　　　　　　　　　　　　　　　　　　　　　｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜　　　　　　　　　　　　　　　　　　　　　确认号（ａｃｋｎｏｗｌｅｄｇｅｍｅｎｔ　ｎｕｍｂｅｒ）　　　　　　　　　　　　　　　　　｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜　 ＴＣＰ报头　 ｜　　保　　          ｜Ｕ｜Ａ｜Ｐ｜Ｒ｜Ｓ｜Ｆ｜                                                                  ｜
	 * ｜　　　长度　　  ｜　　留　　          ｜Ｒ｜Ｃ｜Ｓ｜Ｓ｜Ｙ｜Ｉ｜　　　　　　窗口大小（ｗｉｎｄｏｗ　ｓｉｚｅ）                    ｜
	 * ｜　　（４位）    ｜　（６位）          ｜Ｇ｜Ｋ｜Ｈ｜Ｔ｜Ｎ｜Ｎ｜                                                                  ｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜              校验和（ｃｈｅｃｋｓｕｍ）                     ｜           紧急指针（ｕｒｇｅｎｔ　ｐｏｉｎｔｅｒ）                ｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜                                                选项＋填充（０或多个３２位字）                                                  　｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜                                                   数据（０或多个字节）                                                           ｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 **/

public:
	static const char TCP_HEADER_SIZE = 20;
	static const char FIN = 1;
	static const char SYN = 2;
	static const char RST = 4;
	static const char PSH = 8;
	static const char ACK = 16;
	static const char URG = 32;

	static const char offset_src_port = 0; // 16位源端口
	static const char offset_dest_port = 2; // 16位目的端口
	static const char offset_seq = 4; // 32位序列号
	static const char offset_ack = 8; // 32位确认号
	static const char offset_lenres = 12; // 4位首部长度 + 4位保留位
	static const char offset_flag = 13; // 2位保留字 + 6位标志位
	static const char offset_win = 14; //16位窗口大小
	static const char offset_crc = 16; // 16位校验和
	static const char offset_urp = 18; // 16位紧急偏移量

	char *mData;
	int mOffset;

	TCPHeader()
	{
	}

	TCPHeader(char *data, int offset)
	{
		mData = data;
		mOffset = offset;
	}
	
	unsigned int getHeaderLength()
	{
    	int lenres = mData[mOffset + offset_lenres] & 0xFF;
    	return (lenres >> 4) * 4;
	}

	void setHeaderLength(char value)
	{
		char temp = mData[mOffset + offset_lenres];
		mData[mOffset + offset_lenres] = (char)((((value + 3) / 4) << 4) | (temp & 0x0F));
	}

	unsigned short getWindow()
	{
		return  CommonMethods::readShort(mData, mOffset + offset_win) & 0xFFFF;
	}

	void setWindow(short value)
	{
		CommonMethods::writeShort(mData, mOffset + offset_win, value);
	}

	unsigned short getUrp()
	{
		return CommonMethods::readShort(mData, mOffset + offset_urp) & 0xFFFF;
	}

	void setUrp(short value)
	{
		CommonMethods::writeShort(mData, mOffset + offset_urp, value);
	}

	unsigned short getSourcePort()
	{
		return CommonMethods::readShort(mData, mOffset + offset_src_port) & 0xFFFF;
	}

	void setSourcePort(short value)
	{
		CommonMethods::writeShort(mData, mOffset + offset_src_port, value);
	}

	unsigned short getDestinationPort()
	{
		return CommonMethods::readShort(mData, mOffset + offset_dest_port) & 0xFFFF;
	}

	void setDestinationPort(short value)
	{
		CommonMethods::writeShort(mData, mOffset + offset_dest_port, value);
	}
	
	unsigned char getFlag()
	{
		return mData[mOffset + offset_flag] & 0x3F;
	}

	void setFlag(char value)
	{
    	char temp = mData[mOffset + offset_flag];
    	mData[mOffset + offset_flag] = (temp & 0xC0) | (value & 0x3F);
	}

	unsigned short getCrc()
	{
		return CommonMethods::readShort(mData, mOffset + offset_crc) & 0xFFFF;
	}

	void setCrc(short value)
	{
		CommonMethods::writeShort(mData, mOffset + offset_crc, value);
	}

	unsigned int getSeqID()
	{
		return CommonMethods::readInt(mData, mOffset + offset_seq) & 0xFFFFFFFF;
	}

	void setSeqID(int value)
	{
		CommonMethods::writeInt(mData, mOffset + offset_seq, value);
	}

	unsigned int getAckID()
	{
		return CommonMethods::readInt(mData, mOffset + offset_ack) & 0xFFFFFFFF;
	}

	void setAckID(int value)
	{
		CommonMethods::writeInt(mData, mOffset + offset_ack, value);
	}

	// 计算TCP校验和
	// TCP检验和 = 整个TCP报文（不合检验和部分） +  源地址 + 目标地址 + 协议 + tcp报文长度
	bool ComputeTCPChecksum(IPHeader &ipHeader)
	{
		ipHeader.ComputeIPChecksum();
		int ipData_len = ipHeader.getDataLength();
		if (ipData_len < 0)
		{
			return false;
		}

		long sum = ipHeader.getsum(ipHeader.mData, ipHeader.mOffset + IPHeader::offset_src_ip, 8);
		sum += ipHeader.getProtocol() & 0xFF;
		sum += ipData_len;

		short oldCrc = getCrc();
		setCrc((short) 0);

		short newCrc = ipHeader.checksum(sum, mData, mOffset, ipData_len);

		setCrc(newCrc);
		return oldCrc == newCrc;
	}

	std::string toString()
	{
		std::stringstream ss;
		ss  << ((getFlag() & SYN) == SYN ? "SYN" : "")
		    << ((getFlag() & ACK) == ACK ? "ACK" : "")
		    << ((getFlag() & PSH) == PSH ? "PSH" : "")
		    << ((getFlag() & RST) == RST ? "RST" : "")
		    << ((getFlag() & FIN) == FIN ? "FIN" : "")
		    << ((getFlag() & URG) == URG ? "URG" : "")
		    << " "
		    << getSourcePort()
		    << "->"
		    << getDestinationPort()
		    << " "
		    << getSeqID()
		    << ":"
		    << getAckID();
		return ss.str();
	}
};

#endif

