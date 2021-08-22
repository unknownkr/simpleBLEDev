#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"

#include "user_data.h"

#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

#define APPLICATION_AREA_END 0x78000
#define PAGE_SIZE 0x1000

#define VALIDATE_CODE 0xEDC
#define USER_STORAGE_BANK_LEN 512


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);

static uint32_t user_storage_data[USER_STORAGE_BANK_LEN][2];

NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    /* Set a handler for fstorage events. */
    .evt_handler = fstorage_evt_handler,

    /* These below are the boundaries of the flash space assigned to this instance of fstorage.
     * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
     * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
     * last page of flash available to write data. */
    .start_addr = APPLICATION_AREA_END - PAGE_SIZE,
    .end_addr   = APPLICATION_AREA_END,
};

static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
    if (p_evt->result != NRF_SUCCESS)
    {
        NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
        return;
    }

    switch (p_evt->id)
    {
        case NRF_FSTORAGE_EVT_WRITE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        case NRF_FSTORAGE_EVT_ERASE_RESULT:
        {
            NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                         p_evt->len, p_evt->addr);
        } break;

        default:
            break;
    }
}

static void print_flash_info(nrf_fstorage_t * p_fstorage)
{
    NRF_LOG_INFO("========| flash info |========");
    NRF_LOG_INFO("erase unit: \t%d bytes",      p_fstorage->p_flash_info->erase_unit);
    NRF_LOG_INFO("program unit: \t%d bytes",    p_fstorage->p_flash_info->program_unit);
    NRF_LOG_INFO("==============================");
}

bool wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    uint32_t timeout = 250;
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        nrf_delay_ms(2);
        timeout--;
        if(timeout <= 0){
            return false;
        }
    }
    return true;
}

static void write_data(uint32_t addr, uint32_t *data, uint32_t len)
{
    NRF_LOG_INFO("Writing to 0x%08x 0x%08x 0x%08x, len(%d)", addr, data[0], data[1], len);
    NRF_LOG_FLUSH();
    ret_code_t rc = nrf_fstorage_write(&fstorage, addr, data, len, NULL);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&fstorage);
}

static void read_data(uint32_t addr, uint32_t *buffer, uint32_t len)
{
    NRF_LOG_INFO("Read 0x%08x, len(%d)", addr, len);
    NRF_LOG_FLUSH();
    ret_code_t rc = nrf_fstorage_read(&fstorage, addr, buffer, len);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&fstorage);
}

static void erase_flash(uint32_t addr, uint32_t len)
{
    NRF_LOG_INFO("Erase flash 0x%08x, len(%d)", addr, len);
    NRF_LOG_FLUSH();
    ret_code_t rc = nrf_fstorage_erase(&fstorage, addr, len, NULL);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&fstorage);
}

void user_data_storage_init()
{
    nrf_fstorage_api_t * p_fs_api;
    p_fs_api = &nrf_fstorage_sd;

    ret_code_t rc = nrf_fstorage_init(&fstorage, p_fs_api, NULL);
    APP_ERROR_CHECK(rc);
    wait_for_flash_ready(&fstorage);

    print_flash_info(&fstorage);

    read_data(fstorage.start_addr, user_storage_data, PAGE_SIZE);
}

bool write_user_data(USER_DATA_ID idx, uint32_t val)
{
    user_storage_data[idx][0] = VALIDATE_CODE;
    user_storage_data[idx][1] = val;
}

bool read_user_data(USER_DATA_ID idx, uint32_t* buffer)
{
    if(user_storage_data[idx][0] != VALIDATE_CODE) {
        NRF_LOG_INFO("Invalid data at %d(0x%08x)", idx, fstorage.start_addr + (idx * 8));
        NRF_LOG_FLUSH();
        return false;
    }

    *buffer = user_storage_data[idx][1];
}

bool flash_sync()
{
    erase_flash(fstorage.start_addr, 1);
    write_data(fstorage.start_addr, user_storage_data, sizeof(user_storage_data));
}


#if defined(USER_DATA_TEST)

void test_write_userdata()
{
    for(int i = USER_DATA_ID_00;i<USER_DATA_ID_MAX;i++){
        write_user_data(i, i);
    }
}

void test_read_userdata()
{
    uint32_t data = 0;
    for(int i = USER_DATA_ID_00 ; i<USER_DATA_ID_MAX;i++){
        read_user_data(i, &data);
        NRF_LOG_INFO("Read data: id(%d) 0x%08x", i, data);
    }
}

void test_erase_userdata()
{
    erase_flash(fstorage.start_addr, 1);
}

void test_write_n_read_userdata()
{
    uint32_t data[USER_DATA_ID_MAX] = {};
    uint32_t temp = 0xAABBCCDD;

    for(int i = USER_DATA_ID_00 ; i<USER_DATA_ID_MAX;i++){
        write_user_data(i, temp);
        data[i] = temp;
        temp -= 0x10;
    }

    flash_sync();

    uint32_t buffer;
    for(int i = USER_DATA_ID_00; i<USER_DATA_ID_MAX;i++){
        read_user_data(i, &buffer);
        if(data[i] == buffer){
            NRF_LOG_DEBUG("Data matched 0x%08x == 0x%08x", data[i], buffer );
        } else {
            NRF_LOG_DEBUG("Failed, Data not matched 0x%08x == 0x%08x", data[i], buffer );
        }
    }
}

#endif
