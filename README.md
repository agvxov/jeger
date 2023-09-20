# Jëger
A regex engine.

### Syntax
The aim was to follow Vim's regex syntax. Esoteric special characters such as "\zs" are not implemented, however the just is supported.

```C
match_t * regex_match(const regex_t * const regex, const char * const string, const bool start_of_string);
```
Returns a sentinel terminated array of `match_t` objects.
The sentinel object is defined as `(match_t){ .position = -1, .width = -1, };`.
The position and width of non-sentinel `match_t`s is guaranteed to be => 0.

| Symbol | Meaning (TODO: fill in) |
| :----: | :---------------------: |
| .      | |
| =      | |
| +      | |
| *      | |
| ?      | |
| \\<    | |
| \\>    | |
| ^      | |
| \t     | |
| \n     | |
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
| \d     | |
| \D     | |
| \x     | |
| \X     | |
| \o     | |
| \O     | |
| \w     | |
| \W     | |
| \h     | |
| \a     | |
| \l     | |
| \L     | |
| \u     | |
| \U     | |
| [\<range\>]   | |
| [\^\<range\>] | |
