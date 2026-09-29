# Compatibility interfaces

OpenDoors retains these aliases and entry points so established source and
binary consumers continue to build. New code should use the current names in
the right-hand column.

## Color spelling aliases

| Compatibility name | Current interface |
| --- | --- |
| `od_chat_colour1` | [`od_chat_color1`](control/customization.md) |
| `od_chat_colour2` | [`od_chat_color2`](control/customization.md) |
| `od_colour_char` | [`od_color_char`](control/customization.md) |
| `od_colour_delimiter` | [`od_color_delimiter`](control/customization.md) |
| `od_colour_names` | [`od_color_names`](control/customization.md) |
| `od_full_colour` | [`od_full_color`](control/customization.md) |
| `od_colour_config()` | [`od_color_config()`](api/od_color_config.md) |
| `od_set_colour()` | [`od_set_color()`](api/od_set_color.md) |

These are spelling aliases only; they do not select different behavior.

## Renamed control fields

The following names expand to members of [`od_control`](control/index.md).

| Compatibility name | Current member |
| --- | --- |
| `key_help` | [`key_status[6]`](control/customization.md) |
| `key_nohelp` | [`key_status[0]`](control/customization.md) |
| `user_credit` | [`user_net_credit`](control/caller.md) |
| `caller_netmailentered` | [`user_netmailentered`](control/caller.md) |
| `caller_echomailentered` | [`user_echomailentered`](control/caller.md) |
| `caller_logintime` | [`user_logintime`](control/caller.md) |
| `caller_logindate` | [`user_logindate`](control/caller.md) |
| `caller_timelimit` | [`user_timelimit`](control/caller.md) |
| `caller_loginsec` | [`user_loginsec`](control/caller.md) |
| `caller_credit` | [`user_net_credit`](control/caller.md) |
| `caller_userrecord`, `caller_usernum` | [`user_num`](control/caller.md) |
| `caller_readthru` | [`user_readthru`](control/caller.md) |
| `caller_numpages` | [`user_numpages`](control/caller.md) |
| `caller_downlimit` | [`user_downlimit`](control/caller.md) |
| `caller_timeofcreation` | [`user_timeofcreation`](control/caller.md) |
| `caller_logonpassword` | [`user_logonpassword`](control/caller.md) |
| `caller_wantchat` | [`user_wantchat`](control/caller.md) |
| `caller_ansi` | [`user_ansi`](control/caller.md) |
| `caller_callsign` | [`user_callsign`](control/caller.md) |
| `caller_sex` | [`user_sex`](control/caller.md) |
| `ra_deducted_time` | [`user_deducted_time`](control/caller.md) |
| `ra_menustack` | [`user_menustack`](control/caller.md) |
| `ra_menustackpointer` | [`user_menustackpointer`](control/caller.md) |
| `ra_userhandle` | [`user_handle`](control/caller.md) |
| `ra_comment` | [`user_comment`](control/caller.md) |
| `ra_firstcall` | [`user_firstcall`](control/caller.md) |
| `ra_combinedrecord` | [`user_combinedrecord`](control/caller.md) |
| `ra_birthday` | [`user_birthday`](control/caller.md) |
| `ra_subdate` | [`user_subdate`](control/caller.md) |
| `ra_screenwidth` | [`user_screenwidth`](control/caller.md) |
| `ra_msg_area` | [`user_msg_area`](control/caller.md) |
| `ra_file_area` | [`user_file_area`](control/caller.md) |
| `ra_language` | [`user_language`](control/caller.md) |
| `ra_date_format` | [`user_date_format`](control/caller.md) |
| `ra_forward_to` | [`user_forward_to`](control/caller.md) |
| `ra_error_free` | [`user_error_free`](control/caller.md) |
| `ra_sysop_next` | [`sysop_next`](control/caller.md) |
| `ra_emsi_session` | [`user_emsi_session`](control/caller.md) |
| `ra_emsi_crtdef` | [`user_emsi_crtdef`](control/caller.md) |
| `ra_emsi_protocols` | [`user_emsi_protocols`](control/caller.md) |
| `ra_emsi_capabilities` | [`user_emsi_capabilities`](control/caller.md) |
| `ra_emsi_requests` | [`user_emsi_requests`](control/caller.md) |
| `ra_emsi_software` | [`user_emsi_software`](control/caller.md) |
| `ra_hold_attr1` | [`user_hold_attr1`](control/caller.md) |
| `ra_hold_attr2` | [`user_hold_attr2`](control/caller.md) |
| `ra_hold_len` | [`user_hold_len`](control/caller.md) |
| `od_avatar` | [`user_avatar`](control/caller.md) |
| `od_rbbs_node` | [`od_node`](control/connection.md) |

