#include <stdint.h>

enum class ShelfCommands
{
    NONE = 0,
    CALIBRATE = 1,
    MEASURE = 2,
    ERROR = 3
};

struct ShelfData
{
    char * s1_mpn;
    int s1_qty;
    float s1_weight_per_item;
    int s1_qty_limit;

    char * s2_mpn;
    int s2_qty;
    float s2_weight_per_item;
    int s2_qty_limit;

    float calib_scalar;
    int32_t calib_offset;
    float total_weight;

    ShelfCommands command = ShelfCommands::NONE;
};

namespace Common
{
    //To set and get the data between shelf controller and message controller
    void set_shelf_data(ShelfData _data);
    ShelfData get_shelf_data();

    //Is set when message controller is started, and is connected to AWS
    void set_connected(bool connected);
    bool get_connected();

    //is set when shelf controller is started
    void set_init_complete(bool complete);
    bool get_init_complete();

    //Should be initialized from the server values when received (message controller should set this) - Acts as notify from message controller to shelf controller
    void set_shelf_data_initialized(bool data_initialized);
    bool get_shelf_data_initialized();

    //Is set from shelf controller when data is updated - Used as notify from shelf controller to message controller when new data is available
    void set_shelf_data_updated(bool data_updated);
    bool get_shelf_data_updated();
}

