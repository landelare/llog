// Copyright © Laura Andelare.
//
// LLog is free software. It comes without any warranty, to the extent permitted
// by applicable law. You can redistribute it and/or modify it under the terms
// of the Do What The Fuck You Want To Public License, Version 2, as published
// by Sam Hocevar:
//
//            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//                    Version 2, December 2004
//
// Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
//
// Everyone is permitted to copy and distribute verbatim or modified
// copies of this license document, and changing it is allowed as long
// as the name is changed.
//
//            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
//
//  0. You just DO WHAT THE FUCK YOU WANT TO.

#pragma once

#include <concepts>
#include <format>
#include <string>
#include <type_traits>
#include <utility>
#include "Containers/EnumAsByte.h"
#include "Containers/StringConv.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CoreMiscDefines.h"

#if UE_BUILD_SHIPPING
#define LL_FILE_CVAR(...) static_assert(true)
#define LL_IF_DBG()       if constexpr (false)
#define LL_IF_WRN()       if constexpr (false)
#define LL_IF_ERR()       if constexpr (false)
#define LL_DBG_KEY(...)   static_assert(true)
#define LL_WRN_KEY(...)   static_assert(true)
#define LL_ERR_KEY(...)   static_assert(true)
#else
// Add to the top of a .cpp file to define a cvar controlling the macros there.
// The default debug level is set to 1 = warnings and errors only.
#define LL_FILE_CVAR(cvar) namespace { TAutoConsoleVariable<int>               \
    _thisFileDebugLevel(TEXT("LLog." #cvar), 1, TEXT(#cvar " debug level."),   \
                        ECVF_Cheat); } static_assert(true)

// Use as if() replacements to, e.g., debug draw based on the cvar.
// Suggested .clang-format: IfMacros: ["LL_IF_DBG", "LL_IF_WRN", "LL_IF_ERR"]
#define LL_IF_DBG() if (_thisFileDebugLevel.GetValueOnAnyThread() >= 2)
#define LL_IF_WRN() if (_thisFileDebugLevel.GetValueOnAnyThread() >= 1)
#define LL_IF_ERR() if (_thisFileDebugLevel.GetValueOnAnyThread() >= 0)

// Use these to spam log messages that replace the previous one of the same key.
// __LINE__ is a convenient key.
#define LL_DBG_KEY(key, message, ...) do { LL_IF_DBG() LLog::LogCore(          \
    LLog::ELogLevel::Debug  , key, L##message, ##__VA_ARGS__); } while (false)
#define LL_WRN_KEY(key, message, ...) do { LL_IF_WRN() LLog::LogCore(          \
    LLog::ELogLevel::Warning, key, L##message, ##__VA_ARGS__); } while (false)
#define LL_ERR_KEY(key, message, ...) do { LL_IF_ERR()  LLog::LogCore(         \
    LLog::ELogLevel::Error  , key, L##message, ##__VA_ARGS__); } while (false)
#endif

// Use these to Just Print Stuff™ without caring about keys.
#define LL_DBG(message, ...) LL_DBG_KEY((uint64)-1, message, ##__VA_ARGS__)
#define LL_WRN(message, ...) LL_WRN_KEY((uint64)-1, message, ##__VA_ARGS__)
#define LL_ERR(message, ...) LL_ERR_KEY((uint64)-1, message, ##__VA_ARGS__)

#pragma region Implementation

// Some versions of the MSSTL don't implement P2508R1, so here it goes:
#if __cpp_lib_format <= 202110L && defined(_MSVC_STL_VERSION)
namespace std
{
template<typename C, typename... A>
using basic_format_string = _Basic_format_string<C, A...>;
template<typename... A> using format_string = _Fmt_string<A...>;
template<typename... A> using wformat_string = _Fmt_wstring<A...>;
}
#endif

