# Jëger
A regex engine.

### Syntax
The aim was to follow Vim's regex syntax. Esoteric special characters such as "\zs" are not implemented, however the just is supported.

```C
match_t * regex_match(const regex_t * const regex, const char * const string, const bool start_of_string);
```
Returns a sentinel terminated array of `match_t` objects.
The sentinel object is defined as `(match_t){ .position = -1, .width = -1, };`.

```C
bool is_sentinel(const match_t * const match);
```
This is the function you must check whether a `match_t` is a sentinel or not.
I.e. make this the break condition while looping the results.


| Symbol | Meaning (TODO: fill in) |
| :----: | :---------------------: |
| .      | |
| ?      | One or zero of the previous token |
| =      | Same as ? |
| *      | Any number of the previous token |
| +      | One or more of the previous token |
| \\<    | Start of word |
| \\>    | End of word |
| ^      | Start of string |
| \t     | Tab |
| \n     | New line |
| \b     | |
| \i     | |
| \I     | |
| \k     | |
| \K     | |
| \f     | |
| \F     | |
| \p     | |
| \P     | |
| \s     | |
| \d     | Digit char |
| \D     | Not digit char |
| \x     | Hex char|
| \X     | Not hex char |
| \o     | Octal char |
| \O     | Not octal char |
| \w     | Word char|
| \W     | Not word char|
| \h     | |
| \a     | Ascii letter |
| \l     | Lowercase ascii letter |
| \L     | Not (lowercase ascii letter) |
| \u     | Uppercase ascii letter |
| \U     | Not (uppercase ascii letter) |
| [\<range\>]   | Any of \<range\> |
| [\^\<range\>] | None of \<range\> |
