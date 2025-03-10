
namespace Display
{
    enum ActiveShelf
    {
        NONE,
        SHELF_1,
        SHELF_2
    };

    void start();
    void load_splash_screen();
    void load_main_screen();
    void set_active_shelf(ActiveShelf shelf);
    void set_shelf_data(ActiveShelf shelf, const char *mpn, const char *qty);
}