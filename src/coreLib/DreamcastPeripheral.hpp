#pragma once

#include <stdint.h>
#include "MapleBusInterface.hpp"

//! Base class for a connected Dreamcast peripheral
class DreamcastPeripheral
{
    public:
        //! Constructor
        //! @param[in] addr  This peripheral's address (mask bit)
        //! @param[in] bus  The bus that this peripheral is connected to
        //! @param[in] playerIndex  Player index of this peripheral [0,3]
        DreamcastPeripheral(uint8_t addr, MapleBusInterface& bus, uint32_t playerIndex) :
            mBus(bus), mPlayerIndex(playerIndex), mAddr(addr)
        {}

        //! Virtual destructor
        virtual ~DreamcastPeripheral() {}

        //! Handles incoming data destined for this device
        //! @param[in] len  Number of words in payload
        //! @param[in] cmd  The received command
        //! @param[in] payload  Payload data associated with the command
        //! @returns true iff the data was handled
        virtual bool handleData(uint8_t len,
                                uint8_t cmd,
                                const uint32_t *payload) = 0;

        //! @param[in] subPeripheralIndex  Sub peripheral index [0,4]
        //! @returns the sub peripheral mask for the given sub peripheral index
        static inline uint8_t subPeripheralMask(int32_t subPeripheralIndex)
        {
            return SUB_PERIPHERAL_ADDR_START_MASK << subPeripheralIndex;
        }

        //! @param[in] subPeripheralMask  Sub peripheral mask
        //! @returns the index of the first sub peripheral mask that was matched
        static inline int32_t subPeripheralIndex(uint8_t subPeripheralMask)
        {
            uint8_t mask = SUB_PERIPHERAL_ADDR_START_MASK;
            for (uint32_t i = 0; i < MAX_SUB_PERIPHERALS; ++i, mask<<=1)
            {
                if (subPeripheralMask & mask)
                {
                    return i;
                }
            }
            return -1;
        }

        //! Get recipient address for a peripheral with given player index and address
        //! @param[in] playerIndex  Player index of peripheral [0,3]
        //! @param[in] addr  Peripheral's address (mask bit)
        //! @returns recipient address
        static inline uint8_t getRecipientAddress(uint32_t playerIndex, uint8_t addr)
        {
            return (playerIndex << 6) | addr;
        }

        //! @returns recipient address for this peripheral
        inline uint8_t getRecipientAddress() { return getRecipientAddress(mPlayerIndex, mAddr); }

        //! The task that DreamcastNode yields control to after this peripheral is detected
        //! @param[in] currentTimeUs  The current time in microseconds
        //! @returns true iff still connected
        virtual bool task(uint64_t currentTimeUs) = 0;

    public:
        //! The maximum number of sub peripherals that a main peripheral can handle
        static const uint32_t MAX_SUB_PERIPHERALS = 5;
        //! Main peripheral address mask
        static const uint8_t MAIN_PERIPHERAL_ADDR_MASK = 0x20;
        //! The first sub peripheral address
        static const uint8_t SUB_PERIPHERAL_ADDR_START_MASK = 0x01;

    protected:
        //! The bus that this peripheral is connected to
        MapleBusInterface& mBus;
        //! Player index of this peripheral [0,3]
        const uint32_t mPlayerIndex;
        //! Address of this device
        const uint8_t mAddr;
};
