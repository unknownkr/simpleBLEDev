#if !defined(USER_DATA_H)
#define USER_DATA_H

typedef enum{
    USER_DATA_ID_00,
    USER_DATA_ID_01,
    USER_DATA_ID_02,
    USER_DATA_ID_03,
    USER_DATA_ID_04,
    USER_DATA_ID_05,
    USER_DATA_ID_06,
    USER_DATA_ID_07,
    USER_DATA_ID_08,
    USER_DATA_ID_09,
    USER_DATA_ID_10,
    USER_DATA_ID_11,
    USER_DATA_ID_12,
    USER_DATA_ID_13,
    USER_DATA_ID_14,
    USER_DATA_ID_15,
    USER_DATA_ID_MAX // End index of user data, User data idx can be increased to 512
} USER_DATA_ID;

void user_data_storage_init();
bool write_user_data(USER_DATA_ID idx, uint32_t val);
bool read_user_data(USER_DATA_ID idx, uint32_t* buffer);
bool erase_user_data(USER_DATA_ID idx);
bool flash_sync();

#define USER_DATA_TEST

#if defined(USER_DATA_TEST)
void test_write_userdata();
void test_read_userdata();
void test_erase_userdata();
void test_write_n_read_userdata();
#endif

#endif