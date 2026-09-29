#ifdef _WIN32
#include <winsock2.h>
#endif

#include <stdlib.h>

#include "test_support.h"

#define LEGACY_SCREEN_BUFFER_SIZE (4U + 80U * 25U * 2U)

#ifdef ODPLAT_WIN32
static BOOL ODTestCreateSocketPair(SOCKET *pDoorSocket, SOCKET *pPeerSocket)
{
   WSADATA socket_data;
   SOCKET listener = INVALID_SOCKET;
   SOCKET door_socket = INVALID_SOCKET;
   SOCKET peer_socket = INVALID_SOCKET;
   struct sockaddr_in address;
   int address_size = sizeof(address);

   if(WSAStartup(MAKEWORD(2, 2), &socket_data) != 0)
      return(FALSE);

   listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(listener == INVALID_SOCKET)
      goto failure;

   memset(&address, 0, sizeof(address));
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
   if(bind(listener, (struct sockaddr *)&address, sizeof(address)) != 0
      || listen(listener, 1) != 0
      || getsockname(listener, (struct sockaddr *)&address,
         &address_size) != 0)
   {
      goto failure;
   }

   peer_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if(peer_socket == INVALID_SOCKET
      || connect(peer_socket, (struct sockaddr *)&address,
         sizeof(address)) != 0)
   {
      goto failure;
   }

   door_socket = accept(listener, NULL, NULL);
   if(door_socket == INVALID_SOCKET)
      goto failure;

   closesocket(listener);
   *pDoorSocket = door_socket;
   *pPeerSocket = peer_socket;
   return(TRUE);

failure:
   if(listener != INVALID_SOCKET)
      closesocket(listener);
   if(door_socket != INVALID_SOCKET)
      closesocket(door_socket);
   if(peer_socket != INVALID_SOCKET)
      closesocket(peer_socket);
   WSACleanup();
   return(FALSE);
}
#endif /* ODPLAT_WIN32 */

int main(void)
{
   unsigned char cells[12];
   unsigned char readback[12];
   unsigned char *snapshot;
   unsigned char *legacy_snapshot;
   void *window;
   DWORD snapshot_size;
   INT row;
   INT column;
   int index;
#ifdef ODPLAT_WIN32
   SOCKET door_socket;
   SOCKET peer_socket;
#endif

   ODTestConfigureLocal();
   od_control.user_ansi = TRUE;
   od_control.user_screenwidth = 100;
   od_control.user_screen_length = 30;
   od_control.od_info_type = CUSTOM;
#ifdef ODPLAT_WIN32
   OD_TEST_CHECK(ODTestCreateSocketPair(&door_socket, &peer_socket));
   od_control.baud = 38400;
   od_control.od_connect_speed = 38400;
   od_control.od_use_socket = TRUE;
   od_control.od_open_handle = (DWORD_PTR)door_socket;
#else
   od_control.baud = 1;
#endif
   fprintf(stderr, "screen: initializing\n");
   od_init();
   fprintf(stderr, "screen: initialized\n");

   for(index = 0; index < 6; ++index)
   {
      cells[index * 2] = (unsigned char)('A' + index);
      cells[index * 2 + 1] = (unsigned char)(L_WHITE | (D_BLUE << 4));
   }
   OD_TEST_CHECK(od_puttext(94, 2, 99, 2, cells));
   memset(readback, 0, sizeof(readback));
   OD_TEST_CHECK(od_gettext(94, 2, 99, 2, readback));
   OD_TEST_CHECK(memcmp(cells, readback, sizeof(cells)) == 0);
   od_control.od_error = ERR_NONE;
   OD_TEST_CHECK(!od_puttext(0, 2, 1, 2, cells));
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);
   od_control.od_error = ERR_NONE;
   OD_TEST_CHECK(!od_gettext(2, 2, 1, 2, readback));
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);

   od_set_cursor(5, 100);
   od_get_cursor(&row, &column);
   OD_TEST_CHECK(row == 5 && column == 100);
   od_set_color(L_YELLOW, D_BLUE);
   OD_TEST_CHECK(od_control.od_cur_attrib == (L_YELLOW | (D_BLUE << 4)));
   od_set_attrib(L_CYAN | (D_RED << 4));
   OD_TEST_CHECK(od_control.od_cur_attrib == (L_CYAN | (D_RED << 4)));
   od_disp("XY", 2, TRUE);
   od_disp_str("Z");
   od_putch('!');
   od_repeat('.', 3);
   od_printf("%s %d", "value", 7);
   od_disp_emu("plain", TRUE);
   od_emulate('E');
   od_clr_line();

   OD_TEST_CHECK(od_draw_box(2, 3, 20, 8));
   OD_TEST_CHECK(!od_draw_box(0, 3, 20, 8));
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);
   window = od_window_create(25, 3, 45, 8, "Title", L_WHITE, L_YELLOW,
      D_BLUE, 0);
   OD_TEST_CHECK(window != NULL);
   OD_TEST_CHECK(od_window_remove(window));
   OD_TEST_CHECK(!od_window_remove(NULL));
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);
   OD_TEST_CHECK(od_window_create(0, 3, 45, 8, "Bad", L_WHITE, L_YELLOW,
      D_BLUE, 0) == NULL);
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);
   OD_TEST_CHECK(od_scroll(2, 3, 20, 8, 1, SCROLL_NORMAL));
   OD_TEST_CHECK(od_scroll(2, 3, 20, 8, 0, SCROLL_NORMAL));
   OD_TEST_CHECK(!od_scroll(20, 3, 2, 8, 1, SCROLL_NORMAL));
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);

   snapshot_size = od_save_screen_size();
   OD_TEST_CHECK(snapshot_size != 0);
   snapshot = (unsigned char *)malloc((size_t)snapshot_size);
   OD_TEST_CHECK(snapshot != NULL);
   OD_TEST_CHECK(!od_save_screen_ex(NULL, snapshot_size));
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);
   OD_TEST_CHECK(!od_save_screen_ex(snapshot, snapshot_size - 1));
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);
   OD_TEST_CHECK(od_save_screen_ex(snapshot, snapshot_size));
   od_clr_scr();
   OD_TEST_CHECK(od_restore_screen_ex(snapshot, snapshot_size));
   OD_TEST_CHECK(!od_restore_screen_ex(snapshot, snapshot_size - 1));
   OD_TEST_CHECK(od_control.od_error == ERR_PARAMETER);
   free(snapshot);

   legacy_snapshot = (unsigned char *)malloc(LEGACY_SCREEN_BUFFER_SIZE);
   OD_TEST_CHECK(legacy_snapshot != NULL);
   OD_TEST_CHECK(od_save_screen(legacy_snapshot));
   od_clr_scr();
   OD_TEST_CHECK(od_restore_screen(legacy_snapshot));
   free(legacy_snapshot);

   fprintf(stderr, "screen: shutting down\n");
   od_exit(0, FALSE);
   fprintf(stderr, "screen: shut down\n");
#ifdef ODPLAT_WIN32
   closesocket(peer_socket);
   WSACleanup();
#endif
   return(0);
}
