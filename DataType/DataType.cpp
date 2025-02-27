
//***************************************************************************
// DataType.cpp : Defines the entry point for the console application.
//
//***************************************************************************

#include "pch.h"

//***************************************************************************
//
void UseConstValueToCPP()
{
	cout << "//***************************************************************************" << endl;
	cout << "// C++ 자료형 상수 이용" << endl;
	cout << "//***************************************************************************" << endl;
	cout << "char " << "\t\t\t" << " : " << sizeof(char) << " Bytes (" << CHAR_MIN << " ~ " << CHAR_MAX << ")" << endl;
	cout << "signed char " << "\t\t" << " : " << sizeof(signed char) << " Bytes (" << SCHAR_MIN << " ~ " << SCHAR_MAX << ")" << endl;
	cout << "unsigned char " << "\t\t" << " : " << sizeof(unsigned char) << " Bytes (" << 0 << " ~ " << UCHAR_MAX << ")" << endl;
	cout << "wchar_t " << "\t\t" << " : " << sizeof(wchar_t) << " Bytes (" << WCHAR_MIN << " ~ " << WCHAR_MAX << ")" << endl;
	cout << "bool " << "\t\t\t" << " : " << sizeof(bool) << " Bytes (" << false << " ~ " << true << ")" << endl;
	cout << "int " << "\t\t\t" << " : " << sizeof(int) << " Bytes (" << INT_MIN << " ~ " << INT_MAX << ")" << endl;
	cout << "unsigned int " << "\t\t" << " : " << sizeof(unsigned int) << " Bytes (" << 0 << " ~ " << UINT_MAX << ")" << endl;
	cout << "__int8 " << "\t\t\t" << " : " << sizeof(__int8) << " Bytes (" << (int)_I8_MIN << " ~ " << (int)_I8_MAX << ")" << endl;
	cout << "unsigned __int8 " << "\t" << " : " << sizeof(unsigned __int8) << " Bytes (" << 0 << " ~ " << (int)_UI8_MAX << ")" << endl;
	cout << "__int16 " << "\t\t" << " : " << sizeof(__int16) << " Bytes (" << _I16_MIN << " ~ " << _I16_MAX << ")" << endl;
	cout << "unsigned __int16 " << "\t" << " : " << sizeof(unsigned __int16) << " Bytes (" << 0 << " ~ " << _UI16_MAX << ")" << endl;
	cout << "__int32 " << "\t\t" << " : " << sizeof(__int32) << " Bytes (" << _I32_MIN << " ~ " << _I32_MAX << ")" << endl;
	cout << "unsigned __int32 " << "\t" << " : " << sizeof(unsigned __int32) << " Bytes (" << 0 << " ~ " << _UI32_MAX << ")" << endl;
	cout << "__int64 " << "\t\t" << " : " << sizeof(__int64) << " Bytes (" << _I64_MIN << " ~ " << _I64_MAX << ")" << endl;
	cout << "unsigned __int64 " << "\t" << " : " << sizeof(unsigned __int64) << " Bytes (" << 0 << " ~ " << _UI64_MAX << ")" << endl;
	cout << "short " << "\t\t\t" << " : " << sizeof(short) << " Bytes (" << SHRT_MIN << " ~ " << SHRT_MAX << ")" << endl;
	cout << "unsigned short " << "\t\t" << " : " << sizeof(unsigned short) << " Bytes (" << 0 << " ~ " << USHRT_MAX << ")" << endl;
	cout << "long " << "\t\t\t" << " : " << sizeof(long) << " Bytes (" << LONG_MIN << " ~ " << LONG_MAX << ")" << endl;
	cout << "unsigned long " << "\t\t" << " : " << sizeof(unsigned long) << " Bytes (" << 0 << " ~ " << ULONG_MAX << ")" << endl;
	cout << "long long " << "\t\t" << " : " << sizeof(long long) << " Bytes (" << LLONG_MIN << " ~ " << LLONG_MAX << ")" << endl;
	cout << "unsigned long long " << "\t" << " : " << sizeof(unsigned long long) << " Bytes (" << 0 << " ~ " << ULLONG_MAX << ")" << endl;
	cout << "float " << "\t\t\t" << " : " << sizeof(float) << " Bytes (" << FLT_MIN << " ~ " << FLT_MAX << ")" << endl;
	cout << "double " << "\t\t\t" << " : " << sizeof(double) << " Bytes (" << DBL_MIN << " ~ " << DBL_MAX << ")" << endl;
	cout << "long double " << "\t\t" << " : " << sizeof(long double) << " Bytes (" << LDBL_MIN << " ~ " << LDBL_MAX << ")" << endl;
}

