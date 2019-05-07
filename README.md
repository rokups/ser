ser - a serialization library
=============================

This is a proof of concept serialization library inspired by [cereal](https://uscilab.github.io/cereal/). Goals of this
library are:
* Write only one serialization function to support both serialization and deserialization of all supported formats.
* Support for multiple serialization formats
* Support for custom per-format type serialization
* Support for serialization of object references
* Cross-language support (C# primarily, through using SWIG)
* Speed is not a concern, user comfort is
* No rtti
