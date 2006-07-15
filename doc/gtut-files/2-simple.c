#include <gras.h>

int server(int argc, char *argv[]) {
  gras_socket_t mysock;   /* socket on which I listen */
  gras_socket_t toclient; /* socket used to write to the client */
  
  gras_init(&argc,argv);

  gras_msgtype_declare("hello", NULL);
  mysock = gras_socket_server(12345);

  gras_msg_wait(60, gras_msgtype_by_name("hello"), &toclient, NULL /* no payload */);
  
  fprintf(stderr,"Cool, we received the message from %s:%d.\n",
   	  gras_socket_peer_name(toclient), gras_socket_peer_port(toclient));
  
  gras_exit();
  return 0;
}
int client(int argc, char *argv[]) {
  gras_socket_t mysock;   /* socket on which I listen */
  gras_socket_t toserver; /* socket used to write to the server */

  gras_init(&argc,argv);

  gras_msgtype_declare("hello", NULL);
  mysock = gras_socket_server_range(1024, 10000, 0, 0);
  
  fprintf(stderr,"Client ready; listening on %d\n", gras_socket_my_port(mysock));
  
  gras_os_sleep(1.5); /* sleep 1 second and half */
  toserver = gras_socket_client("Jacquelin", 12345);
  
  gras_msg_send(toserver,gras_msgtype_by_name("hello"), NULL);
  fprintf(stderr,"That's it, we sent the data to the server\n");

  gras_exit();
  return 0;
}
