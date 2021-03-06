#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

// Adjust the CPU clock frequency here
#define CPU_FREQ_KHZ 133000

// Amount of time in nanoseconds at which each bit transmits (value should be divisible by 3)
// 480 ns achieves just over 2 mbps, just as the Dreamcast does
#define MAPLE_NS_PER_BIT 480

// The minimum amount of time we check for an open line before taking control of it
#define MAPLE_OPEN_LINE_CHECK_TIME_US 10

// Added percentage on top of the expected completion time
#define MAPLE_WRITE_TIMEOUT_EXTRA_PERCENT 20

// Maximum amount of time waiting for the beginning of a response when one is expected
#define MAPLE_RESPONSE_TIMEOUT_US 500

// Default maximum amount of time to spend trying to read on the maple bus
// 4000 us accommodates the maximum number of words (256) at 2 mbps
#define DEFAULT_MAPLE_READ_TIMEOUT_US 4000

#endif // __CONFIGURATION_H__
