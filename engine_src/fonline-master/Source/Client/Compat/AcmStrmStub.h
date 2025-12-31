#pragma once

#include "Common.h"

FO_BEGIN_NAMESPACE();

// Minimal Android-friendly stub for the Windows-only ACM unpacker used by SoundManager.
// The real implementation lives in `acmstrm.h`; this version simply reports failure
// so callers can fall back or skip ACM decoding on non-Windows platforms.
class CACMUnpacker
{
public:
    CACMUnpacker(void*, int32& channels, int32& freq, int32& samples)
    {
        channels = 0;
        freq = 0;
        samples = 0;
    }

    auto readAndDecompress(void*, size_t) -> size_t
    {
        return 0;
    }
};

FO_END_NAMESPACE();
