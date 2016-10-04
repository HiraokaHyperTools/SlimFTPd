#ifndef _MBCS_STR_H_
#define _MBCS_STR_H_

#include <mbstring.h>

typedef std::basic_string<byte> tstring;

inline unsigned int __mbsnextc(const char *str) {
	return _mbsnextc(reinterpret_cast<const byte *>(str));
}

inline const char *__mbsdec(const char *start, const char *current, size_t count) {
	for (; count != 0; count--) {
		current = reinterpret_cast<const char *>(_mbsdec(reinterpret_cast<const byte *>(start), reinterpret_cast<const byte *>(current)));
	}
	return current;
}

inline char *__mbsdec(char *start, char *current, size_t count) {
	for (; count != 0; count--) {
		current = reinterpret_cast<char *>(_mbsdec(reinterpret_cast<byte *>(start), reinterpret_cast<byte *>(current)));
	}
	return current;
}

inline unsigned int __mbsprevc(const char *start, const char *current, size_t count) {
	return __mbsnextc(__mbsdec(start, current, count));
}

inline const char *__mbschr(const char *_Str, unsigned int _Ch) {
	return reinterpret_cast<const char *>(_mbschr(reinterpret_cast<const byte *>(_Str), _Ch));
}

inline char *__mbschr(char *_Str, unsigned int _Ch) {
	return reinterpret_cast<char *>(_mbschr(reinterpret_cast<byte *>(_Str), _Ch));
}

inline const char *__mbsrchr(const char *_Str, unsigned int _Ch) {
	return reinterpret_cast<const char *>(_mbsrchr(reinterpret_cast<const byte *>(_Str), _Ch));
}

inline char *__mbsrchr(char *_Str, unsigned int _Ch) {
	return reinterpret_cast<char *>(_mbsrchr(reinterpret_cast<byte *>(_Str), _Ch));
}

inline int __mbsicmp(const char *string1, const char *string2) {
	return _mbsicmp(reinterpret_cast<const byte *>(string1), reinterpret_cast<const byte *>(string2));
}

inline char *__mbsinc(const char *current) {
	return reinterpret_cast<char *>(_mbsinc(reinterpret_cast<const byte *>(current)));
}

inline size_t __strlen(const char *str) {
	return strlen(str);
}

inline size_t __mbslen(const char *str) {
	return _mbslen(reinterpret_cast<const byte *>(str));
}

inline size_t __mbscspn(const char *str, const char *strCharSet) {
	return _mbscspn(reinterpret_cast<const byte *>(str), reinterpret_cast<const byte *>(strCharSet));
}

inline int __mbscmp(const char *string1, const char *string2) {
	return _mbscmp(reinterpret_cast<const byte *>(string1), reinterpret_cast<const byte *>(string2));
}

inline int __mbsnicmp(const char *string1, const char *string2, size_t count) {
	return _mbsnicmp(reinterpret_cast<const byte *>(string1), reinterpret_cast<const byte *>(string2), count);
}

inline int __strnicmp(const char *string1, const char *string2, size_t count) {
	return _strnicmp(string1, string2, count);
}

inline errno_t __mbsncpy_s(char *strDest, size_t _DstSizeInBytes, const char *strSource, size_t count) {
	return _mbsncpy_s(reinterpret_cast<byte *>(strDest), _DstSizeInBytes, reinterpret_cast<const byte *>(strSource), count);
}

inline int __mbctolower(unsigned int i) {
	return _mbctolower(i);
}

#endif // _MBCS_STR_H_