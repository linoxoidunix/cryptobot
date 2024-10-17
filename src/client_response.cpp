#include "aot/bus/bus_component.h"
#include "aot/client_response.h"

void Exchange::BusEventResponse::Accept(bus::Component* comp){
    comp->AsyncHandleEvent(this);
}
