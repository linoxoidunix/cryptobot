#include "aot/bus/bus.h"

#include "aot/bus/bus_event.h"
#include "aot/bus/bus_component.h"
#include "aot/Logger.h"
// void aot::Bus::AsyncSend(bus::Component* publisher, bus::Event* event) {
//     logd("start send order from {}", (void*)publisher);
//     auto it = subscribers_.find(publisher);
//     if (it != subscribers_.end()) {
//         logd("found {} subscribers", it->second.size());
//         SetNumberCopyEvent(event, it->second.size());
//         for (auto component : it->second) {
//             boost::asio::post(
//                 strand_, [component, event]() { 
//                     event->Accept(component); 
//                     });
//         }
//     }
// }

// void aot::Bus::SetNumberCopyEvent(bus::Event* event, size_t number) const{
//     for (auto i = 0; i < number; i++) {
//             event->AddReference();
//         }
// }

