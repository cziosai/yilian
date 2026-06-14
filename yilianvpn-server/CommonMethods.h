#ifndef COMMONMETHODS_H
#define COMMONMETHODS_H
#include <vector>
#include <string>
#include <cstring>
#include <sstream>

class CommonMethods
{
public:
	static unsigned int readInt(char *data, int offset)
	{
		int r = ((data[offset] & 0xFF) << 24)
		        | ((data[offset + 1] & 0xFF) << 16)
		        | ((data[offset + 2] & 0xFF) << 8)
		        | (data[offset + 3] & 0xFF);
		return r;
	}

	static unsigned short readShort(char *data, int offset)
	{
		int r = ((data[offset] & 0xFF) << 8) | (data[offset + 1] & 0xFF);
		return (short) r;
	}

	static void writeInt(char *data, int offset, unsigned int value)
	{
		data[offset] = (char) (value >> 24);
		data[offset + 1] = (char) (value >> 16);
		data[offset + 2] = (char) (value >> 8);
		data[offset + 3] = (char) value;
	}

	static void writeShort(char *data, int offset, unsigned short value)
	{
		data[offset] = (char) (value >> 8);
		data[offset + 1] = (char) (value);
	}

	static void arraycopy(char *srcbuf, int srcoffset, char *desbuf, int desoffset, int size)
	{
		memmove(desbuf + desoffset, srcbuf + srcoffset, size);
	}
	
	static std::string ipIntToString(int ip)
	{
		std::stringstream ss;
		ss << ((ip >> 24) & 0x00FF) << "." << ((ip >> 16) & 0x00FF) << "." << ((ip >> 8) & 0x00FF) << "." << (ip & 0x00FF);
		return ss.str();
	}

	static int ipStringToInt(const std::string ip)
	{
		std::vector<std::string> arrayStrings = split(ip, '.');
		int r = (atoi(arrayStrings[0].c_str()) << 24)
		        | (atoi(arrayStrings[1].c_str()) << 16)
		        | (atoi(arrayStrings[2].c_str()) << 8)
		        | (atoi(arrayStrings[3].c_str()));
		return r;
	}

	static std::vector<std::string> split(std::string s, const char word)
	{
		std::istringstream iss(s);
		std::string token;
		std::vector<std::string> vs;
		while(getline(iss, token, word))
		{
			vs.push_back(token);
		}
		return vs;
	}
};

#endif

