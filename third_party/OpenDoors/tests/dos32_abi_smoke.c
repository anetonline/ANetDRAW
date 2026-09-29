#include <stddef.h>
#include "OpenDoor.h"

#define ABI_ASSERT(name, condition) typedef char name[(condition) ? 1 : -1]

ABI_ASSERT(enum_text_format_is_int,
   sizeof(tODEditTextFormat) == sizeof(int));
ABI_ASSERT(enum_menu_result_is_int,
   sizeof(tODEditMenuResult) == sizeof(int));
ABI_ASSERT(enum_input_event_is_int,
   sizeof(tODInputEventType) == sizeof(int));
ABI_ASSERT(edit_options_size, sizeof(tODEditOptions) == 40);
ABI_ASSERT(edit_options_format_offset,
   offsetof(tODEditOptions, TextFormat) == 16);
ABI_ASSERT(edit_options_callback_offset,
   offsetof(tODEditOptions, pfMenuCallback) == 20);
ABI_ASSERT(input_event_size, sizeof(tODInputEvent) == 8);
ABI_ASSERT(input_event_remote_offset,
   offsetof(tODInputEvent, bFromRemote) == 4);
ABI_ASSERT(input_event_key_offset,
   offsetof(tODInputEvent, chKeyPress) == 5);
ABI_ASSERT(control_size, sizeof(tODControl) == 5703);
ABI_ASSERT(control_baud_offset, offsetof(tODControl, baud) == 60);
ABI_ASSERT(control_connect_offset,
   offsetof(tODControl, od_connect_speed) == 64);
ABI_ASSERT(control_address_offset,
   offsetof(tODControl, od_com_address) == 68);
ABI_ASSERT(control_handle_offset,
   offsetof(tODControl, od_open_handle) == 83);

int main(void)
{
   return(od_control_get() == &od_control ? 0 : 1);
}
