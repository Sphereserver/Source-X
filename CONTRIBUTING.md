# Coding Notes (add as you wish to standardize the coding for new contributors)

- Make sure you can compile and run the program before pushing a commit.
- Rebasing instead of pulling the project is a better practice to avoid unnecessary "merge branch master" commits.
- Removing/Changing/Adding anything that was working in one way for years should be followed by an ini setting if the changes
  cannot be replicated from script to keep some backwards compatibility.
- Comment your code, add information about its logic. It's very important since it helps others to understand your work.
- Be sure to use Sphere's custom datatypes and the string formatting macros described in src/common/datatypes.h.
- When casting numeric data types, always prefer C-style casts, like `(int)`, to C++ `static_cast<int>()`. It's way more concise.
- Be wary that in SphereScript unsigned values does not exist, all numbers are considered signed, and that 64 bits integers meant
  to be printed to or retrieved by scripts should always be signed.
- Don't use "long" except if you know why do you actually need it. Always prefer "int" or "llong".
  Use fixed width variables only for values that need to fit a limited range.
- For strings, use pointers:  
  to "char" for strings that should always have ASCII encoding;  
  to "tchar" for strings that may be ASCII or Unicode, depending on compilation settings (more info in "datatypes.h");  
  to "wchar" for string that should always have Unicode encoding.

## Naming Conventions

These are meant to be applied to new code and, if there's some old code not following them, it would be nice to update it.

- Pointer variables should have as first prefix "p".
- Unsigned variables should have as first (or second to "p") prefix "u".
- Boolean variables should have the prefix "f" (it stands for flag).
- Classes need to have the first letter uppercase and the prefix "C".
- Private or protected methods (functions) and members (variables) of a class or struct need to have the prefix "\_". This is a new convention, the old one used the "m\_" prefix for the members.
- Constants (static const class members, to be preferred to preprocessor macros) should have the prefix "k".
- After the prefix, the descriptive name should begin with an upper letter.

**Variables meant to hold numerical values:**

- For char, short, int, long, llong, use the prefix: "i" (stands for integer).
- For byte, word and dword use respectively the prefixes: "b", "w", "dw". Do not add the unsigned prefix.
- For float and double, use the prefix: "r" (stands for real number).

**Variables meant to hold characters (also strings):**

- For char, wchar, tchar use respectively the prefixes "c", "wc", "tc".
- When handling strings, "lpstr", "lpcstr", "lpwstr", "lpcwstr", "lptstr", "lpctstr" data types are preferred aliases.  
  You'll find a lot of "psz" prefixes for strings: the reason is that in the past Sphere coders wanted to be consistent with Microsoft's Hungarian Notation.  
  The correct and up-to-date notation is "pc" for lpstr/lpcstr (which are respectively `char*` and `const char*`), "pwc" (`wchar*` and `const wchar*`),
  "ptc" for lptstr/lpctstr (`tchar*` and `const tchar*`).  
  Use the "s" or "ps" (if pointer) when using `CString` or `std::string`. Always prefer `CString` over `std::string`, unless in your case there are obvious advantages for using the latter.

### Examples:

+ Class or Struct: "CChar".
+ Class internal variable, signed integer: "_iAmount".
+ Tchar pointer: "ptcName".
+ Dword: "dwUID".

## Coding Style Conventions

- Indent with **spaces** of size 4.
- Use the Allman indentation style:
```cpp
while (x == y)  
{  
    something();  
    somethingelse();  
}
```
- Even if a single statement follows the if/else/while... clauses, use the brackets:
```cpp
if (fTrue)  
{  
    g_Log.EventWarn("True!\n");  
}
```
