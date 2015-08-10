#pragma once

#include <string>
#include <iostream>
#include "common/DefType.h"


bool ReadTo(std::istream& stream, const char* name);
bool Read(std::istream& stream, const char* name, int& data);
bool Read(std::istream& stream, const char* name, long& data);
bool Read(std::istream& stream, const char* name, float& data);
bool Read(std::istream& stream, const char* name, bool& data);
bool Read(std::istream& stream, const char* name, ulong& data);
bool Read(std::istream& stream, const char* name, short& data);
bool Read(std::istream& stream, const char* name, char& data);
bool Read(std::istream& stream, const char* name, uchar& data);
//bool Read(istream& stream, const char* name, float& data);
bool Read(std::istream& stream, const char* name, std::string& str);
//bool Read(istream& stream, const char* name, CString& str);
bool Read(std::istream& stream, const char* name, char *str);

bool Write(std::ostream& stream, const char* name, char* str);
bool Write(std::ostream& stream, const char* name, std::string& str);
bool Write(std::ostream& stream, const char* name, int& data);
bool Write(std::ostream& stream, const char* name, long& data);
bool Write(std::ostream& stream, const char* name, bool& data);
bool Write(std::ostream& stream, const char* name, float& data);
bool Write(std::ostream& stream, const char* name, ulong& data);
bool Write(std::ostream& stream, const char* name, short& data);
bool Write(std::ostream& stream, const char* name, char& data);
bool Write(std::ostream& stream, const char* name, uchar& data);
//bool Write(ostream& stream, const char* name, CString& str);

