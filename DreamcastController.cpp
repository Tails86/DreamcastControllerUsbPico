#include "DreamcastController.hpp"
#include <string.h>


DreamcastController::DreamcastController(MapleBus& bus, uint32_t playerIndex, UsbGamepad& gamepad) :
    mBus(bus),
    mPlayerIndex(playerIndex),
    mGamepad(gamepad),
    mNextCheckTime(0),
    mWaitingForData(false),
    mNoDataCount(0),
    mControllerCondition()
{
    mGamepad.updateControllerConnected(true);
}

DreamcastController::~DreamcastController()
{
    mGamepad.updateControllerConnected(false);
}

bool DreamcastController::task(uint64_t currentTimeUs)
{
    uint32_t len;
    bool newData = false;
    const uint32_t* dat = mBus.getReadData(len, newData);

    if (newData)
    {
        // Process data
        mWaitingForData = false;
        mNoDataCount = 0;
        uint8_t cmd = *dat >> 24;
        if (cmd == MapleBus::COMMAND_RESPONSE_DATA_XFER && len >= 4 && dat[1] == 1)
        {
            // Handle condition data
            memcpy(mControllerCondition.words, &dat[2], 8);

            // TODO: Move magic numbers to enum
            mGamepad.setButton(0, 0 == mControllerCondition.a);
            mGamepad.setButton(1, 0 == mControllerCondition.b);
            mGamepad.setButton(3, 0 == mControllerCondition.x);
            mGamepad.setButton(4, 0 == mControllerCondition.y);
            mGamepad.setButton(11, 0 == mControllerCondition.start);

            // mGamepad.setButton(12, 0 == mControllerCondition.up);
            // mGamepad.setButton(13, 0 == mControllerCondition.down);
            // mGamepad.setButton(14, 0 == mControllerCondition.left);
            // mGamepad.setButton(15, 0 == mControllerCondition.right);

            mGamepad.setDigitalPad(UsbGamepad::DPAD_UP, 0 == mControllerCondition.up);
            mGamepad.setDigitalPad(UsbGamepad::DPAD_DOWN, 0 == mControllerCondition.down);
            mGamepad.setDigitalPad(UsbGamepad::DPAD_LEFT, 0 == mControllerCondition.left);
            mGamepad.setDigitalPad(UsbGamepad::DPAD_RIGHT, 0 == mControllerCondition.right);

            mGamepad.setAnalogTrigger(true, static_cast<int32_t>(mControllerCondition.l) - 128);
            mGamepad.setAnalogTrigger(false, static_cast<int32_t>(mControllerCondition.r) - 128);

            mGamepad.setAnalogThumbX(true, static_cast<int32_t>(mControllerCondition.lAnalogLR) - 128);
            mGamepad.setAnalogThumbY(true, static_cast<int32_t>(mControllerCondition.lAnalogUD) - 128);
            mGamepad.setAnalogThumbX(false, static_cast<int32_t>(mControllerCondition.rAnalogLR) - 128);
            mGamepad.setAnalogThumbY(false, static_cast<int32_t>(mControllerCondition.rAnalogUD) - 128);

            mGamepad.send();
        }
    }

    bool rv = true;
    if (currentTimeUs > mNextCheckTime)
    {
        // See if we need to update connection status to disconnected
        if (mWaitingForData)
        {
            if (++mNoDataCount >= NO_DATA_DISCONNECT_COUNT)
            {
                mNoDataCount = 0;
                mControllerCondition.reset();
                rv = false;
            }
        }
        else
        {
            // Get controller status
            uint32_t data = 1;
            if (mBus.write(MapleBus::COMMAND_GET_CONDITION, 0x20, &data, 1, true))
            {
                mWaitingForData = true;
                mNextCheckTime = currentTimeUs + US_PER_CHECK;
            }
        }
    }

    return rv;
}
