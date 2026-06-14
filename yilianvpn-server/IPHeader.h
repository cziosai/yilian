#ifndef IPHEADER_H
#define IPHEADER_H
#include "CommonMethods.h"

class IPHeader
{

	/**
	 * IP报文格式
	 * 0                                   　　　　           15  16　　　　　　　　　　　　　　　　　　　　　　　　     31
	 * ｜　－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜  ４　位     ｜   ４位首     ｜      ８位服务类型      ｜      　　         １６位总长度            　            ｜
	 * ｜  版本号     ｜   部长度     ｜      （ＴＯＳ）　      ｜      　 　 （ｔｏｔａｌ　ｌｅｎｇｔｈ）    　           ｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜  　　　　　　　　１６位标识符                         ｜　３位    ｜　　　　１３位片偏移                         ｜
	 * ｜            （ｉｎｄｅｎｔｉｆｉｅｒ）                 ｜　标志    ｜      （ｏｆｆｓｅｔ）　　                   ｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜      ８位生存时间ＴＴＬ    ｜       ８位协议          ｜　　　　　　　　１６位首部校验和                         ｜
	 * ｜（ｔｉｍｅ　ｔｏ　ｌｉｖｅ）｜  （ｐｒｏｔｏｃｏｌ）   ｜              （ｃｈｅｃｋｓｕｍ）                       ｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜                                ３２位源ＩＰ地址（ｓｏｕｒｃｅ　ａｄｄｒｅｓｓ）                                  ｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜                           ３２位目的ＩＰ地址（ｄｅｓｔｉｎａｔｉｏｎ　ａｄｄｒｅｓｓ）                           ｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜                                             ３２位选项（若有）                                                   ｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 * ｜                                                                                                                  ｜
	 * ｜                                                  数据                                                            ｜
	 * ｜                                                                                                                  ｜
	 * ｜－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－｜
	 **/
public:
	static const char IP4_HEADER_SIZE = 20;
	static const char V4_VERSION = 4;
	static const char ICMP = 1;
	static const char IGMP = 2;
	static const char TCP = 6;  // 6: TCP协议号
	static const char UDP = 17; // 17: UDP协议号
	static const char offset_proto = 9; // 9：8位协议偏移
	static const char offset_src_ip = 12; // 12：源ip地址偏移
	static const char offset_dest_ip = 16; // 16：目标ip地址偏移
	static const char offset_ver_ihl = 0; // 0: 版本号（4bits） + 首部长度（4bits）
	static const char offset_tos = 1; // 1：服务类型偏移
	static const char offset_tlen = 2; // 2：总长度偏移
	static const char offset_identification = 4; // 4：16位标识符偏移
	static const char offset_flags_fo = 6; // 6：标志（3bits）+ 片偏移（13bits）
	static const char offset_ttl = 8; // 8：生存时间偏移
	static const char offset_crc = 10; // 10：首部校验和偏移
	static const char offset_op_pad = 20; // 20：选项 + 填充

	char *mData;
	int mOffset;

	IPHeader()
	{
	}

	IPHeader(char *data, int offset)
	{
		mData = data;
		mOffset = offset;
	}

	int getDataLength()
	{
		return (int)getTotalLength() - (int)getHeaderLength();
	}

	unsigned char getVersion()
	{
		return (mData[mOffset + offset_ver_ihl] >> 4) & 0x0F;
	}

	void setVersion(unsigned char ver)
	{
		char hlen = mData[mOffset + offset_ver_ihl] & 0x0F;
		mData[mOffset + offset_ver_ihl] = (char) ((ver << 4) | hlen);
	}

	unsigned char getHeaderLength()
	{
		return ((mData[mOffset + offset_ver_ihl] & 0x0F) * 4);
	}

	void setHeaderLength(unsigned char value)
	{
		char ver = mData[mOffset + offset_ver_ihl] & 0xF0;  // 保留高4位版本号
    	mData[mOffset + offset_ver_ihl] = (char)(ver | ((value + 3) / 4));
	}

	unsigned char getTos()
	{
		return mData[mOffset + offset_tos];
	}

	void setTos(unsigned char value)
	{
		mData[mOffset + offset_tos] = value;
	}

	unsigned short getTotalLength()
	{
		return CommonMethods::readShort(mData, mOffset + offset_tlen);
	}

	void setTotalLength(unsigned short value)
	{
		CommonMethods::writeShort(mData, mOffset + offset_tlen, value);
	}

	unsigned short getIdentification()
	{
		return CommonMethods::readShort(mData, mOffset + offset_identification);
	}

	void setIdentification(unsigned short value)
	{
		CommonMethods::writeShort(mData, mOffset + offset_identification, value);
	}

	unsigned short getFlagsAndOffset()
	{
		return CommonMethods::readShort(mData, mOffset + offset_flags_fo);
	}

	void setFlagsAndOffset(unsigned short value)
	{
		CommonMethods::writeShort(mData, mOffset + offset_flags_fo, value);
	}

	unsigned char getTTL()
	{
		return mData[mOffset + offset_ttl];
	}

	void setTTL(unsigned char value)
	{
		mData[mOffset + offset_ttl] = value;
	}

	unsigned char getProtocol()
	{
		return mData[mOffset + offset_proto];
	}

	void setProtocol(unsigned char value)
	{
		mData[mOffset + offset_proto] = value;
	}

	unsigned short getCrc()
	{
		return CommonMethods::readShort(mData, mOffset + offset_crc);
	}

	void setCrc(unsigned short value)
	{
		CommonMethods::writeShort(mData, mOffset + offset_crc, value);
	}

	unsigned int getSourceIP()
	{
		return CommonMethods::readInt(mData, mOffset + offset_src_ip);
	}

	void setSourceIP(unsigned int value)
	{
		CommonMethods::writeInt(mData, mOffset + offset_src_ip, value);
	}

	unsigned int getDestinationIP()
	{
		return CommonMethods::readInt(mData, mOffset + offset_dest_ip);
	}

	void setDestinationIP(unsigned int value)
	{
		CommonMethods::writeInt(mData, mOffset + offset_dest_ip, value);
	}

	//计算校验和
	unsigned short checksum(long sum, char *buf, int offset, int len)
	{
		sum += getsum(buf, offset, len);
		while ((sum >> 16) > 0)
		{
			sum = (sum & 0xFFFF) + (sum >> 16);
		}
		return (short)(~sum & 0xFFFF);
	}

	long getsum(char *buf, int offset, int len)
	{
		long sum = 0;
		while (len > 1)
		{
			sum += CommonMethods::readShort(buf, offset) & 0xFFFF;
			offset += 2;
			len -= 2;
		}

		if (len > 0)
		{
			sum += (buf[offset] & 0xFF) << 8;
		}

		return sum;
	}

	// 计算IP包的校验和
	bool ComputeIPChecksum()
	{
		short oldCrc = getCrc();
		setCrc((short) 0);
		short newCrc = checksum(0, mData, mOffset, getHeaderLength());
		setCrc(newCrc);
		return oldCrc == newCrc;
	}

	std::string toString()
	{
		std::stringstream ss;
		ss  << CommonMethods::ipIntToString(getSourceIP())
		    << "->"
		    << CommonMethods::ipIntToString(getDestinationIP())
		    << " Pro="
		    << getProtocol()
		    << ", Hlen="
		    << getHeaderLength()
		    << ", TotalLen="
		    << getTotalLength();
		return ss.str();
	}
};

#endif

