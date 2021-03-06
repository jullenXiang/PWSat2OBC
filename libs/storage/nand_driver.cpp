#include "nand.h"
#include <stdint.h>
#include "logger/logger.h"
#include "nand_driver.h"
#include "storage.h"
#include "system.h"
#include "yaffs_guts.h"
#include "yaffsfs.h"

static int ReadChunk(struct yaffs_dev* dev, int nand_chunk, u8* data, int data_len, u8* oob, int oob_len, enum yaffs_ecc_result* ecc_result)
{
    *ecc_result = YAFFS_ECC_RESULT_NO_ERROR;

    YaffsNANDDriver* driver = static_cast<YaffsNANDDriver*>(dev->driver_context);

    if (data == NULL)
    {
        data_len = 0;
    }
    if (oob == NULL)
    {
        oob_len = 0;
    }

    NANDOperation op;
    op.offset = NANDPageOffsetFromChunk(&driver->geometry, nand_chunk);
    op.dataBuffer = data;
    op.dataSize = data_len;
    op.spareBuffer = oob;
    op.spareSize = oob_len;

    const uint16_t pagesCount = NANDAffectedPagesCount(&driver->geometry, &op);

    for (uint16_t page = 0; page < pagesCount; page++)
    {
        NANDOperationSlice slice = NANDGetOperationSlice(&driver->geometry, &op, page);

        if (slice.dataSize > 0)
        {
            const FlashStatus status = driver->flash.readPage(&driver->flash, slice.offset, slice.dataBuffer, slice.dataSize);

            switch (status)
            {
                case FlashStatusOK:
                    break;
                case FlashStatusErrorCorrected:
                    *ecc_result = YAFFS_ECC_RESULT_FIXED;
                    break;
                case FlashStatusErrorNotCorrected:
                case FlashStatusChecksumCorrupted:
                    *ecc_result = YAFFS_ECC_RESULT_UNFIXED;
                    return YAFFS_FAIL;

                case FlashStatusInvalidDevice:
                case FlashStatusWriteError:
                case FlashStatusReadError:
                    return YAFFS_FAIL;
            }
        }

        if (slice.spareSize > 0)
        {
            FlashStatus status = driver->flash.readSpare(&driver->flash, slice.offset, slice.spareBuffer, slice.spareSize);

            if (status != FlashStatusOK)
            {
                return YAFFS_FAIL;
            }
        }
    }

    return YAFFS_OK;
}

static int WriteChunk(struct yaffs_dev* dev, int nand_chunk, const u8* data, int data_len, const u8* oob, int oob_len)
{
    YaffsNANDDriver* driver = static_cast<YaffsNANDDriver*>(dev->driver_context);

    if (data == NULL)
    {
        data_len = 0;
    }
    if (oob == NULL)
    {
        oob_len = 0;
    }

    NANDOperation op;
    op.offset = NANDPageOffsetFromChunk(&driver->geometry, nand_chunk);
    op.dataBuffer = (uint8_t*)data;
    op.dataSize = data_len;
    op.spareBuffer = (uint8_t*)oob;
    op.spareSize = oob_len;

    const uint16_t pagesCount = NANDAffectedPagesCount(&driver->geometry, &op);

    for (uint16_t page = 0; page < pagesCount; page++)
    {
        const NANDOperationSlice slice = NANDGetOperationSlice(&driver->geometry, &op, page);

        if (slice.dataSize > 0)
        {
            const FlashStatus status = driver->flash.writePage(&driver->flash, slice.offset, slice.dataBuffer, slice.dataSize);

            if (status != FlashStatusOK)
            {
                return YAFFS_FAIL;
            }
        }

        if (slice.spareSize > 0)
        {
            FlashStatus status = driver->flash.writeSpare(&driver->flash, slice.offset, slice.spareBuffer, slice.spareSize);

            if (status != FlashStatusOK)
            {
                return YAFFS_FAIL;
            }
        }
    }

    return YAFFS_OK;
}

static int EraseBlock(struct yaffs_dev* dev, int block_no)
{
    LOGF(LOG_LEVEL_INFO, "EraseBlock %d", block_no);

    YaffsNANDDriver* driver = static_cast<YaffsNANDDriver*>(dev->driver_context);

    uint32_t baseAddress = NANDBlockOffset(&driver->geometry, block_no);

    FlashStatus status = driver->flash.eraseBlock(&driver->flash, baseAddress);

    if (status != FlashStatusOK)
    {
        return YAFFS_FAIL;
    }

    return YAFFS_OK;
}

static int MarkBadBlock(struct yaffs_dev* dev, int block_no)
{
    LOGF(LOG_LEVEL_INFO, "MarkBadBlock %d", block_no);

    YaffsNANDDriver* driver = static_cast<YaffsNANDDriver*>(dev->driver_context);

    uint32_t blockAddress = NANDBlockOffset(&driver->geometry, block_no);

    FlashStatus status = driver->flash.markBadBlock(&driver->flash, blockAddress);

    if (status != FlashStatusOK)
    {
        return YAFFS_FAIL;
    }

    return YAFFS_OK;
}

static int CheckBadBlock(struct yaffs_dev* dev, int block_no)
{
    YaffsNANDDriver* driver = static_cast<YaffsNANDDriver*>(dev->driver_context);

    uint32_t baseAddress = NANDBlockOffset(&driver->geometry, block_no);

    if (driver->flash.isBadBlock(&driver->flash, baseAddress))
    {
        LOGF(LOG_LEVEL_ERROR, "Block %d is marked bad", block_no);
        return YAFFS_FAIL;
    }

    return YAFFS_OK;
}

static int FlashInitialize(struct yaffs_dev* dev)
{
    LOG(LOG_LEVEL_INFO, "Initializing flash");

    YaffsNANDDriver* driver = static_cast<YaffsNANDDriver*>(dev->driver_context);

    driver->flash.initialize(&driver->flash);

    return YAFFS_OK;
}

void SetupYaffsNANDDriver(struct yaffs_dev* dev, YaffsNANDDriver* driver)
{
    dev->drv.drv_check_bad_fn = CheckBadBlock;
    dev->drv.drv_erase_fn = EraseBlock;
    dev->drv.drv_initialise_fn = FlashInitialize;
    dev->drv.drv_mark_bad_fn = MarkBadBlock;
    dev->drv.drv_read_chunk_fn = ReadChunk;
    dev->drv.drv_write_chunk_fn = WriteChunk;

    dev->driver_context = driver;
}
