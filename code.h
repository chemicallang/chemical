#pragma once


#include <ctype.h>
#include <assert.h>

class Code
{
    //
    // To facilitate the scanning, the character set is partitioned into
    // 8 classes using the array CODE. The classes are described below
    // together with some self-explanatory functions defined on CODE.
    //
    enum {
             LOG_BASE_SIZE       = 9,
             LOG_COMPLEMENT_SIZE = 7,
             BASE_SIZE           = 512,
             SLOT_SIZE           = 128,
             SLOT_MASK           = 127,

             NEWLINE_CODE        = 1,
             SPACE_CODE          = 2,
             BAD_CODE            = 3,
             DIGIT_CODE          = 4,
             LOWER_CODE          = 5,
             UPPER_CODE          = 6,
             OTHER_LETTER_CODE   = 7 /* accented letters in the range 128-255 */
         };

    static char code[256];

public:

    enum {
             NULL_CHAR = 0,
             CTL_Z = 26,
             HORIZONTAL_TAB = '\t',
             LINE_FEED = '\n'
         };


    static char ToUpper(char c) { return (IsLower(c) ? toupper(c) : c); }
    static char ToLower(char c) { return (IsUpper(c) ? tolower(c) : c); }

    //
    // VERIFY takes as argument a character string and checks whether or not each
    // character is a digit. If all are digits, then 1 is returned; if not, then
    // 0 is returned.
    //
    static bool verify(const char *item)
    {
       while (IsDigit(*item))
            item++;
        return (*item == '\0');
    }

    static bool verify(const char *item, int length)
    {
        int i;
        for (i = 0; i < length && IsDigit(item[i]); i++)
            ;
        return (i == length);
    }

    //
    // TRANSLATE takes as arguments a character array, which it folds to upper
    // to uppercase and returns.   
    //
    static char *translate(char *str, int len)
    {
        for (int i = 0; i < len; i++)
            str[i] = ToUpper(str[i]);
        return(str);
    }

    //
    // Compare two character strings s1 and s2 to check whether or not s2
    // is a substring of s1.  The string s2 is assumed to be in lowercase
    // and NULL terminated. However, s1 does not have to (indeed, may not
    // be NULL terminated.                                               
    //                                                                   
    // The test below may look awkward. For example, why not use:        
    //                  if (tolower(s1[i]) != s2[i])  ?                  
    // because tolower(ch) is sometimes implemented as (ch-'A'+'a') which
    // does not work when "ch" is already a lower case character.        
    //                                                                   
    static bool strxeq(const char *s1, const char *s2)
    {
        for (; *s2 != '\0'; s1++, s2++)
        {
            if (*s1 != *s2  && *s1 != toupper(*s2))
                return false;
        }

        return true;
    }

    //
    // Compare two character strings s1 and s2 to identify the longest
    // substring of s1 that matches a substring of s2.
    // The string s2 is assumed to be in lowercase and NULL terminated.
    // However, s1 does not have to (indeed, may not be NULL terminated.                                               
    //                                                                   
    // The test below may look awkward. For example, why not use:        
    //                  if (tolower(s1[i]) != s2[i])  ?                  
    // because tolower(ch) is sometimes implemented as (ch-'A'+'a') which
    // does not work when "ch" is already a lower case character.        
    //                                                                   
    static int strxsub(const char *s1, const char *s2)
    {
        int i;
        for (i = 0; s2[i] != '\0'; i++)
        {
            if (s1[i] != s2[i]  && s1[i] != toupper(s2[i]))
                break;
        }

        return i;
    }

    static inline bool IsNewline(int c) // \r characters are replaced by \x0a in read_input.
    {
        return code[(unsigned char) c] == NEWLINE_CODE; // '\x0a' or '\x0d'
    }

    static inline bool IsSpaceButNotNewline(char c)
    {
        return code[(unsigned char) c] == SPACE_CODE;
    }

    static inline bool IsSpace(int c)
    {
        return code[(unsigned char) c] <= SPACE_CODE;
    }

    static inline bool IsDigit(int c)
    {
        return code[(unsigned char) c] == DIGIT_CODE;
    }

    static inline bool IsHexDigit(int c)
    {
        return IsDigit(c) || (IsAlpha(c) && ToUpper(c) <= 'F');
    }

    static inline bool IsUpper(int c)
    {
        return code[(unsigned char) c] == UPPER_CODE;
    }

    static inline bool IsLower(int c)
    {
        return code[(unsigned char) c] == LOWER_CODE;
    }

    static inline bool IsAlpha(int c)
    {
        return code[(unsigned char) c] >= LOWER_CODE;
    }

    static inline bool IsAlnum(int c)
    {
        return code[(unsigned char) c] >= DIGIT_CODE;
    }

    static bool IsValidVariableName(const char *name)
    {
        if (IsAlpha(*name))
        {
            name++;
            while (IsAlnum(*name))
                name++;
            return *name == '\0';
        }
        return false;
    }
};

