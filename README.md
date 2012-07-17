PyHashmap
==========

This is a simple python wrapper around google's sparsehash and densemap
libraries, and it also uses google's city hash algorithm for fast
hashing.

This currently relies on the following libraries:

* [sparsehash](http://code.google.com/p/sparsehash/)
* [cityhash](http://code.google.com/p/cityhash/)
* [boost.python](www.boost.org/libs/python)

as well as a few other [boost](http://www.boost.org/) libraries. This
is currently fairly preliminary, and it's far from portable, but
hopefully that will come shortly.

