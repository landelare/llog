# LLog

An opinionated, header-only debugging helper and logging library for Unreal.
This library is intended to be owned instead of being treated as third-party code.
There are no releases or versions.
Don't even fork or clone this repository.
Download the .h, copy it into your project, rename things, make it yours!

# Feature tour

* Less than 250 lines of self-documenting code, including the license.
* Three hardcoded debug levels: debug, warning, error.
* Per-file runtime control of debug levels with CVars.
* Logging macros using the well-documented, standard std::format syntax.
* if macros to keep your debug draws in sync with the logs.
* Everything gets written to UE_LOG and AddOnScreenDebugMessage.
* Support for Unreal types: you can log FStrings without the operator\* hack,
  FVector is supported directly, UObjects are logged as their class and name, etc.
* Supports Unreal and STL-style containers and tuples, with arbitrary nesting.
* Adding your own handlers is as simple as continuing an else if constexpr chain.
* Compile-time format string checking, courtesy of std::format itself.
* Bad runtime performance. Still better than BP and it gets compiled to nothing
  in Shipping anyway.
* Incompatible with unity builds.
