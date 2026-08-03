#pragma once
enum class AudioResult
{
    Ok,
    AcmConvert, // MMRESULT
    BadFormat,
    WaveOut,
    File,       // NTSTATUS
    TooLarge,
};

struct AudioError
{
    AudioResult Result;
    int Error;

    constexpr AudioError(AudioResult r) noexcept : Result{ r }, Error{} {}
    constexpr AudioError(AudioResult r, std::integral auto e) noexcept
        : Result{ r }, Error{ (int)e } {}
};