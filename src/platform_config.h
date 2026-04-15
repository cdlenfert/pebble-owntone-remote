#pragma once

// Platform-specific capacity and buffer limits.
// All feature code should use these constants instead of inline #ifdefs
// scattered across source files.

#if defined(PBL_PLATFORM_APLITE)
  // Aplite: ~24KB available RAM after OS overhead.
  // Smaller caps reduce peak heap pressure.
  #define PLATFORM_INBOX_SIZE    1536
  #define PLATFORM_OUTBOX_SIZE   256
#else
  #define PLATFORM_INBOX_SIZE    2048
  #define PLATFORM_OUTBOX_SIZE   512
#endif

#define MAX_QUEUE_ITEMS        10

// Shared limits (same on all platforms)
#define MAX_OUTPUTS       8
#define MAX_FAVORITES     30
#define MAX_RESULTS       8
#define MAX_STRING_LENGTH 64
