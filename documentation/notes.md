# Rationale
now, as it turns out BF + Flex is unholy.
bf ']' requires moving backwards one way or another.

flex can in fact not do that in any documented way,
 theoretically it should be possible to fuck with
 the internal pointers using macros and constantly
 emptying the buffers, but that does not sound fun.

What does sound fun however, is writting a mini-flex
 which has only the most surface level features,
 BUT is fully hackable, embeded friedly with
 minimal memory requirements and a somewhat proper subset of flex.

Another usage could be to scan unconventional info,
 for example command line arguments (without UB).

# CLI
[ ] --debug (dump tables)
[X] --trace
[ ] --table=[static|switch]

# Syntax

```lex
    // definition section
%%
    // rule section
%%
    // code section
```

## Definitions
```
%option <option>
```
[ ] yylineno
[ ] table="[static|switch]"
[ ] alphabet="\<a\>-\<z\>"
[ ] 7bit/8bit (compatibility; alias to the above)
[ ] prefix="\<...\>"
[ ] start

## Rules
Only the following forms should be supported:
```lex
<rule>
    // or
<state>{
<rule>
}
```
