# LLog

A highly-opinionated, header-only debugging helper and logging library for
Unreal.
This library is intended to be owned instead of being treated as third-party code.
There are no releases or versions.
Don't even fork or clone this repository.
Download the .h, copy it into your project, change things, make it yours!

# Feature tour

* Less than 250 lines of code, including the license header and embedded
  documentation.
* Zero overhead in Shipping (of course).
* Three lovingly-hardcoded logging levels: debug, warning, error.
* Easy per-file runtime control of log verbosity with CVars.
* Logging macros using the well-documented, standard std::format syntax.
* Compile-time format string validation, courtesy of std::format itself.
* `if` macros to easily turn debug draws on or off in sync with the same CVars
  that control logging.
* Everything gets written to UE_LOG and AddOnScreenDebugMessage.
* Better support for Unreal types than Unreal itself: no need for operator\*
  hacks just to log a FString, FVectors work without .ToString(), UENUMs are
  displayed as their names instead of their numeric values, smart pointers are
  unwrapped, UObjects are shown as their name with added type and validity
  information, and so on.
* Unreal and STL-style containers and tuples are both supported, with arbitrary
  nesting. Ever wanted to log a
  TArray\<std::map\<FString,&nbsp;std::deque\<TTuple\<int,&nbsp;std::tuple\<TWeakObjectPtr\<AActor\>\>\>\>\>\>&&?
  Me neither, but it works anyway.
* Adding your own types is as simple as
  [continuing an else if constexpr chain](LLog.h#L193) or using the official
  method of specializing std::formatter.
  Note that LLog uses wchar_t, not char.
* Incompatible with unity builds.
