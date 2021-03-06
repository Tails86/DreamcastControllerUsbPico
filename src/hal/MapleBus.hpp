#ifndef __MAPLE_BUS_H__
#define __MAPLE_BUS_H__

#include "MapleBusInterface.hpp"
#include "pico/stdlib.h"
#include "hardware/structs/systick.h"
#include "hardware/dma.h"
#include "configuration.h"
#include "utils.h"
#include "maple.pio.h"
#include "hardware/pio.h"

//! Handles communication over Maple Bus. This class is currently only setup to handle communication
//! from a host which initiates communication. This can easily be modified to handle communication
//! for a device, but that is not a use-case of this project.
//!
//! If this is ever modified to be a device, keep in mind that the maple_in state machine doesn't
//! sample the full end sequence. The application side should wait a sufficient amount of time
//! after bus goes neutral before responding in that case. Waiting for neutral bus is enough anyway.
//!
//! @warning this class is not "thread safe" - it should only be used by 1 core.
class MapleBus : public MapleBusInterface
{
    public:
        //! Maple Bus constructor
        //! @param[in] pinA  GPIO index for pin A. The very next GPIO will be designated as pin B.
        //! @param[in] senderAddr  The address of this device
        MapleBus(uint32_t pinA, uint8_t senderAddr);

        //! Writes a command to the Maple Bus where the sender address is what was given in the
        //! constructor.
        //! @param[in] command  The command byte - should be a value in Command enumeration
        //! @param[in] recipientAddr  The address of the device receiving this command
        //! @param[in] payload  The payload words to send
        //! @param[in] len  Number of words in payload
        //! @param[in] expectResponse  Set to true in order to start receive after send is complete
        //! @param[in] readTimeoutUs  When response is expected, the receive timeout in microseconds
        //! @returns true iff the bus was "open" and send has started
        bool write(uint8_t command,
                   uint8_t recipientAddr,
                   const uint32_t* payload,
                   uint8_t len,
                   bool expectResponse,
                   uint32_t readTimeoutUs=DEFAULT_MAPLE_READ_TIMEOUT_US);

        //! Writes a command with the given custom frame word. The internal sender address is
        //! ignored and instead the given frame word is sent verbatim.
        //! @param[in] frameWord  The first word to put out on the bus
        //! @param[in] payload  The payload words to send
        //! @param[in] len  Number of words in payload
        //! @param[in] expectResponse  Set to true in order to start receive after send is complete
        //! @param[in] readTimeoutUs  When response is expected, the receive timeout in microseconds
        //! @returns true iff the bus was "open" and send has started
        bool write(uint32_t frameWord,
                   const uint32_t* payload,
                   uint8_t len,
                   bool expectResponse,
                   uint32_t readTimeoutUs=DEFAULT_MAPLE_READ_TIMEOUT_US);

        //! Writes a command with the given words. The internal sender address is ignored and
        //! instead the given words are sent verbatim.
        //! @param[in] words  All words to send
        //! @param[in] len  Number of words in words (must be at least 1)
        //! @param[in] expectResponse  Set to true in order to start receive after send is complete
        //! @param[in] readTimeoutUs  When response is expected, the receive timeout in microseconds
        //! @returns true iff the bus was "open" and send has started
        bool write(const uint32_t* words,
                   uint8_t len,
                   bool expectResponse,
                   uint32_t readTimeoutUs=DEFAULT_MAPLE_READ_TIMEOUT_US);

        //! Called from a PIO ISR when read has completed for this sender.
        void readIsr();

        //! Called from a PIO ISR when write has completed for this sender.
        void writeIsr();

        //! Retrieves the last valid set of data read.
        //! @param[out] len  The number of words received
        //! @param[out] newData  Set to true iff new data was received since the last call
        //! @returns a pointer to the bytes read.
        //! @warning this call should be serialized with calls to write() as those calls may change
        //!          the data in the underlying buffer which is returned.
        const uint32_t* getReadData(uint32_t& len, bool& newData);

        //! Processes timing events for the current time.
        //! @param[in] currentTimeUs  The current time to process for (0 to internally get time)
        void processEvents(uint64_t currentTimeUs=0);

        //! @returns true iff the bus is currently busy reading or writing.
        inline bool isBusy() { return mWriteInProgress || mReadInProgress; }

    private:
        //! Ensures that the bus is open and starts the write PIO state machine.
        bool writeInit();

        //! If new data is available and is valid, updates mLastValidRead.
        void updateLastValidReadBuffer();

        //! Swaps the endianness of a 32 bit word from the given source to the given destination
        //! while also computing a crc over the 4 bytes.
        //! @param[out] dest  destination word to write to
        //! @param[in] source  source word to read from
        //! @param[in,out] crc  crc word to start from and write to
        static inline void swapByteOrder(volatile uint32_t& dest, uint32_t source, uint8_t& crc)
        {
            const uint8_t* src = reinterpret_cast<uint8_t*>(&source);
            volatile uint8_t* dst = reinterpret_cast<volatile uint8_t*>(&dest) + 3;
            for (uint j = 0; j < 4; ++j, ++src, --dst)
            {
                *dst = *src;
                crc ^= *src;
            }
        }

        //! Initializes all interrupt service routines for all Maple Busses
        static void initIsrs();

    private:
        //! Pin A GPIO index for this bus
        const uint32_t mPinA;
        //! Pin B GPIO index for this bus
        const uint32_t mPinB;
        //! Pin A GPIO mask for this bus
        const uint32_t mMaskA;
        //! Pin B GPIO mask for this bus
        const uint32_t mMaskB;
        //! GPIO mask for all bits used by this bus
        const uint32_t mMaskAB;
        //! The address of this device
        const uint8_t mSenderAddr;
        //! The PIO state machine used for output by this bus
        const MapleOutStateMachine mSmOut;
        //! The PIO state machine index used for input by this bus
        const MapleInStateMachine mSmIn;
        //! The DMA channel used for writing by this bus
        const int mDmaWriteChannel;
        //! The DMA channel used for reading by this bus
        const int mDmaReadChannel;

        //! The output word buffer - 256 + 2 extra words for bit count and CRC
        volatile uint32_t mWriteBuffer[258];
        //! The input word buffer - 256 + 1 extra word for CRC
        volatile uint32_t mReadBuffer[257];
        //! Holds the last know valid read buffer (no CRC - that was validated)
        uint32_t mLastValidRead[256];
        //! Number of words stored in mLastValidRead, including the frame word
        uint32_t mLastValidReadLen;
        //! True iff mLastValidRead was updated since last call to getReadData()
        bool mNewDataAvailable;
        //! True iff mReadBuffer was updated since last call to updateLastValidReadBuffer()
        volatile bool mReadUpdated;
        //! True when write is currently in progress
        volatile bool mWriteInProgress;
        //! True if read should be started immediately after write has completed
        bool mExpectingResponse;
        //! True when read is currently in progress
        volatile bool mReadInProgress;
        //! The time at which the next timeout will occur
        volatile uint64_t mProcKillTime;
        //! True once receive is detected
        volatile bool mRxDetected;
        //! Receive timeout for the current expected response
        uint32_t mReadTimeoutUs;
};

#endif // __MAPLE_BUS_H__
