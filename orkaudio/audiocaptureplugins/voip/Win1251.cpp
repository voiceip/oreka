#include "Win1251.h"

#define D0_CYR_UTF_HEX 0xD090		// utf-8 hex correspond to first cyrillic char which has D0
#define D1_CYR_UTF_HEX 0xD180 		// utf-8 hex correspond to first cyrillic char which has D1
void InitializeWin1251Table(unsigned short utf[256])
{
	utf[0] = 0;

	utf[192] = D0_CYR_UTF_HEX;		//In Win 1251 table, Cyrillic characters from 192 to 255
	for (int i=0; i<48; i++)
	{
		utf[192 + i] =  htons(D0_CYR_UTF_HEX + i);		//reverse the order of bytes in order to copy them properly later
	}

	utf[240] = D1_CYR_UTF_HEX;		//In Win 1251 table, Cyrillic characters from 240 to 255
	for (int i=0; i<16; i++)
	{
		utf[240 + i] =  htons(D1_CYR_UTF_HEX + i);
	}
}

void ConvertWin1251ToUtf8(char partyName[PARTY_NAME_SIZE],unsigned short utf[256])
{
	int c_pos;
	int tmpPos = 0;
	unsigned char tmpPartyName[PARTY_NAME_SIZE*2];		//double size to avoid crash out of memory, because Cyrillic characters occupy 2 bytes
	unsigned short *beginTable = utf;
	unsigned char *beginTmpPartyName = tmpPartyName;

	for(int i=0; i<PARTY_NAME_SIZE; i++)
	{
		if((unsigned char)partyName[i] > 191)
		{
			c_pos = (int)((unsigned char)partyName[i]);
			memcpy(beginTmpPartyName+tmpPos, (unsigned char*)(beginTable+c_pos), sizeof(unsigned short));
			tmpPos += 2;
		}
		else
		{
			tmpPartyName[tmpPos] = partyName[i];
			tmpPos++;
		}
	}
	char *partyNamePtr = partyName;
	memset(partyNamePtr, 0, PARTY_NAME_SIZE);
	memcpy((unsigned char*)partyNamePtr, beginTmpPartyName, PARTY_NAME_SIZE);

}

