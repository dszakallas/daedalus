# Daedalus

Learning the Metal graphics API through reimplementing Computer Graphics uni course homework assignments.

Supports only macOS at the moment. I vendored the [metal-cpp](https://developer.apple.com/metal/cpp/) and metal-cpp-extensions convenience libs provided by Apple. 
I had to modify the latter by adding some bridge casts to be includable in Objective-C++.

To build, you'll need Xcode >=14.
