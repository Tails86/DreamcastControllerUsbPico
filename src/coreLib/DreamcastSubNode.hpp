#pragma once

#include "DreamcastNode.hpp"

//! Handles communication for a Dreamcast sub node for a single bus.
class DreamcastSubNode : public DreamcastNode
{
    public:
        //! Constructor
        //! @param[in] addr  The address of this sub node
        //! @param[in] bus  The bus on which this node communicates
        //! @param[in] playerData  The player data passed to any connected peripheral
        DreamcastSubNode(uint8_t addr, MapleBusInterface& bus, PlayerData playerData);

        //! Copy constructor
        DreamcastSubNode(const DreamcastSubNode& rhs);

        //! Inherited from DreamcastNode
        virtual bool handleData(uint8_t len,
                                uint8_t cmd,
                                const uint32_t *payload);

        //! Inherited from DreamcastNode
        virtual void task(uint64_t currentTimeUs);

        //! Called from the main node when a main peripheral disconnects. A main peripheral
        //! disconnecting should cause all sub peripherals to disconnect.
        virtual void mainPeripheralDisconnected();

        //! Called from the main node to update the connection state of peripherals on this sub node
        virtual void setConnected(bool connected);

    private:
        //! Number of microseconds in between each info request when no peripheral is detected
        static const uint32_t US_PER_CHECK = 16000;
        //! The clock time of the next info request when no peripheral is detected
        uint64_t mNextCheckTime;
        //! Detected peripheral connection state
        bool mConnected;

};
