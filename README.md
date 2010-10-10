
<A name="toc1-3" title="ZFL - ØMQ Function Library" />
# ZFL - ØMQ Function Library

<A name="toc2-6" title="Contents" />
## Contents


**<a href="#toc2-11">Overview</a>**
&emsp;<a href="#toc3-14">Scope and Goals</a>
&emsp;<a href="#toc3-23">Ownership and License</a>
&emsp;<a href="#toc3-30">Contributing</a>

**<a href="#toc2-41">Using ZFL</a>**
&emsp;<a href="#toc3-44">Building and installing</a>
&emsp;<a href="#toc3-64">Using ZFL</a>
&emsp;<a href="#toc3-75">Class Model</a>
&emsp;<a href="#toc3-108">ZFL Classes</a>

**<a href="#toc2-121">Under the Hood</a>**
&emsp;<a href="#toc3-124">Adding a New Class</a>
&emsp;<a href="#toc3-135">Memory leak testing</a>
&emsp;<a href="#toc3-148">This Document</a>

<A name="toc2-11" title="Overview" />
## Overview

<A name="toc3-14" title="Scope and Goals" />
### Scope and Goals

ZFL is the ØMQ Function Library, a thin portability & function library for ZeroMQ applications in C/C++.  It is written as clear readable C classes, portable to all ØMQ platforms, and licensed under the LGPL.

The main goal is to allow the construction of industrial-scale ØMQ services and devices that integrate properly with the operating system, while staying 100% portable.  ZFL acts as a primary abstraction layer on top of the ØMQ API, mainly for C applications but also for other languages.  ZFL is inspired by the iMatix Standard Function Library (SFL), and borrows liberally from it.  See http://legacy.imatix.com/html/sfl/.

ZFL is meant to be lightweight, consistent, class-based, minimalistic, highly efficient, and aimed at making it faster and easier to develop realistic, secure, and portable ØMQ devices and applications.

<A name="toc3-23" title="Ownership and License" />
### Ownership and License

ZFL is maintained by Pieter Hintjens.  Its authors are listed in the AUTHORS file.  It is held by the zeromq organization at github.com.

The authors of ZFL grant you free use of this software under the terms of the GNU Lesser General Public License (LGPL). For details see the files `COPYING` and `COPYING.LESSER` in this directory.

<A name="toc3-30" title="Contributing" />
### Contributing

To submit an issue use the [issue tracker](http://github.com/zeromq/zfl/issues).  All discussion happens on the [zeromq-dev](zeromq-dev@lists.zeromq.org) list or #zeromq IRC channel at irc.freenode.net.

The proper way to submit patches is to clone this repository, make your changes, and use git to create a patch.  See http://www.zeromq.org/docs:contributing.  All contributors are listed in AUTHORS.

All classes are maintained by a single person, who is the responsible editor for that class and who is named in the header as such.  This is usually the originator of the class.  When several people collaborate on a class, one single person is always the lead maintainer and the one to blame when it breaks.

The general rule is, if you contribute code to ZFL you must be willing to maintain it as long as there are users of it.  Code with no active maintainer will in general be deprecated and/or removed.

<A name="toc2-41" title="Using ZFL" />
## Using ZFL

<A name="toc3-44" title="Building and installing" />
### Building and installing

ZFL uses autotools for packaging.  To build from git (all example commands are for Linux):

    git clone git://github.com/zeromq/zfl.git
    cd zfl
    sh autogen.sh
    ./configure
    make all
    sudo make install
    sudo ldconfig

You will need the pkg-config, libtool, and autoreconf packages.  Set the LD_LIBRARY_PATH to /usr/local/libs unless you install elsewhere.

After building, you can run the ZFL selftests:

    cd src
    ./zfl_selftest

<A name="toc3-64" title="Using ZFL" />
### Using ZFL

Include `zfl.h` in your application and link with libzfl.  Here is a typical gcc link command:

    gcc -lzfl -lzmq myapp.c -o myapp

You should read `zfl.h`.  This file includes `zmq.h` and the system header files that typical ØMQ applications will need.  The provided 'c' shell script lets you write simple portable build scripts:

    c -lzfl -lzmq -l myapp

<A name="toc3-75" title="Class Model" />
### Class Model

ZFL consists of classes, each class consisting of a .h and a .c.  Classes may depend on other classes.

`zfl.h` includes all classes header files, all the time.  For the user, ZFL forms one single package.  All classes start by including `zfl.h`.  All applications that use ZFL start by including `zfl.h`.  `zfl.h` also defines a limited number of small, useful macros and typedefs that have proven useful for writing clearer C code.

The canonical example for ZFL style is the zfl_base class, which defines the template for all other classes.  The nomenclature for all classes is consistent.  We use zfl_base as an example:

* Source files: zfl_base.c, zfl_base.h
* Methods: zfl_base_test, zfl_base_print, ...

All classes are based on a flat C class system and follow these rules:

* Class typedef: `zfl_base_t`
* Constructor: `zfl_base_new`
* Destructor: `zfl_base_destroy`
* Property methods: `zfl_base_property_set`, `zfl_base_property`
* Class structures are private (defined in the .c source but not the .h)
* Properties are accessed only via methods named as described above.
* In the class source code the object is always called `self`.
* The constructor may take arbitrary arguments, and returns NULL on failure, or a new object.
* The destructor takes a pointer to an object reference and nullifies it.

Return values for methods are:

* For methods that return an object reference, either the reference, or NULL on failure.
* For methods that signal success/failure, a return value of 0 means sucess, -1 failure.

Private/static functions in a class are named `s_functionname` and are not exported via the header file.

All classes have a test method called `zfl_classname_test`.

<A name="toc3-108" title="ZFL Classes" />
### ZFL Classes

These are the existing ZFL classes:

* zfl_base - base class, provides no functionality.
* zfl_blob - work with length-specified binary objects.
* zfl_config - work with configured ØMQ sockets.
* zfl_msg - work with ØMQ multipart messages.
* zfl_tree - work with hierarchical tree.
* zfl_tree_json - load a zfl_tree from JSON data.
* zfl_tree_zpl - load a zfl_tree from http://rfc.zeromq.org/spec:4 data.

<A name="toc2-121" title="Under the Hood" />
## Under the Hood

<A name="toc3-124" title="Adding a New Class" />
### Adding a New Class

If you define a new ZFL class `myclass` you need to:

* Write the `zfl_myclass.c` and `zfl_myclass.h` source files, in `zfl/src` and `zfl/include` respectively.
* Add`#include <zfl_myclass.h>` to `zfl/include/zfl.h`.
* Add the myclass header and test call to `src/zfl_selftest.c`.
* Add a reference documentation to 'doc/zfl_myclass.txt'.
* Add myclass to 'src/Makefile.am` and `doc/Makefile.am`.

<A name="toc3-135" title="Memory leak testing" />
### Memory leak testing

To test against memory leaks we use the mtrace tool under Linux.  The zfl_selftest.c program calls MALLOC_TRACE, which zfl_prelude.h sets to mtrace() under Linux.  This is how we build and run the selftests with mtrace:

    #  Run selftests and check memory
    gcc -g -o zfl_selftest zfl*.c -lzmq
    export MALLOC_TRACE=mtrace.txt
    zfl_selftest -v
    mtrace zfl_selftest mtrace.txt

Note that mtrace is not threadsafe and will not work consistently in multithreaded applications or test cases.  All test cases should therefore be single-threaded.

<A name="toc3-148" title="This Document" />
### This Document

This document is originally at README.txt and is built using [gitdown](http://github.com/imatix/gitdown).
