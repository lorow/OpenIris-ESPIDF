#include <string>
#include <memory>
#include <unordered_map>
#include <mongoose.h>

#include "esp_log.h"

struct RequestContext
{
  mg_connection *connection;
};

class RestAPI
{
  using route_handler = void (RestAPI::*)(RequestContext *);
  typedef std::unordered_map<std::string, route_handler> route_map;
  std::string url;
  route_map routes;

  struct mg_mgr mgr;

private:
  void handle_reboot(RequestContext *context);

public:
  // this will also need command manager
  RestAPI(std::string url);
  void begin();
  void handle_request(struct mg_connection *connection, int event, void *event_data);
  void poll();
};

namespace RestAPIHelpers
{
  void event_handler(struct mg_connection *connection, int event, void *event_data);
};