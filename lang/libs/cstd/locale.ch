
/**
 * The struct lconv contains numeric and monetary formatting rules as defined by a C locale. Objects of this struct may be obtained with localeconv. The members of lconv are values of type char and of type char*. Each char* member except decimal_point may be pointing at a null character (that is, at an empty C-string). The members of type char are all non-negative numbers, any of which may be CHAR_MAX if the corresponding value is not available in the current C locale.
 */
public struct lconv {

    // the character used as the decimal point
    const decimal_point : *char

    // the character used to separate groups of digits before the decimal point
    const thousands_sep : *char

    // a string whose elements indicate the sizes of digit groups
    const grouping : *char

    // the character used as the decimal point
    const mon_decimal_point : *char

    // the character used to separate groups of digits before the decimal point
    const mon_thousands_sep : *char

    // a string whose elements indicate the sizes of digit groups
    const mon_grouping : *char

    // a string used to indicate non-negative monetary quantity
    const positive_sign : *char

    // a string used to indicate negative monetary quantity
    const negative_sign : *char

    // the symbol used for currency in the current C locale
    const currency_symbol : *char

    // the number of digits after the decimal point to display in a monetary quantity
    const frac_digits : char

    // 1 if currency_symbol is placed before non-negative value, 0 if after
    const p_cs_precedes : char

    // 1 if currency_symbol is placed before negative value, 0 if after
    const n_cs_precedes : char

    // indicates the separation of currency_symbol, positive_sign, and the non-negative monetary value
    const p_sep_by_space : char

    // indicates the separation of currency_symbol, negative_sign, and the negative monetary value
    const n_sep_by_space : char

    // indicates the position of positive_sign in a non-negative monetary value
    const p_sign_posn : char

    // indicates the position of negative_sign in a negative monetary value
    const n_sign_posn : char

    // the string used as international currency name in the current C locale
    const int_curr_symbol : *char

    // the number of digits after the decimal point to display in an international monetary quantity
    const int_frac_digits : char

    // 1 if int_curr_symbol is placed before non-negative international monetary value, ​0​ if after
    const int_p_cs_precedes : char

    // 1 if int_curr_symbol is placed before negative international monetary value, ​0​ if after
    const int_n_cs_precedes : char

    // indicates the separation of int_curr_symbol, positive_sign, and the non-negative international monetary value
    const int_p_sep_by_space : char

    // indicates the separation of int_curr_symbol, negative_sign, and the negative international monetary value
    const int_n_sep_by_space : char

    // indicates the position of positive_sign in a non-negative international monetary value
    const int_p_sign_posn : char

    // indicates the position of negative_sign in a negative international monetary value
    const int_n_sign_posn : char

}

/**
 * The setlocale function installs the specified system locale or its portion as the new C locale. The modifications remain in effect and influences the execution of all locale-sensitive C library functions until the next call to setlocale. If locale is a null pointer, setlocale queries the current C locale without modifying it.
 * @param category	-	locale category identifier, one of the LC_xxx macros. May be null.
 * @param locale	-	system-specific locale identifier. Can be "" for the user-preferred locale or "C" for the minimal locale
 * @return pointer to a narrow null-terminated string identifying the C locale after applying the changes, if any, or null pointer on failure.
 * A copy of the returned string along with the category used in this call to setlocale may be used later in the program to restore the locale back to the state at the end of this call.
 */
public func setlocale(category : int, locale : *char) : *mut char

/**
 * The localeconv function obtains a pointer to a static object of type lconv, which represents numeric and monetary formatting rules of the current C locale.
 * @return pointer to the current lconv object.
 */
public func localeconv() : *lconv

/**
 * TODO these macros haven't been done
 *   #define LC_ALL      // implementation defined
 *   #define LC_COLLATE  // implementation defined
 *   #define LC_CTYPE    // implementation defined
 *   #define LC_MONETARY // implementation defined
 *   #define LC_NUMERIC  // implementation defined
 *   #define LC_TIME     // implementation defined
 */
