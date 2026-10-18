# How to contribute

Thanks for taking the time to contribute! :heart:

The following is a set of guidelines for contributing to `FFmpegKit`!

## Project Resources

* [README](README.md) describes this fork, its build options and its licenses
* [Upstream wiki](https://github.com/arthenica/ffmpeg-kit/wiki) has the documentation of the original FFmpegKit
* [Issues](../../issues) is for bugs and feature requests
* [Discussions](../../discussions) is where questions go, when enabled

## Reporting Bugs

Bugs are tracked as [GitHub issues](../../issues). We have a `Bug report` issue 
template which includes all the fields we need to see to confirm a bug and work on it. Try to fill out all template
fields, especially the logs field and steps to reproduce the bug. Reproducing a bug is crucial to be able to fix it.

### FFmpeg Bugs

`FFmpegKit` does not modify the original `FFmpeg` source code. Therefore, if an `FFmpeg` feature or component is not 
working as expected, most probably that problem comes from `FFmpeg`. If you encounter those kind of errors, we expect 
you to install the desktop version of `FFmpeg` and test that feature or component there. If it fails on desktop too 
then it must be reported to [FFmpeg bug tracker](https://trac.ffmpeg.org/). If not, then it is an `FFmpegKit` bug. 
Create an issue and state that this bug doesn't exist on the `desktop` version of the same `FFmpeg` version.

## Feature Requests

Before creating a feature request, please check the existing issues and the scope of this fork (Android only).
Then create an issue, fill out the `Feature request` issue template and provide as many details as possible.

### External Library Requests

`FFmpeg` supports a long list of `external` libraries. In `FFmpegKit` we did our best to support most of them. 
However, there are still many libraries that cannot be used within `FFmpegKit`.

Unfortunately, cross compilation is a challenging process. Because of that, we don't take new external library 
requests. Though, we are open to PRs. If someone wants to contribute we'll be happy to review their changes that
enables another external library in `FFmpeg`. 

## Pull Requests

Although it is not mandatory, our suggestion is to first discuss the change you wish to make via an issue or a 
discussion. `FFmpegKit` is a complex project. There are many things that must be considered when implementing a
feature.


Ensure that your changes rely on official documented methods and test your changes on a device with the 16 KB page size and on a 4 KB one.

Please open pull requests against the `main` branch.

Note that PRs about styling and cosmetic changes will not be accepted.

Thanks!