namespace LLog
{
enum class ELogLevel { Debug, Warning, Error };

template<typename T>
concept THasStdFormat = requires(std::formatter<std::decay_t<T>, wchar_t> fmt,
                                 std::wformat_parse_context pctx,
                                 std::wformat_context fctx)
{
    fmt.parse(pctx);
    fmt.format(std::declval<std::decay_t<T>>(), fctx);
};

template<typename T>
concept TIterable = requires(T t) { ++std::begin(t); std::begin(t) != std::end(t); }
                 || requires(T t) { ++begin(t); begin(t) != end(t); }; // ADL

template<typename T>
auto FormatArgument(T&& value) // std::format doesn't like if this returns decltype(auto)
{
    using V = std::decay_t<T>;

    // Block these magic enums before they would implicitly convert to something else
    static_assert(!std::same_as<V, EForceInit> && !std::same_as<V, EInPlace> &&
                  !std::same_as<V, ENoInit>, "Invalid type");

    // Early out for everything that std::format can handle
    if constexpr (THasStdFormat<T>)
        return std::forward<T>(value);
    // Unpack smart pointers and TObjectPtr
    else if constexpr (requires { std::forward<T>(value).operator->(); })
        return FormatArgument(std::forward<T>(value).operator->());
    else
    {
        // Get type and validity information for UObjects
        UClass* type = nullptr;
        bool valid = true;
        if constexpr (std::is_convertible_v<T, const UObject*>)
        {
            type = value ? static_cast<const UObject*>(value)->GetClass()
                         : std::remove_pointer_t<V>::StaticClass();
            valid = IsValid(value);
        }
        else if constexpr (std::is_convertible_v<T, const UObject&>)
        {
            type = static_cast<const UObject&>(value).GetClass();
            valid = IsValid(std::addressof(static_cast<const UObject&>(value)));
        }

        // Try to coerce the value into a FString somehow, respecting && overloads
        FString str(TEXT("?LLog error"));
        if ([&]() { if constexpr (std::is_pointer_v<V>) return value == nullptr;
                    else return false; }()) // The if constexpr must be wrapped
            str = TEXT("nullptr");
        else if constexpr (requires { str = std::forward<T>(value)->GetName(); })
            str = std::forward<T>(value)->GetName();
        else if constexpr (requires { str = std::forward<T>(value).GetName(); })
            str = std::forward<T>(value).GetName();
        else if constexpr (requires { str = std::forward<T>(value)->ToString(); })
            str = std::forward<T>(value)->ToString();
        else if constexpr (requires { str = std::forward<T>(value).ToString(); })
            str = std::forward<T>(value).ToString();
        else if constexpr (requires { str = std::forward<T>(value)->c_str(); })
            str = std::forward<T>(value)->c_str();
        else if constexpr (requires { str = std::forward<T>(value).c_str(); })
            str = std::forward<T>(value).c_str();
        else if constexpr (TIsUEnumClass<V>::Value) // won't work for legacy UENUMs
            str = StaticEnum<V>()->GetNameByValue(static_cast<int64>(value)).ToString();
        else if constexpr (TIsTEnumAsByte<V>::Value)
            str = StaticEnum<typename V::EnumType>()->GetNameByValue(value.GetIntValue()).ToString();
        else if constexpr (std::is_enum_v<V>) // Enums without a compile-time detectable UEnum
            str = LexToString(static_cast<int64>(value));
        else if constexpr (std::is_assignable_v<FString, T>)
            str = std::forward<T>(value);
        else if constexpr (std::is_convertible_v<T, FName>) // EName literals
            str = FName(std::forward<T>(value)).ToString();
        else if constexpr (requires { LexToString(std::forward<T>(value)); })
            str = LexToString(std::forward<T>(value));
        else if constexpr (TIterable<T>) // Collections
        {
            str = TEXT("[");
            for (auto&& i : std::forward<T>(value))
                str += std::format(L"{}, ", FormatArgument(std::forward<decltype(i)>(i))).c_str();
            if (str.EndsWith(TEXT(", ")))
                str.LeftChopInline(2);
            str += TEXT("]");
        }
        else if constexpr (TIsTuple_V<V>)
        {
            str = TEXT("{");
            std::forward<T>(value).ApplyBefore([&str]<typename... A>(A&&... args)
            {
                ((str += std::format(L"{}, ", FormatArgument(std::forward<A>(args))).c_str()), ...);
            });
            if (str.EndsWith(TEXT(", ")))
                str.LeftChopInline(2);
            str += TEXT("}");
        }
        else if constexpr (requires { std::tuple_size<V>::value; }) // Can't use tuple_size_v
        {
            str = TEXT("{");
            std::apply([&str]<typename... A>(A&&... args)
            {
                ((str += std::format(L"{}, ", FormatArgument(std::forward<A>(args))).c_str()), ...);
            }, std::forward<T>(value));
            if (str.EndsWith(TEXT(", ")))
                str.LeftChopInline(2);
            str += TEXT("}");
        }
        // else if constexpr (your stuff goes here) { }
        else
            static_assert(false && std::is_void_v<T>, "Unhandled type");

        // Prepend extra type and validity info for UObjects
        // UObject*& counts as a pointer
        if (type)
            str = (valid ? TEXT("(") : TEXT("(invalid ")) +
                  type->GetName() +
                  (std::is_pointer_v<V> ? TEXT("*)") : TEXT("&)")) + str;

        return std::wstring(StringCast<wchar_t>(*str).Get());
    }
}

template<typename T>
using TFormatted = decltype(FormatArgument(std::declval<T>()));

template<typename... A>
FString FormatMessage(std::wformat_string<TFormatted<A>...> format, A&&... args)
{
    return FString(std::format(std::move(format),
                               FormatArgument(std::forward<A>(args))...).c_str());
}

template<typename... A>
void LogCore(ELogLevel level, uint64 key,
             std::wformat_string<TFormatted<A>...> format, A&&... args)
{
    DEFINE_LOG_CATEGORY_STATIC(LLog, Display, Display);
    auto message = FormatMessage(std::move(format), std::forward<A>(args)...);
    float ttl; FColor color; FVector2D scale;
    switch (level)
    {
        case ELogLevel::Debug:
            UE_LOG(LLog, Display, TEXT("%s"), *message);
            ttl = 1; color = FColor::Cyan; scale = FVector2D(1); break;
        case ELogLevel::Warning:
            UE_LOG(LLog, Warning, TEXT("%s"), *message);
            ttl = 10; color = FColor::Orange; scale = FVector2D(1.25); break;
        case ELogLevel::Error:
        default:
            UE_LOG(LLog, Error, TEXT("%s"), *message);
            ttl = 60; color = FColor::Red; scale = FVector2D(2); break;
    }
    if (GEngine) [[likely]]
        GEngine->AddOnScreenDebugMessage(key, ttl, color, message, true, scale);
}
}

#pragma endregion
