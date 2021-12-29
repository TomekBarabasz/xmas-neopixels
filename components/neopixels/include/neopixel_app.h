namespace NeopixelApp
{
ESP_EVENT_DECLARE_BASE(NEOPIXEL_EVENTS);
esp_event_loop_handle_t create_event_loop();
extern "C" void neopixel_main(void*);
enum Commands
{
    CmdSet,
    CmdStartAnimation,
    CmdReconfigure
};
struct CmdSetArgs
{
    uint16_t num_parts;
    uint8_t  refresh;   //0:no refresh, 1:refresh w/o wait 2:refresh with wait
    struct Part
    {
        uint8_t r,g,b;
        uint16_t first, count;
    } parts[1];
};
struct CmdStartAnimationArgs
{
    uint32_t animation_id;
    uint8_t animation_prms[1];
};
struct CmdReconfigureArgs
{
    uint8_t num_segments;
    struct Segment
    {
        uint16_t num_leds;
        uint8_t strip_type;
        uint8_t gpio_num;
        uint8_t rmt_channel;
        uint8_t rmt_mem_block_num;
    } segments[1];
};
}