//***************************************************************************
//
void UseNumericLimitsToCPP()
{
	cout << "//***************************************************************************" << endl;
	cout << "// Visual C++ 자료형 numeric_limits 클래스 이용" << endl;
	cout << "//***************************************************************************" << endl;
	cout << "char " << "\t\t\t" << " : " << sizeof(char) << " Bytes (" << (int)(std::numeric_limits<char>::min)() << " ~ " << (int)(std::numeric_limits<char>::max)() << ")" << endl;
	cout << "signed char " << "\t\t" << " : " << sizeof(signed char) << " Bytes (" << (int)(std::numeric_limits<signed char>::min)() << " ~ " << (int)(std::numeric_limits<signed char>::max)() << ")" << endl;
	cout << "unsigned char " << "\t\t" << " : " << sizeof(unsigned char) << " Bytes (" << (int)(std::numeric_limits<unsigned char>::min)() << " ~ " << (int)(std::numeric_limits<unsigned char>::max)() << ")" << endl;
	cout << "wchar_t " << "\t\t" << " : " << sizeof(wchar_t) << " Bytes (" << (std::numeric_limits<wchar_t>::min)() << " ~ " << (std::numeric_limits<wchar_t>::max)() << ")" << endl;
	cout << "bool " << "\t\t\t" << " : " << sizeof(bool) << " Bytes (" << (std::numeric_limits<bool>::min)() << " ~ " << (std::numeric_limits<bool>::max)() << ")" << endl;
	cout << "int " << "\t\t\t" << " : " << sizeof(int) << " Bytes (" << (std::numeric_limits<int>::min)() << " ~ " << (std::numeric_limits<int>::max)() << ")" << endl;
	cout << "unsigned int " << "\t\t" << " : " << sizeof(unsigned int) << " Bytes (" << (std::numeric_limits<unsigned int>::min)() << " ~ " << (std::numeric_limits<unsigned int>::max)() << ")" << endl;
	cout << "__int8 " << "\t\t\t" << " : " << sizeof(__int8) << " Bytes (" << (int)(std::numeric_limits<__int8>::min)() << " ~ " << (int)(std::numeric_limits<__int8>::max)() << ")" << endl;
	cout << "unsigned __int8 " << "\t" << " : " << sizeof(unsigned __int8) << " Bytes (" << (int)(std::numeric_limits<unsigned __int8>::min)() << " ~ " << (int)(std::numeric_limits<unsigned __int8>::max)() << ")" << endl;
	cout << "__int16 " << "\t\t" << " : " << sizeof(__int16) << " Bytes (" << (std::numeric_limits<__int16>::min)() << " ~ " << (std::numeric_limits<__int16>::max)() << ")" << endl;
	cout << "unsigned __int16 " << "\t" << " : " << sizeof(unsigned __int16) << " Bytes (" << (std::numeric_limits<unsigned __int16>::min)() << " ~ " << (std::numeric_limits<unsigned __int16>::max)() << ")" << endl;
	cout << "__int32 " << "\t\t" << " : " << sizeof(__int32) << " Bytes (" << (std::numeric_limits<__int32>::min)() << " ~ " << (std::numeric_limits<__int32>::max)() << ")" << endl;
	cout << "unsigned __int32 " << "\t" << " : " << sizeof(unsigned __int32) << " Bytes (" << (std::numeric_limits<unsigned __int32>::min)() << " ~ " << (std::numeric_limits<unsigned __int32>::max)() << ")" << endl;
	cout << "__int64 " << "\t\t" << " : " << sizeof(__int64) << " Bytes (" << (std::numeric_limits<__int64>::min)() << " ~ " << (std::numeric_limits<__int64>::max)() << ")" << endl;
	cout << "unsigned __int64 " << "\t" << " : " << sizeof(unsigned __int64) << " Bytes (" << (std::numeric_limits<unsigned __int64>::min)() << " ~ " << (std::numeric_limits<unsigned __int64>::max)() << ")" << endl;
	cout << "short " << "\t\t\t" << " : " << sizeof(short) << " Bytes (" << (std::numeric_limits<short>::min)() << " ~ " << (std::numeric_limits<short>::max)() << ")" << endl;
	cout << "unsigned short " << "\t\t" << " : " << sizeof(unsigned short) << " Bytes (" << (std::numeric_limits<unsigned short>::min)() << " ~ " << (std::numeric_limits<unsigned short>::max)() << ")" << endl;
	cout << "long " << "\t\t\t" << " : " << sizeof(long) << " Bytes (" << (std::numeric_limits<long>::min)() << " ~ " << (std::numeric_limits<long>::max)() << ")" << endl;
	cout << "unsigned long " << "\t\t" << " : " << sizeof(unsigned long) << " Bytes (" << (std::numeric_limits<unsigned long>::min)() << " ~ " << (std::numeric_limits<unsigned long>::max)() << ")" << endl;
	cout << "long long " << "\t\t" << " : " << sizeof(long long) << " Bytes (" << (std::numeric_limits<long long>::min)() << " ~ " << (std::numeric_limits<long long>::max)() << ")" << endl;
	cout << "unsigned long long " << "\t" << " : " << sizeof(unsigned long long) << " Bytes (" << (std::numeric_limits<unsigned long long>::min)() << " ~ " << (std::numeric_limits<unsigned long long>::max)() << ")" << endl;
	cout << "float " << "\t\t\t" << " : " << sizeof(float) << " Bytes (" << (std::numeric_limits<float>::min)() << " ~ " << (std::numeric_limits<float>::max)() << ")" << endl;
	cout << "double " << "\t\t\t" << " : " << sizeof(double) << " Bytes (" << (std::numeric_limits<double>::min)() << " ~ " << (std::numeric_limits<double>::max)() << ")" << endl;
	cout << "long double " << "\t\t" << " : " << sizeof(long double) << " Bytes (" << (std::numeric_limits<long double>::min)() << " ~ " << (std::numeric_limits<long double>::max)() << ")" << endl;
}

