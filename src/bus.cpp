#include "aot/bus/bus.h"

#include "aot/bus/bus_event.h"
#include "aot/bus/bus_component.h"

void aot::Bus::AsyncSend(bus::Component* publisher, bus::Event* event) {
    auto it = subscribers_.find(publisher);
    if (it != subscribers_.end()) {
        SetNumberCopyEvent(event, it->second.size());
        for (auto component : it->second) {
            boost::asio::post(
                strand_, [component, event]() { event->Accept(component); });
        }
    }
}

void aot::Bus::SetNumberCopyEvent(bus::Event* event, size_t number) {
    for (auto i = 0; i < number; i++) {
            event->AddReference();
        }
}
