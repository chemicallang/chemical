public type WCHAR = u16;

public type PDWORD = *mut DWORD;
public type LPWORD = *mut WORD;

public type PWCHAR = *mut WCHAR;
public type LPWCH = *mut WCHAR;
public type PWCH = *mut WCHAR;

public type LPCWCH = *WCHAR
public type PCWCH = *WCHAR

public type CCHAR = char;
public type LCID = DWORD;
public type PLCID = PDWORD;
public type LANGID = WORD;

/* Types use for passing & returning polymorphic values */
public type WPARAM = UINT_PTR;
public type LPARAM = LONG_PTR;
public type LRESULT = LONG_PTR;

public type PCNZWCH = *WCHAR;

public type PCHAR = *mut char
public type LPCH = *mut char
public type PCH = *mut char;

public type LPCCH = *char
public type PCCH = *char

public type LPBOOL = *mut BOOL;

// This is to be deprecated, please use the NLSVERSIONINFOEX
// structure below in the future.  The difference is that
// guidCustomversion is required to uniquely identify a sort
public struct _nlsversioninfo {		// Use NLSVERSIONINFOEX instead
    var dwNLSVersionInfoSize : DWORD;     // 12 bytes
    var dwNLSVersion : DWORD;
    var dwDefinedVersion : DWORD;         // Deprecated, use dwNLSVersion instead
};
public type NLSVERSIONINFO = _nlsversioninfo
public type LPNLSVERSIONINFO = *mut _nlsversioninfo

@dllimport
@stdcall
@extern
public func CompareStringEx(
    lpLocaleName : LPCWSTR,
    dwCmpFlags : DWORD,
    lpString1 : LPCWCH,
    cchCount1 : int,
    lpString2 : LPCWCH,
    cchCount2 : int,
    lpVersionInformation : LPNLSVERSIONINFO,
    lpReserved : LPVOID,
    lParam : LPARAM
) : int;

@dllimport
@stdcall
@extern
public func CompareStringOrdinal(
    lpString1 : LPCWCH,
    cchCount1 : int,
    lpString2 : LPCWCH,
    cchCount2 : int,
    bIgnoreCase : BOOL
) : int

@dllimport
@extern
@stdcall
public func CompareStringW(
    Locale : LCID,
    dwCmpFlags : DWORD,
    lpString1 : PCNZWCH,
    cchCount1 : int,
    lpString2 : PCNZWCH,
    cchCount2 : int
) : int

@dllimport
@extern
@stdcall
public func FoldStringW(
    dwMapFlags : DWORD,
    lpSrcStr : LPCWCH,
    cchSrc : int,
    lpDestStr : LPWSTR,
    cchDest : int
) : int

@dllimport
@extern
@stdcall
public func GetStringTypeExW(
    Locale : LCID,
    dwInfoType : DWORD,
    lpSrcStr : LPCWCH,
    cchSrc : int,
    lpCharType : LPWORD
) : BOOL

@dllimport
@extern
@stdcall
public func GetStringTypeW(
    dwInfoType : DWORD,
    lpSrcStr : LPCWCH,
    cchSrc : int,
    lpCharType : LPWORD
) : BOOL

//
//  NLS Code Page Dependent APIs.
//

@dllimport
@stdcall
@extern
public func MultiByteToWideChar(
    CodePage : UINT,
    dwFlags : DWORD,
    lpMultiByteStr : LPCCH,
    cbMultiByte : int,
    lpWideCharStr : LPWSTR,
    cchWideChar : int
) : int


@dllimport
@stdcall
@extern
public func WideCharToMultiByte(
    CodePage : UINT,
    dwFlags : DWORD,
    lpWideCharStr : LPCWCH,
    cchWideChar : int,
    lpMultiByteStr : LPSTR,
    cbMultiByte : int,
    lpDefaultChar : LPCCH,
    lpUsedDefaultChar : LPBOOL
) : int