//***************************************************************************
//
void UseNumericLimitsToWindows()
{
	cout << "//***************************************************************************" << endl;
	cout << "// Windows 자료형 numeric_limits 클래스 이용" << endl;
	cout << "//***************************************************************************" << endl;
	cout << "CHAR " << "\t\t\t" << " : " << sizeof(CHAR) << " Bytes (" << (int)(std::numeric_limits<CHAR>::min)() << " ~ " << (int)(std::numeric_limits<CHAR>::max)() << ")" << endl;
	cout << "WCHAR " << "\t\t\t" << " : " << sizeof(WCHAR) << " Bytes (" << (int)(std::numeric_limits<WCHAR>::min)() << " ~ " << (int)(std::numeric_limits<WCHAR>::max)() << ")" << endl;
	cout << "TCHAR " << "\t\t\t" << " : " << sizeof(TCHAR) << " Bytes (" << (int)(std::numeric_limits<TCHAR>::min)() << " ~ " << (int)(std::numeric_limits<TCHAR>::max)() << ")" << endl;
	cout << "BOOL " << "\t\t\t" << " : " << sizeof(BOOL) << " Bytes (" << (std::numeric_limits<BOOL>::min)() << " ~ " << (std::numeric_limits<BOOL>::max)() << ")" << endl;
	cout << "BYTE " << "\t\t\t" << " : " << sizeof(BYTE) << " Bytes (" << (int)(std::numeric_limits<BYTE>::min)() << " ~ " << (int)(std::numeric_limits<BYTE>::max)() << ")" << endl;
	cout << "WORD " << "\t\t\t" << " : " << sizeof(WORD) << " Bytes (" << (std::numeric_limits<WORD>::min)() << " ~ " << (std::numeric_limits<WORD>::max)() << ")" << endl;
	cout << "DWORD " << "\t\t\t" << " : " << sizeof(DWORD) << " Bytes (" << (std::numeric_limits<DWORD>::min)() << " ~ " << (std::numeric_limits<DWORD>::max)() << ")" << endl;
	cout << "INT " << "\t\t\t" << " : " << sizeof(INT) << " Bytes (" << (std::numeric_limits<INT>::min)() << " ~ " << (std::numeric_limits<INT>::max)() << ")" << endl;
	cout << "UINT " << "\t\t\t" << " : " << sizeof(UINT) << " Bytes (" << (std::numeric_limits<UINT>::min)() << " ~ " << (std::numeric_limits<UINT>::max)() << ")" << endl;
	cout << "LONG " << "\t\t\t" << " : " << sizeof(LONG) << " Bytes (" << (std::numeric_limits<LONG>::min)() << " ~ " << (std::numeric_limits<LONG>::max)() << ")" << endl;
	cout << "ULONG " << "\t\t\t" << " : " << sizeof(ULONG) << " Bytes (" << (std::numeric_limits<ULONG>::min)() << " ~ " << (std::numeric_limits<ULONG>::max)() << ")" << endl;
	cout << "DWORD32 " << "\t\t" << " : " << sizeof(DWORD32) << " Bytes (" << (std::numeric_limits<DWORD32>::min)() << " ~ " << (std::numeric_limits<DWORD32>::max)() << ")" << endl;
	cout << "DWORD64 " << "\t\t" << " : " << sizeof(DWORD64) << " Bytes (" << (std::numeric_limits<DWORD64>::min)() << " ~ " << (std::numeric_limits<DWORD64>::max)() << ")" << endl;
	cout << "INT8 " << "\t\t\t" << " : " << sizeof(INT8) << " Bytes (" << (int)(std::numeric_limits<INT8>::min)() << " ~ " << (int)(std::numeric_limits<INT8>::max)() << ")" << endl;
	cout << "UINT8 " << "\t\t\t" << " : " << sizeof(UINT8) << " Bytes (" << (int)(std::numeric_limits<UINT8>::min)() << " ~ " << (int)(std::numeric_limits<UINT8>::max)() << ")" << endl;
	cout << "INT16 " << "\t\t\t" << " : " << sizeof(INT16) << " Bytes (" << (std::numeric_limits<INT16>::min)() << " ~ " << (std::numeric_limits<INT16>::max)() << ")" << endl;
	cout << "UINT16 " << "\t\t\t" << " : " << sizeof(UINT16) << " Bytes (" << (std::numeric_limits<UINT16>::min)() << " ~ " << (std::numeric_limits<UINT16>::max)() << ")" << endl;
	cout << "INT32 " << "\t\t\t" << " : " << sizeof(INT32) << " Bytes (" << (std::numeric_limits<INT32>::min)() << " ~ " << (std::numeric_limits<INT32>::max)() << ")" << endl;
	cout << "UINT32 " << "\t\t\t" << " : " << sizeof(UINT32) << " Bytes (" << (std::numeric_limits<UINT32>::min)() << " ~ " << (std::numeric_limits<UINT32>::max)() << ")" << endl;
	cout << "INT64 " << "\t\t\t" << " : " << sizeof(INT64) << " Bytes (" << (std::numeric_limits<INT64>::min)() << " ~ " << (std::numeric_limits<INT64>::max)() << ")" << endl;
	cout << "UINT64 " << "\t\t\t" << " : " << sizeof(UINT64) << " Bytes (" << (std::numeric_limits<UINT64>::min)() << " ~ " << (std::numeric_limits<UINT64>::max)() << ")" << endl;
	cout << "LONG32 " << "\t\t\t" << " : " << sizeof(LONG32) << " Bytes (" << (std::numeric_limits<LONG32>::min)() << " ~ " << (std::numeric_limits<LONG32>::max)() << ")" << endl;
	cout << "ULONG32 " << "\t\t" << " : " << sizeof(ULONG32) << " Bytes (" << (std::numeric_limits<ULONG32>::min)() << " ~ " << (std::numeric_limits<ULONG32>::max)() << ")" << endl;
	cout << "LONG64 " << "\t\t\t" << " : " << sizeof(LONG64) << " Bytes (" << (std::numeric_limits<LONG64>::min)() << " ~ " << (std::numeric_limits<LONG64>::max)() << ")" << endl;
	cout << "ULONG64 " << "\t\t" << " : " << sizeof(ULONG64) << " Bytes (" << (std::numeric_limits<ULONG64>::min)() << " ~ " << (std::numeric_limits<ULONG64>::max)() << ")" << endl;
}

//***************************************************************************
//
int main()
{
	//UseConstValueToCPP();
	//UseNumericLimitsToCPP();
	UseNumericLimitsToWindows();

  	return 0;
}

