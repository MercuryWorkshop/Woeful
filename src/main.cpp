#include "server.h"
#include <cstdio>
#include <cstdlib>
#include <string>

int main(int, char *[]) {
  int num_port = 9001;

  {
    char *env_port = getenv("PORT");
    if (env_port == NULL)
      goto env_port_break;

    int converted_port = atoi(env_port);
    if (converted_port == 0)
      goto env_port_break;
    num_port = converted_port;
  }

env_port_break:
  // TODO: cli args
  start_wisp_server(num_port);
}
