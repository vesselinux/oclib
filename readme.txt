OCLIB: An Implementation of Online Codes

1 Description

OCLIB is a software library implementing a class of rateless error-correction codes called Online Codes. Online codes (OC) are especially suitable for encoding and trasmitting data over channels with an unknown or variable loss rate. OC were introduced by Petar Maymounkov in [1]. They are efficient error-correction codes, requiring $O(1)$ time to generate each encoding block and $O(n)$ time to decode an original message of length $n$. More detailed desrciption of Online codes and further references can be found at http://en.wikipedia.org/wiki/Online_codes .

[1] P. Maymounkov, Online codes, NYU Technical Report TR2002-833, 2002, pdos.csail.mit.edu/~petar/papers/maymounkov-online.pdf

2 Compilation

$ make oc

3 Example

By running the command 

$ ./oc alice.txt

on the file alice.txt, the latter is encoded using Online Codes and is "transmitted" over the loopback interface. The decoded file "received" on the other side is called alice.txt.dcd . If the two files are identical, the decoding was successful:

$ ls -l alice.txt alice.txt.dcd
-rw-r--r-- 1 foo foo 167545 Dec 20  2011 alice.txt
-rw-r--r-- 1 foo foo 167545 Jun 12 14:08 alice.txt.dcd
$ cmp alice.txt alice.txt.dcd
$

contact: un3_14qliar@yahoo.com