## Status and color aliases

| Compatibility name | Current value |
| --- | --- |
| `B_YELLOW` | [`L_YELLOW`](constants/colors.md), equivalent to blinking brown when used as a background |
| `B_WHITE` | [`L_WHITE`](constants/colors.md), equivalent to blinking grey when used as a background |
| `STATUS_USER1` | [`STATUS_ALTERNATE_1`](constants/display.md) |
| `STATUS_USER2` | [`STATUS_ALTERNATE_2`](constants/display.md) |
| `STATUS_USER3` | [`STATUS_ALTERNATE_3`](constants/display.md) |
| `STATUS_USER4` | [`STATUS_ALTERNATE_4`](constants/display.md) |
| `STATUS_SYSTEM` | [`STATUS_ALTERNATE_5`](constants/display.md) |
| `STATUS_HELP` | [`STATUS_ALTERNATE_7`](constants/display.md) |

## Global-style configuration aliases

These macros expose current [`od_control`](control/index.md) members as legacy
global-style names.

| Compatibility name | Current member |
| --- | --- |
| `od_registered_to` | [`od_control.od_reg_name`](control/customization.md) |
| `od_registration_key` | [`od_control.od_reg_key`](control/customization.md) |
| `od_program_name` | [`od_control.od_prog_name`](control/customization.md) |
| `od_log_messages` | [`od_control.od_logfile_messages`](control/customization.md) |
| `od_config_text` | [`od_control.od_cfg_text`](control/customization.md) |
| `od_config_lines` | [`od_control.od_cfg_lines`](control/customization.md) |
| `od_config_colours`, `od_config_colors` | [`od_control.od_color_names`](control/customization.md) |
| `config_file` | [`od_control.od_config_file`](control/customization.md) |
| `config_filename` | [`od_control.od_config_filename`](control/customization.md) |
| `config_function` | [`od_control.od_config_function`](control/customization.md) |
| `default_personality` | [`od_control.od_default_personality`](control/customization.md) |
| `logfile` | [`od_control.od_logfile`](control/customization.md) |
| `mps` | [`od_control.od_mps`](control/customization.md) |

`od_kernal()` is a retained misspelling of [`od_kernel()`](api/od_kernel.md).

## Obsolete entry points

`od_init_with_config` is an obsolete macro which assigns the built-in
configuration component and then calls [`od_init()`](api/od_init.md). New code
should make those assignments explicitly. `od_log_open()` and `od_emulate()`
are obsolete public entry points retained for older applications; use the
logging component and [`od_disp_emu()`](api/od_disp_emu.md) in new work.

Optional component routines `ODConfigInit`, `ODLogEnable`, and `ODMPSEnable`
and built-in personality procedures are normally selected through the
[`INCLUDE_*`](constants/components.md) and
[`PER_*`](constants/components.md) macros rather than called directly.

Installed packages contain [`OpenDoor.h`](api/index.md), the general OpenDoors
API, and [`ODStat.h`](personality/index.md), the DOS personality SDK. Other
module headers are private implementation interfaces. They remain in the
source tree for building OpenDoors but are not installed or covered by the
public compatibility guarantees described on this site.
