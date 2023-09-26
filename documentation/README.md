# Abstraction
	     +---------------------+
	     |                     |
	     |                     |
	     |   State register    |
	     |                     |
	     |                     |
	     +---------------------+


	+---------------------------------+
	|     State transition table      |
	+---------------------------------+


	+---------------------------------+
	|    Fallback transition table    |
	+---------------------------------+

---

State transition table look up

 + success --> continue

 + fail    --> look up fallback table
  * success --> continue
  * fail    --> return

EOS ? --> look up fallback table

 + success --> is 0 width?
  * success --> continue
  * fail    --> return 

 + fail    --> return

---
# Legend

|      | Start | End |
| :--: | :---: | :-: |
| Line |   SOS | EOS |
| Word |   SOW | EOW |

---
##### HALT\_AND\_CATCH\_FIRE
H&C is a special state signalling that we have hit a dead end.
The reason why need it and we cant just instanly quick is backtracking.

---
##### [^example]
This is a negative range.
```
let myNegativeRange = {'e', 'x', 'a', 'm', 'p', 'l'}
```
None of the characters in $myNegativeRange must be accepted.
The way this is a compiled is that we first hook all chars in $myNegativeRange to H&C,
then define an OFFSHOOT of width 1.
Put differently:
if we read something illegal we abort this branch,
if what we read was not illegal, we deduct that it must have been legal and we continue.

Handling "negatives" this way allows us to be "alphabet agnostic" in a sense.
Many implementations will presume ASCII, with its fixed 7/8 bit width
and create look up tables.
Which is fast and cute, but this strategy becomes a giant memory hog
if we ever wanted to use it on, say UTF-8 (from 256 te/c (table entries per char) to 4'294'967'295 te/c).


#### .
This is the dot operator.
It matches any 1 char.

Similar how negative ranges are implemented,
it takes advantage of the fallback table.
It simply ignores the state transition table and rather unconditionally hooks itself to the next state.


#### ^
This is the carrot operator.
It matches the SOS (start of the string).

For explanation purposes multilining (match '\n') is irrelevant.
That behaves just like a literal.

What is more interesting is how SOS is recognized.
Since `regex_assert()` is recursive the current state is continuesly passed along,
however at out first frame, it's not just always 0.
`regex_match()` decides depending on the current position of the string.
Basically we have the first 2 states (0, 1) reserved and always missing from the state transmission table.
	+ 0 -  SOS
	+ 1 - !SOS
Normally both are _hooked_ to state 2,
and we pretend nothing has ever happened.
But when carrot operator is compiled, it sets a special compiler flag FORCE\_START\_OF\_STRING,
which forbids the hooking of state 1 to 2,
therefor when `regex_match()` calls from, say position 2,
it passes in 1 as the starting state,
no state transition table entry will be found since thats forbidden to begin with,
no jumps are found(!),
the machine checks whether the current state (1) is the accepting state (>=2)
and finally returns failiour.


#### \<
This is the SOW operator.
SOW must match:
```
^myword
[^\h]myword
```
Not only that, this combination is key,
either it has to be the start of the string
or there has to be at least something which is not a symbol char.
With out the last condition "eexample" would match "\\\<exaple\\\>"
as the iteration of `regex_match()` reaches "example".

From a more practical perspective:
``` C
\<myword\>
// Must match
"myword"
" myword"
```
