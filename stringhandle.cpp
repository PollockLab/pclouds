/**
 * STP: This is clearly a cpp file. It used to be named *.c.
 *
 */

#include "stringhandle.h"
#include "macrodefine.h"
#include <cstring>

int strcompare(int i, char *source, char *target)
{
	int count;
	for (count = 0; count <i; count++)
	   if (*(source +count) != *(target +count))
		 return 0;

	return 1;
}

void strright(int i, char *source, char *temp)
{
	int count, j;
     
	j= i-1;
	
	int size = strlen(source);
     
	for (count = size - 1; count > size-i-1; count--)
	{
		*(temp+j) = *(source+count);
		j--;
	}

	*(temp+i) = '\0';
}

void strleft(int i, char* source, char* target)
{
	int count, j;
     
	for (count= 0; count < i; count++)
		*(target+count) = *(source+count);

	*(target+i) = '\0';
}

int strreplaceright(char *source, char *target, char *replace)
{
	 int i;
	 for (i=0; i<(signed)strlen(source) - (signed)strlen(replace); i++)
		 *(target+i) = *(source + i);
	 for (i=(signed)strlen(source) - (signed)strlen(replace); i< (signed)strlen(source); i++)
		 *(target +i) = *(replace + i - (signed)strlen(source) + (signed)strlen(replace));
	 return 1;
}

int straddright(char *source, char *target, char *add)
{
	int i;
	for (i=0; i<(signed)strlen(source); i++)
		*(target+i) = *(source +i); 
	for (i=(signed)strlen(source); i< (signed)strlen(source)+(signed)strlen(add); i++)
		*(target+i) = *(add + i - (signed)strlen(source));
	return 1;
}

int stringinitial(char *pchSource, int iLength)
{
	memset(pchSource, ' ', iLength);
	
	*(pchSource+iLength) = '\0';

	return (1);
}

void getsubstring(char* pchSource, char *pchTarget, int nStart, int nEnd)
{
	int count;
	
	for (count = nStart; count <= nEnd; count ++)
		*(pchTarget +count - nStart) = *(pchSource + count);
}

void getreversecomplement(const char* pchSource, char* pchTarget)
{
	int count;
		
	int size = strlen(pchSource);
	
	for ( count = 0; count < strlen(pchSource); count++)
	{
		if ( *(pchSource+count) == 'A' || *(pchSource+count) == 'a')
			*(pchTarget + size - 1 - count) = 'T';
		else if (*(pchSource+count) == 'T' || *(pchSource+count) == 't')
			*(pchTarget + size - 1 - count) = 'A';
		else if (*(pchSource+count) == 'C' || *(pchSource+count) == 'c')
			*(pchTarget + size - 1 - count) = 'G';
		else if (*(pchSource+count) == 'G' || *(pchSource+count) == 'g')
			*(pchTarget + size - 1 - count) = 'C';
		else *(pchTarget + size - 1 - count) = 'N';
	}
	
	*(pchTarget + size) = '\0';
}

int stringcompare(char *pchSource, char *pchTarget)
{
	int count = 0;
	
	int nCompared = 1;
	
	for (count = 0; count< (signed)strlen(pchSource); )
	{
		if ((*(pchSource + count) == *(pchTarget + count)) || (*(pchSource + count) + 32 == *(pchTarget + count)) || (*(pchSource + count)-32 == *(pchTarget + count)))
			count ++;
		else 
			return(1);
	}
	
	return(0);
}

int stringcompare(const char *pchSource, const char *pchTarget)
{
	int count = 0;
	
	int nCompared = 1;
	
	for (count = 0; count< (signed)strlen(pchSource); )
	{
		if ((*(pchSource + count) == *(pchTarget + count)) || (*(pchSource + count) + 32 == *(pchTarget + count)) || (*(pchSource + count)-32 == *(pchTarget + count)))
			count ++;
		else 
			return(1);
	}
	
	return(0);
}

int stringcopy(char *pchSource, char *pchTarget, int iLength)
{
	int count;
	
	stringinitial(pchTarget, iLength);
	
	for (count = 0; count < iLength; count++)
		*(pchTarget + count) = *(pchSource + count);
		
	return(1);

}

int stringtonumber(const std::string& m_strTemp)
{
	int temp = 0;
	
	int length = m_strTemp.length();
	
	for (int i=0; i< length; i++)
		temp = temp * 10 + m_strTemp[i] - ZERO;
	
	return(temp);
}

unsigned int stringtolargenumber(const std::string& m_strTemp)
{
	unsigned int temp = 0;
	
	int length = m_strTemp.length();
	
	for (int i=0; i< length; i++)
		temp = temp * 10 + (unsigned)(m_strTemp[i] - ZERO);
	
	return(temp);
}

void stringtoarray(const std::string& src, char* target)
{
	int i;
	
	int length = src.length();
	
	for(i = 0; i< length; i++)
		target[i] = src[i];
	
	target[i] = '\0';
}

bool isonessr(char *pchPattern)
{
	int i;

	for (i = 0; i< strlen(pchPattern) -1; i++)
	{
		if (pchPattern[i+1] != pchPattern[i])
			return(0);
	}

	return(1);
}

bool istwossr(char *pchPattern)
{
	int i;

	for (i = 0; i< strlen(pchPattern)-2; i ++)
	{
		if (pchPattern[i+2] != pchPattern[i])
			return(0);
	}

	return(1);
}

bool isthreessr(char *pchPattern)
{
	int i;

	for (i = 0; i< strlen(pchPattern)-3; i ++)
	{
		if (pchPattern[i+3] != pchPattern[i])
			return(0);
	}

	return(1);
}

bool isfourssr(char *pchPattern)
{
	int i;

	for (i = 0; i< strlen(pchPattern)-4; i ++)
	{
		if (pchPattern[i+4] != pchPattern[i])
			return(0);
	}

	return(1);
}

bool issegmentvalid(char *pchPattern)
{
	int count;
	
	//STP: Passing in size will be faster. Perhaps a global would work better.
	int size = strlen(pchPattern);
	if(size == 0) { return(0); }
	char temp;
	
	for (count =0; count < size; count ++)
	{
		temp = *(pchPattern+count);
		
		if ( temp != 'A' && temp != 'a' && temp != 'C' && temp != 'c' && temp != 'T' && temp != 't' && temp != 'G' && temp != 'g' )
			return(0);
	}
	
	return(1);
}

